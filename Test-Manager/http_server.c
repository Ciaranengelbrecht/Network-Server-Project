#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "http_server.h"
#include <netdb.h>
#include <ctype.h>

#define MAX_REQUEST_SIZE 2048
#define MAX_RESPONSE_SIZE 2048
#define TOKEN_LENGTH 32
#define SESSION_FILE "sessions_tokens.txt"
/////////////QB connection/////////////

#define CONFIG_FILE "config.txt"

char *getServerAddress()
{
    FILE *configFile = fopen("config.txt", "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *serverAddress = NULL;

    if (configFile == NULL)
    {
        printf("Error: could not open config file.\n");
        return NULL;
    }

    while ((read = getline(&line, &len, configFile)) != -1)
    {
        if (strncmp(line, "QB_SERVER_ADDRESS", 17) == 0)
        {
            // Extract the server address
            char *address = strchr(line, '=') + 1;
            while (*address == ' ')
                address++;
            serverAddress = strdup(address);
            break;
        }
    }

    fclose(configFile);
    if (line)
        free(line);

    if (serverAddress != NULL)
    {
        printf("Server IP address: %s\n", serverAddress);
    }

    return serverAddress;
}

const char *extractIsCorrect(const char *response)
{
    const char *isCorrect = strstr(response, "isCorrect");
    if (isCorrect != NULL)
    {
        isCorrect += strlen("isCorrect") + 3; // Skip ": "
        char value[6];
        int i = 0;
        while (isCorrect[i] != ',' && isCorrect[i] != '}' && isCorrect[i] != '\0')
        {
            value[i] = isCorrect[i];
            i++;
        }
        value[i] = '\0';
        return strdup(value);
    }
    return "Invalid"; // Invalid value or field not found
}

#define TRUE_STR "true"
#define FALSE_STR "false"

int mark_attempt(char *qb_evaluation, int num_attempts)
{
    // parse "isCorrect" value
    char *isCorrect_str = strstr(qb_evaluation, "\"isCorrect\": ");
    if (isCorrect_str == NULL)
        return -2; // parsing error

    isCorrect_str += strlen("\"isCorrect\": "); // move to the value

    int isCorrect = 0;
    if (strncmp(isCorrect_str, TRUE_STR, strlen(TRUE_STR)) == 0)
        isCorrect = 1;
    else if (strncmp(isCorrect_str, FALSE_STR, strlen(FALSE_STR)) == 0)
        isCorrect = 0;
    else
        return -2; // parsing error

    // apply logic
    if (isCorrect)
    {
        switch (num_attempts)
        {
        case 1:
            return 3;
        case 2:
            return 2;
        case 3:
            return 1;
        default:
            return 0;
        }
    }
    else
    {
        if (num_attempts < 3)
            return -1;
        else
            return 0;
    }
}
// Helper function to format the sequence in the response
char *formatSequence(const char *response)
{
    const char *start = strstr(response, "\"sequence\":");
    if (start != NULL)
    {
        start += strlen("\"sequence\":");
        const char *end = strchr(start, ']');
        if (end != NULL)
        {
            size_t sequenceLength = end - start + 1;
            char *sequence = malloc(sequenceLength + 1);
            strncpy(sequence, start, sequenceLength);
            sequence[sequenceLength] = '\0';

            // Remove any extra characters and formatting
            char *formattedSequence = malloc(sequenceLength + 1);
            char *formattedPtr = formattedSequence;
            int insideQuotes = 0;
            for (size_t i = 0; i < sequenceLength; i++)
            {
                char ch = sequence[i];
                if (ch == '\"')
                {
                    insideQuotes = !insideQuotes;
                }
                else if (insideQuotes || ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t')
                {
                    *formattedPtr++ = ch;
                }
            }
            *formattedPtr = '\0';

            free(sequence);
            return formattedSequence;
        }
    }
    return "";
}

void extractBracketGroupsForUser(const char *filename, const char *username, char *group1, char *group2)
{
    FILE *file = fopen(filename, "r");
    if (file)
    {
        char line[256];
        while (fgets(line, sizeof(line), file))
        {
            // Tokenize the line to extract username, group1, and group2
            char *token = strtok(line, " ");
            if (token && strcmp(token, username) == 0)
            {
                // User found, extract the bracket groups
                token = strtok(NULL, " ");
                if (token)
                {
                    // Skip the first bracket group
                    token = strtok(NULL, " ");
                    if (token)
                    {
                        // Remove the trailing newline character
                        char *newline = strchr(token, '\n');
                        if (newline)
                        {
                            *newline = '\0';
                        }
                        strcpy(group1, token);
                    }
                    // Extract the second bracket group
                    token = strtok(NULL, " ");
                    if (token)
                    {
                        // Remove the trailing newline character
                        char *newline = strchr(token, '\n');
                        if (newline)
                        {
                            *newline = '\0';
                        }
                        strcpy(group2, token);
                    }
                }
                break; // Exit the loop after finding the user
            }
        }
        fclose(file);
    }
}

int connectToServer(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(getServerAddress());
    serv_addr.sin_port = htons(port);

    int connection_attempt = 1;
    int max_attempts = 2;
    int delay_seconds = 5;
    char response[MAX_RESPONSE_SIZE];
    memset(response, 0, sizeof(response));
    while (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        if (connection_attempt > max_attempts)
        {
            printf("Failed to connect to QB server after %d attempts\n", max_attempts);
            // maybe send user to login failed page here
            exit(EXIT_FAILURE);
        }

        printf("Attempt %d: Waiting for QB server to become available...\n", connection_attempt);
        sleep(delay_seconds);
        connection_attempt++;
    }

    printf("Connected to QB server!\n");

    return sockfd;
}

void saveUserAnswer(const char *username, const char *questionId, const char *answer)
{
    const char *filename = "progress.txt";
    const char *tempFilename = "progress_temp.txt";

    // Check if the file exists, if not, create it
    if (access(filename, F_OK) != 0) {
        FILE *file = fopen(filename, "w");
        if (file == NULL) {
            perror("Failed to create progress.txt");
            return;
        }
        fclose(file);
    }

    FILE *file = fopen(filename, "r");
    FILE *tempFile = fopen(tempFilename, "w");

    if (file == NULL || tempFile == NULL)
    {
        perror("Failed to open file");
        return;
    }

    char line[1024];
    char target[1024];
    sprintf(target, "%s-%s:", username, questionId);
    bool found = false;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strstr(line, target) == line)
        {
            fprintf(tempFile, "%s-%s: %s\n", username, questionId, answer);
            found = true;
        }
        else
        {
            fputs(line, tempFile);
        }
    }

    // If the question-answer pair was not found, add it to the end
    if (!found)
    {
        fprintf(tempFile, "%s-%s: %s\n", username, questionId, answer);
    }

    fclose(file);
    fclose(tempFile);

    // Delete the original file and rename the temporary file
    remove(filename);
    rename(tempFilename, filename);
}

void recordMarks(const char *username, const char *questionId, int mark, int attempts) {
    // Check if the file exists, if not, create it
    if (access("marks.txt", F_OK) != 0) {
        FILE *file = fopen("marks.txt", "w");
        if (file == NULL) {
            perror("Failed to create marks.txt");
            return;
        }
        fclose(file);
    }

    // Open the file for reading
    FILE *file = fopen("marks.txt", "r");
    if (file == NULL) {
        perror("Failed to open marks.txt");
        return;
    }

    // Read all lines into a dynamic array
    char **lines = NULL;
    size_t count = 0;
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        lines = realloc(lines, (count + 1) * sizeof(char *));
        lines[count] = strdup(line);
        count++;
    }
    fclose(file);

    // Prepare the new record
    char newRecord[1024];
    sprintf(newRecord, "%s %s %d %d\n", username, questionId, mark, attempts);

    // Find the line to update
    size_t i;
    for (i = 0; i < count; i++) {
        char tempUsername[100];
        char tempQuestionId[100];
        sscanf(lines[i], "%s %s", tempUsername, tempQuestionId);
        if (strcmp(tempUsername, username) == 0 && strcmp(tempQuestionId, questionId) == 0) {
            // Update the line
            free(lines[i]);
            lines[i] = strdup(newRecord);
            break;
        }
    }

    // If the line was not found, append it
    if (i == count) {
        lines = realloc(lines, (count + 1) * sizeof(char *));
        lines[count] = strdup(newRecord);
        count++;
    }

    // Write all lines back to the file
    file = fopen("marks.txt", "w");
    if (file == NULL) {
        perror("Failed to open marks.txt");
        // Don't forget to free the lines
        for (i = 0; i < count; i++) {
            free(lines[i]);
        }
        free(lines);
        return;
    }

    for (i = 0; i < count; i++) {
        fputs(lines[i], file);
        free(lines[i]);
    }
    free(lines);
    fclose(file);

}

int getAttemptNumber(const char* username, int questionId) {
    // Check if the file exists, if not, create it
    if (access("marks.txt", F_OK) != 0) {
        FILE *file = fopen("marks.txt", "w");
        if (file == NULL) {
            perror("Failed to create progress.txt");
            return -1;
        }
        fclose(file);
    }

    FILE* file = fopen("marks.txt", "r");
    if (file == NULL) {
        printf("Failed to open marks.txt\n");
    }

    char currUsername[100];
    int currQuestionId, mark, attemptNumber;
    int found = 0; // Flag to indicate if the username and questionId were found

    while (fscanf(file, "%s %d %d %d", currUsername, &currQuestionId, &mark, &attemptNumber) == 4) {
        if (strcmp(username, currUsername) == 0 && questionId == currQuestionId) {
            found = 1;
            break;
        }
    }

    fclose(file);

    if (found) {
        return attemptNumber;
    } else {
        return 1; // Return a default value if the username and questionId were not found
    }
}
int updateAttemptNumber(const char* username, int questionId) {
    // Check if the file exists, if not, create it
    if (access("marks.txt", F_OK) != 0) {
        FILE *file = fopen("marks.txt", "w");
        if (file == NULL) {
            perror("Failed to create progress.txt");
            return -1;
        }
        fclose(file);
    }

    FILE* file = fopen("marks.txt", "r");
    if (file == NULL) {
        printf("Failed to open marks.txt\n");
    }

    char currUsername[100];
    int currQuestionId, mark, attemptNumber;
    int found = 0; // Flag to indicate if the username and questionId were found

    while (fscanf(file, "%s %d %d %d", currUsername, &currQuestionId, &mark, &attemptNumber) == 4) {
        if (strcmp(username, currUsername) == 0 && questionId == currQuestionId) {
            found = 1;
            break;
        }
    }
    

    fclose(file);

    if (found) {
        return attemptNumber;
    } else {
        return 0; // Return a default value if the username and questionId were not found
    }
}
int getMark(const char* username, int questionId) {
    // Check if the file exists, if not, create it
    if (access("marks.txt", F_OK) != 0) {
        FILE *file = fopen("marks.txt", "w");
        if (file == NULL) {
            perror("Failed to create progress.txt");
            return -1;
        }
        fclose(file);
    }

    FILE* file = fopen("marks.txt", "r");
    if (file == NULL) {
        printf("Failed to open marks.txt\n");
        return -1; // Return a default value or handle the error as needed
    }

    char currUsername[100];
    int currQuestionId, mark, attemptNumber;
    int found = 0; // Flag to indicate if the username and questionId were found

    while (fscanf(file, "%s %d %d %d", currUsername, &currQuestionId, &mark, &attemptNumber) == 4) {
        if (strcmp(username, currUsername) == 0 && questionId == currQuestionId) {
            found = 1;
            break;
        }
    }

    fclose(file);

    if (found) {
        return mark;
    } else {
        return -1; // Return a default value if the username and questionId were not found
    }
}

void http_handle_request(int clientSocket, int sockfd_c_qb, int sockfd_java_qb)
{
    // Read the request from the client
    // ...
    char request[2048];

    ssize_t bytesRead = recv(clientSocket, request, sizeof(request) - 1, 0);
    if (bytesRead == -1)
    {
        perror("Failed to read request");
        close(clientSocket);
        return;
    }
    request[bytesRead] = '\0';
    printf("Received request:\n%s\n", request); // Print the received request for debugging

    // Check if the request is for the login page
    if (strstr(request, "GET /login.html HTTP/1.1\r\n") != NULL)
    {
        FILE *file = fopen("login.html", "r");
        if (file == NULL)
        {
            perror("Failed to open login.html");
            close(clientSocket);
            return;
        }

        // Send the HTTP response headers
        char responseHeaders[200] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        send(clientSocket, responseHeaders, strlen(responseHeaders), 0);

        // Read and send the contents of the login.html file
        char buffer[2048];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
        {
            send(clientSocket, buffer, bytesRead, 0);
        }

        fclose(file);
    }

    // Check request for question_navigation page
    else if (strstr(request, "GET /question_navigation.html HTTP/1.1\r\n") != NULL)
    {
        FILE *file = fopen("question_navigation.html", "r");
        if (file == NULL)
        {
            perror("Failed to open question_navigation.html");
            close(clientSocket);
            return;
        }
        char *cookieHeader = strstr(request, "Cookie: ");
        if (cookieHeader != NULL)
        {
            // Extract the cookie value returned from client
            char *cookieStart = cookieHeader + strlen("Cookie: ");
            char *cookieEnd = strstr(cookieStart, "\r\n");
            if (cookieEnd != NULL)
            {
                size_t cookieLength = cookieEnd - cookieStart;
                char cookie[cookieLength + 1];
                strncpy(cookie, cookieStart, cookieLength);
                cookie[cookieLength] = '\0';

                // exact cookie string
                char *cookieString = cookie;
                char *sessionToken = strchr(cookieString, '=');

                if (sessionToken != NULL)
                {
                    sessionToken++; // Move the pointer to the next character
                }
                if (isSessionValid(sessionToken))
                {
                    // Send the HTTP response headers
                    char responseHeaders[200] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                    send(clientSocket, responseHeaders, strlen(responseHeaders), 0);

                    // Read and send the contents of the question_navigation.html file
                    char buffer[2048];
                    size_t bytesRead;
                    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
                    {
                        send(clientSocket, buffer, bytesRead, 0);
                    }

                    fclose(file);
                }
            }
        }
    }

    else if (strstr(request, "GET /get-questions HTTP/1.1\r\n") != NULL)
    {
        char *cookieHeader = strstr(request, "Cookie: ");
        if (cookieHeader != NULL)
        {
            // Extract the cookie value returned from client
            char *cookieStart = cookieHeader + strlen("Cookie: ");
            char *cookieEnd = strstr(cookieStart, "\r\n");
            if (cookieEnd != NULL)
            {
                size_t cookieLength = cookieEnd - cookieStart;
                char cookie[cookieLength + 1];
                strncpy(cookie, cookieStart, cookieLength);
                cookie[cookieLength] = '\0';

                // exact cookie string
                char *cookieString = cookie;
                char *sessionToken = strchr(cookieString, '=');

                if (sessionToken != NULL)
                {
                    sessionToken++; // Move the pointer to the next character
                }
                if (isSessionValid(sessionToken))
                {
                    char response1[2048];
                    char response2[2048];

                    // Open the file in 'a+' mode. This will create the file if it doesn't exist
                    FILE *sequenceFile = fopen("sequence_ids.txt", "a+");
                    if (sequenceFile != NULL)
                    {
                        char line[2048];
                        int usernameExists = 0;
                        // Move the file pointer to the beginning to read from the start
                        fseek(sequenceFile, 0, SEEK_SET);
                        while (fgets(line, sizeof(line), sequenceFile) != NULL)
                        {
                            char usernameFromFile[2048];
                            sscanf(line, "%s", usernameFromFile);
                            if (strcmp(usernameFromFile, getUsername(sessionToken)) == 0)
                            {
                                usernameExists = 1;
                                break; // Username already exists, break the loop
                            }
                        }

                        if (!usernameExists)
                        {
                            usleep(50000);
                            // QB comms
                            const char *req = "{\"type\": \"request_random_sequence\",\"num_questions\": 5}";
                            send(sockfd_c_qb, req, strlen(req), 0);
                            send(sockfd_java_qb, req, strlen(req), 0);
                            // Receive and print the response from the server
                            usleep(50000);
                            memset(response1, 0, sizeof(response1));
                            if (recv(sockfd_c_qb, response1, sizeof(response1) - 1, 0) < 0)
                            {
                                perror("Response receiving failed");
                            }
                            memset(response2, 0, sizeof(response2));
                            if (recv(sockfd_java_qb, response2, sizeof(response2) - 1, 0) < 0)
                            {
                                perror("Response receiving failed");
                            }

                            char formattedResponse1[2048];
                            char formattedResponse2[2048];
                            usleep(50000);
                            sprintf(formattedResponse1, "%s", formatSequence(response1));
                            sprintf(formattedResponse2, "%s", formatSequence(response2));
                            printf("%s\n", formattedResponse1);
                            printf("%s\n", formattedResponse2);
                            // Write the username, session token, and sequences to the file
                            fprintf(sequenceFile, "%s %s %s %s\n", getUsername(sessionToken), sessionToken, formattedResponse1, formattedResponse2);
                        }

                        fclose(sequenceFile);
                    }
                    else
                    {
                        perror("Failed to open sequence_ids.txt");
                        return;
                    }

                    char group1[256];
                    char group2[256];
                    extractBracketGroupsForUser("sequence_ids.txt", getUsername(sessionToken), group1, group2);
                    printf("Group 1: %s\n", group1);
                    printf("Group 2: %s\n", group2);

                    char reqc[2048];
                    sprintf(reqc, "{\"type\": \"request_questions\", \"questionsequence\": %s}", group1);
                    char reqjava[2048];
                    sprintf(reqjava, "{\"type\": \"request_questions\", \"questionsequence\": %s}", group2);
                    // Send QB the request
                    usleep(50000);
                    send(sockfd_c_qb, reqc, strlen(reqc), 0);
                    send(sockfd_java_qb, reqjava, strlen(reqjava), 0);
                    char response3[2048];
                    char response4[2048];
                    // Receive and print the response from the server
                    memset(response3, 0, sizeof(response3));
                    if (recv(sockfd_c_qb, response3, sizeof(response3) - 1, 0) < 0)
                    {
                        perror("Response receiving failed");
                    }
                    memset(response4, 0, sizeof(response4));
                    if (recv(sockfd_java_qb, response4, sizeof(response4) - 1, 0) < 0)
                    {
                        perror("Response receiving failed");
                    }
                    printf("Response received: %s\n", response3);
                    printf("Response received: %s\n", response4);

                    // Combine both JSON strings into one
                    char combinedJson[8192]; // Make sure the size is large enough
                    sprintf(combinedJson, "{\"CQuestions\": %s, \"javaQuestions\": %s}", response3, response4);

                    // Send the HTTP response headers
                    char responseHeaders[200] = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
                    send(clientSocket, responseHeaders, strlen(responseHeaders), 0);

                    // Send the JSON data
                    send(clientSocket, combinedJson, strlen(combinedJson), 0);
                }
            }
        }
    }

    else if (strstr(request, "POST /login.html HTTP/1.1\r\n") != NULL)
    {
        // Find the position of the username and password in the request
        char *usernameStart = strstr(request, "username=");
        if (usernameStart == NULL)
        {
            perror("Invalid request: missing username");
            close(clientSocket);
            return;
        }
        char *passwordStart = strstr(request, "password=");
        if (passwordStart == NULL)
        {
            perror("Invalid request: missing password");
            close(clientSocket);
            return;
        }

        // Extract the username and password from the request
        char username[100];
        char password[100];
        sscanf(usernameStart + strlen("username="), "%[^&\r\n]", username);
        sscanf(passwordStart + strlen("password="), "%[^&\r\n]", password);
        handleLogin(clientSocket, username, password);
    }

    else if (strstr(request, "POST /logout HTTP/1.1\r\n") != NULL)
    {
        // Retrieve the session token from the request
        // Handle the logout request

        char *cookieHeader = strstr(request, "Cookie: ");
        if (cookieHeader != NULL)
        {
            // Extract the cookie value returned from client
            char *cookieStart = cookieHeader + strlen("Cookie: ");
            char *cookieEnd = strstr(cookieStart, "\r\n");
            if (cookieEnd != NULL)
            {
                size_t cookieLength = cookieEnd - cookieStart;
                char cookie[cookieLength + 1];
                strncpy(cookie, cookieStart, cookieLength);
                cookie[cookieLength] = '\0';

                // exact cookie string
                char *cookieString = cookie;
                char *sessionToken = strchr(cookieString, '=');
                if (sessionToken != NULL)
                {
                    sessionToken++; // Move the pointer to the next character
                    printf("logout successful: %s\n", sessionToken);
                }
                printf("Logging out user with session token: %s\n", sessionToken);

                handleLogout(clientSocket, sessionToken);
            }
        }
    }

    else if (strstr(request, "POST /process-answers HTTP/1.1\r\n") != NULL)
    {
        char *cookieHeader = strstr(request, "Cookie: ");
        if (cookieHeader != NULL)
        {
            // Extract the cookie value returned from client
            char *cookieStart = cookieHeader + strlen("Cookie: ");
            char *cookieEnd = strstr(cookieStart, "\r\n");
            if (cookieEnd != NULL)
            {
                size_t cookieLength = cookieEnd - cookieStart;
                char cookie[cookieLength + 1];
                strncpy(cookie, cookieStart, cookieLength);
                cookie[cookieLength] = '\0';

                // exact cookie string
                char *cookieString = cookie;
                char *sessionToken = strchr(cookieString, '=');
                if (sessionToken != NULL)
                {
                    sessionToken++; // Move the pointer to the next character
                    printf("Received cookie: %s\n", sessionToken);
                }

                // Extract the payload from the request
                char *payloadStart = strstr(request, "\r\n\r\n");
                if (payloadStart == NULL)
                {
                    perror("Invalid request: missing payload");
                    close(clientSocket);
                    return;
                }
                payloadStart += 4; // Move the pointer past the "\r\n\r\n"

                // Parse the payload and save the answers
                char *lineStart = payloadStart;
                char *lineEnd;

                char *allAnswers = calloc(2048, sizeof(char));
                while ((lineEnd = strstr(lineStart, "\n")) != NULL)
                {
                    *lineEnd = '\0'; // Replace the newline character with a null character

                    // Extract the question ID and the answer
                    char questionId[100];
                    char answer[1024];
                    printf("CHECK ME: %s\n", lineStart);

                    sscanf(lineStart, "%[^:]: %[^\n]", questionId, answer);

                    // Save the answer
                    if (strcmp(questionId, "username") != 0)
                    { // Ignore the username line
                        saveUserAnswer(getUsername(sessionToken), questionId, answer);
                        // QB comms
                        char req[2048];

                        int qid = atoi(questionId);
                        printf("MY QID IS: %d\n", qid);

                        sprintf(req, "{\"type\": \"submit_answer\", \"questionID\": %s, \"answer\": \"%s\"}", questionId, answer);
                        // C question marking
                        if (qid >= 21)
                        {
                            send(sockfd_c_qb, req, strlen(req), 0);
                            char response1[1024];
                            memset(response1, 0, sizeof(response1));
                            if (recv(sockfd_c_qb, response1, sizeof(response1) - 1, 0) < 0)
                            {
                                perror("Response receiving failed");
                            }
                            const char *isCorrect1 = extractIsCorrect(response1);

                            printf("Answer mark: %d\n", mark_attempt(response1, 1));

                            printf("Answer correct: %s\n", isCorrect1);

                            // Record the mark and attempts
                            if (getMark(getUsername(sessionToken),  qid) != 3 && getAttemptNumber(getUsername(sessionToken),  qid) < 3 && getMark(getUsername(sessionToken),  qid) !=2  && getMark(getUsername(sessionToken),  qid) !=1){
                            recordMarks(getUsername(sessionToken), questionId, mark_attempt(response1, updateAttemptNumber(getUsername(sessionToken),  qid)+1), updateAttemptNumber(getUsername(sessionToken),  qid)+1);}
                            //send through the correct answer after 3 wrong attempts
                            if (getAttemptNumber(getUsername(sessionToken),  qid) == 3 && getMark(getUsername(sessionToken),  qid) ==0){
                                char ansreq[2048];
                                sprintf(ansreq, "{\"type\": \"request_answer\", \"questionID\": %s}", questionId);
                                send(sockfd_c_qb, ansreq, strlen(ansreq), 0);

                                char responseAns[1024];
                                memset(responseAns, 0, sizeof(responseAns));
                                if (recv(sockfd_c_qb, responseAns, sizeof(responseAns) - 1, 0) < 0)
                                {
                                    perror("Response receiving failed");
                                } ///send to HTML

                                strcat(allAnswers, responseAns);
                                strcat(allAnswers, ",");
                                printf("allanswers is %s\n", allAnswers);

                                printf("Answer is %s\n", responseAns);
                            }
                        }
                        

                        // Java question marking
                        else if (qid >= 11 && qid <= 20)
                        {
                            send(sockfd_java_qb, req, strlen(req), 0);
                            char response2[1024];
                            memset(response2, 0, sizeof(response2));
                            if (recv(sockfd_java_qb, response2, sizeof(response2) - 1, 0) < 0)
                            {
                                perror("Response receiving failed");
                            }
                            const char *isCorrect2 = extractIsCorrect(response2);
                            printf("Answer mark: %d\n", mark_attempt(response2, 1));

                            printf("Answer correct: %s\n", isCorrect2);

                            // Record the mark and attempts
                            if (getMark(getUsername(sessionToken),  qid) != 3 && getAttemptNumber(getUsername(sessionToken),  qid) < 3 && getMark(getUsername(sessionToken),  qid) !=2  && getMark(getUsername(sessionToken),  qid) !=1){
                            recordMarks(getUsername(sessionToken), questionId, mark_attempt(response2, updateAttemptNumber(getUsername(sessionToken),  qid)+1), updateAttemptNumber(getUsername(sessionToken),  qid)+1);}
                            //send through the correct answer after 3 wrong attempts
                            if (getAttemptNumber(getUsername(sessionToken),  qid) == 3 && getMark(getUsername(sessionToken),  qid) ==0){
                                char ansreq[2048];
                                sprintf(ansreq, "{\"type\": \"request_answer\", \"questionID\": %s}", questionId);
                                send(sockfd_java_qb, ansreq, strlen(ansreq), 0);

                                char responseAns[1024];
                                memset(responseAns, 0, sizeof(responseAns));
                                if (recv(sockfd_java_qb, responseAns, sizeof(responseAns) - 1, 0) < 0)
                                {
                                    perror("Response receiving failed");
                                } ///send to HTML

                                strcat(allAnswers, responseAns);
                                strcat(allAnswers, ",");
                                printf("allanswers is %s\n", allAnswers);

                                printf("Answer is %s\n", responseAns);
                            }
                        }
                    }

                    lineStart = lineEnd + 1; // Move to the next line
                }
                /* Send the response to the client */

                if (allAnswers == NULL){
                    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nForm submitted successfully!";
                    send(clientSocket, response, strlen(response), 0);
                }
                else{
                    // Remove the trailing comma if it exists
                    if (strlen(allAnswers) > 0 && allAnswers[strlen(allAnswers) - 1] == ',') {
                        allAnswers[strlen(allAnswers) - 1] = '\0';
                    }
                    char formattedAnswers[2048];
                    sprintf(formattedAnswers, "[%s]", allAnswers);
                    free(allAnswers);
                    allAnswers = strdup(formattedAnswers);

                    char httpResponse[2048];
                    sprintf(httpResponse, "HTTP/1.1 200 OK\r\n" "Content-Type: application/json\r\n" "Content-Length: %zu\r\n" "\r\n" "%s", strlen(allAnswers), allAnswers);
                    printf("this is the httpresponse%s\n", httpResponse);
                    send(clientSocket, httpResponse, strlen(httpResponse), 0);
                }
                free(allAnswers);
            }
        }
    }

    else if (strstr(request, "GET /get-progress") != NULL)
    {
        char *cookieHeader = strstr(request, "Cookie: ");
        if (cookieHeader != NULL)
        {
            // Extract the cookie value returned from client
            char *cookieStart = cookieHeader + strlen("Cookie: ");
            char *cookieEnd = strstr(cookieStart, "\r\n");
            if (cookieEnd != NULL)
            {
                size_t cookieLength = cookieEnd - cookieStart;
                char cookie[cookieLength + 1];
                strncpy(cookie, cookieStart, cookieLength);
                cookie[cookieLength] = '\0';

                // exact cookie string
                char *cookieString = cookie;
                char *sessionToken = strchr(cookieString, '=');
                if (sessionToken != NULL)
                {
                    sessionToken++; // Move the pointer to the next character
                    printf("Received cookie: %s\n", sessionToken);
                }

                // Extract the username from the request
                char *username = getUsername(sessionToken);

                // Check if the file exists, if not, create it
                if (access("progress.txt", F_OK) != 0) {
                    FILE *file = fopen("progress.txt", "w");
                    if (file == NULL) {
                        perror("Failed to create progress.txt");
                        return;
                    }
                    fclose(file);
                }
                
                // Open the progress file
                FILE *file = fopen("progress.txt", "r");
                if (file == NULL)
                {
                    perror("Failed to open progress.txt");
                    close(clientSocket);
                    return;
                }

                // Send the HTTP response headers
                char responseHeaders[200] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
                send(clientSocket, responseHeaders, strlen(responseHeaders), 0);

                // Read the progress file and send the lines that start with the username
                char line[1024];
                char target[1024];
                sprintf(target, "%s", username);
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (strstr(line, target) == line)
                    {
                        send(clientSocket, line, strlen(line), 0);
                    }
                }

                fclose(file);
            }
        }
    }

    else if (strstr(request, "GET /get-marks") != NULL) {
        char *cookieHeader = strstr(request, "Cookie: ");
        if (cookieHeader != NULL)
        {
            // Extract the cookie value returned from client
            char *cookieStart = cookieHeader + strlen("Cookie: ");
            char *cookieEnd = strstr(cookieStart, "\r\n");
            if (cookieEnd != NULL)
            {
                size_t cookieLength = cookieEnd - cookieStart;
                char cookie[cookieLength + 1];
                strncpy(cookie, cookieStart, cookieLength);
                cookie[cookieLength] = '\0';

                // extract cookie string
                char *cookieString = cookie;
                char *sessionToken = strchr(cookieString, '=');
                if (sessionToken != NULL)
                {
                    sessionToken++; // Move the pointer to the next character
                    printf("Received cookie: %s\n", sessionToken);
                }

                // Extract the username from the request
                char *username = getUsername(sessionToken);

                // Check if the file exists, if not, create it
                if (access("marks.txt", F_OK) != 0) {
                    FILE *file = fopen("marks.txt", "w");
                    if (file == NULL) {
                        perror("Failed to create marks.txt");
                        return;
                    }
                    fclose(file);
                }

                // Open the marks file
                FILE *file = fopen("marks.txt", "r");
                if (file == NULL)
                {
                    perror("Failed to open marks.txt");
                    close(clientSocket);
                    return;
                }

                // Send the HTTP response headerns
                char responseHeaders[200] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
                send(clientSocket, responseHeaders, strlen(responseHeaders), 0);

                // Read the marks file and send the lines that start with the username
                char line[1024];
                char target[1024];
                sprintf(target, "%s", username);

                // Start forming the jsonArray
                char jsonArray[4096] = "["; // Increase the size as needed
                bool firstLine = true;

                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (strstr(line, target) == line)
                    {
                        // Parse the line values
                        int qid, mark, attempts;
                        sscanf(line, "%s %d %d %d", username, &qid, &mark, &attempts);

                        // Append comma if this is not the first line
                        if (!firstLine)
                            strcat(jsonArray, ",");
                        else
                            firstLine = false;

                        // Form the JSON line and append it to jsonArray
                        char jsonLine[200];
                        sprintf(jsonLine, "{\"username\":\"%s\", \"qid\":%d, \"mark\":%d, \"attempts\":%d}", username, qid, mark, attempts);
                        strcat(jsonArray, jsonLine);
                    }
                }
                // Append the closing bracket to jsonArray
                strcat(jsonArray, "]");

                // Send the jsonArray to client
                send(clientSocket, jsonArray, strlen(jsonArray), 0);

                fclose(file);
            }
        }
    }

    // Delay to allow the response to be sent
    usleep(1000); // 1 millisecond

    // Close the client socket
    close(clientSocket);
}

void send_file(int client_socket, const char *file_path)
{
    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        perror("Failed to open file");
        return;
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        send(client_socket, buffer, bytesRead, 0);
    }

    fclose(file);
}

void start_server()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t addrLen = sizeof(clientAddress);

    // Create the server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("Failed to create socket");
        return;
    }

    // Bind the socket to a specific IP and port
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Failed to bind");
        return;
    }

    // Start listening for incoming connections
    if (listen(serverSocket, 10) == -1)
    {
        perror("Failed to listen");
        return;
    }

    printf("Server started. Listening on port 8080...\n");
    int sockfd_c_qb = connectToServer(3002);
    int sockfd_java_qb = connectToServer(30022);
    while (1)
    {
        // Accept a new connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addrLen);
        if (clientSocket == -1)
        {
            perror("Failed to accept connection");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        // Handle the client request
        http_handle_request(clientSocket, sockfd_c_qb, sockfd_java_qb);
    }

    // Close the server socket
    close(serverSocket);
    close(sockfd_c_qb);
    close(sockfd_java_qb);
}
