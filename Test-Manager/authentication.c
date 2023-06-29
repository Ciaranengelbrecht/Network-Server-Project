#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "http_server.h"
#include <ctype.h>
#include <time.h>

#define MAX_REQUEST_SIZE 2048
#define MAX_RESPONSE_SIZE 2048
#define TOKEN_LENGTH 32
#define SESSION_FILE "sessions_tokens.txt"

// Structure to represent a credential
typedef struct
{
    char username[100];
    char password[100];
} Credential;

// Structure to represent a session
typedef struct
{
    char username[100];
    char sessionToken[TOKEN_LENGTH + 1];
} Session;

// Structure to represent a linked list node
typedef struct Nodee
{
    Session session;
    struct Nodee *next;
} Nodee;

// Global linked list to store active sessions
Nodee *sessionList = NULL;

// Structure to represent a linked list node
typedef struct Node
{
    Credential credential;
    struct Node *next;
} Node;

#define MAX_SESSIONS 100
Session activeSessions[MAX_SESSIONS];
int numSessions = 0;

void generateToken(char *token)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charsetLength = sizeof(charset) - 1;

    srand(time(NULL));

    for (int i = 0; i < TOKEN_LENGTH; ++i)
    {
        token[i] = charset[rand() % charsetLength];
    }

    token[TOKEN_LENGTH] = '\0';
}

// Function to add a session to the linked list
void addSession(const char *username, const char *sessionToken)
{
    // Store the session information in a file
    FILE *file = fopen(SESSION_FILE, "a");
    if (file == NULL)
    {
        perror("Failed to open session file");
        return;
    }
    fprintf(file, "%s %s\n", username, sessionToken);
    fclose(file);

    // Add the session to the activeSessions array
    Session session;
    strcpy(session.username, username);
    strcpy(session.sessionToken, sessionToken);
    activeSessions[numSessions++] = session;
    // Add the session to the sessionList
    Nodee *newNode = (Nodee *)malloc(sizeof(Nodee));
    newNode->session = session;
    newNode->next = sessionList;
    sessionList = newNode;
}

// Function to remove a session from the linked list
void removeSession(const char *sessionToken)
{
    // If the session list is empty, return
    if (sessionList == NULL)
    {
        return;
    }

    // If the session to be removed is at the head of the list
    if (strcmp(sessionList->session.sessionToken, sessionToken) == 0)
    {
        printf("Removing session from head of the list\n");
        Nodee *temp = sessionList;
        sessionList = sessionList->next;
        free(temp);
        return;
    }

    // Find the session in the linked list
    Nodee *current = sessionList;
    Nodee *previous = NULL;
    while (current != NULL)
    {
        if (strcmp(current->session.sessionToken, sessionToken) == 0)
        {
            printf("Removing session from middle of the list\n");
            previous->next = current->next;
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

// Function to check if a session is valid
int isSessionValid(const char *sessionToken)
{
    // Check if the session token exists in the activeSessions array
    for (int i = 0; i < numSessions; ++i)
    {
        if (strcmp(activeSessions[i].sessionToken, sessionToken) == 0)
        {
            return 1; // Session token found and valid
        }
    }

    // If the session token is not found in the activeSessions array,
    // check if it exists in the session file
    FILE *file = fopen(SESSION_FILE, "r");
    if (file == NULL)
    {
        perror("Failed to open session file");
        return 0;
    }

    char username[100], storedSessionToken[TOKEN_LENGTH + 1];
    while (fscanf(file, "%s %s", username, storedSessionToken) == 2)
    {
        if (strcmp(storedSessionToken, sessionToken) == 0)
        {
            // Add the session to the activeSessions array
            Session session;
            strcpy(session.username, username);
            strcpy(session.sessionToken, sessionToken);
            activeSessions[numSessions++] = session;

            fclose(file);
            return 1; // Session token found and valid
        }
    }

    fclose(file);
    return 0; // Session token not found or expired
}

// Function to add a credential to the linked list
void addCredential(Node **head, const char *username, const char *password)
{
    // Create a new node
    Node *newNode = (Node *)malloc(sizeof(Node));
    strcpy(newNode->credential.username, username);
    strcpy(newNode->credential.password, password);
    newNode->next = NULL;

    // If the linked list is empty, make the new node the head
    if (*head == NULL)
    {
        *head = newNode;
        return;
    }

    // Find the last node in the linked list
    Node *current = *head;
    while (current->next != NULL)
    {
        current = current->next;
    }

    // Append the new node to the end of the linked list
    current->next = newNode;
}

// Function to check if a credential is valid
int isAuthenticated(Node *head, const char *username, const char *password)
{
    Node *current = head;
    while (current != NULL)
    {
        if (strcmp(current->credential.username, username) == 0 &&
            strcmp(current->credential.password, password) == 0)
        {
            return 1; // Valid credential found
        }
        current = current->next;
    }
    return 0; // Credential not found
}

int getSessionToken(const char *username, char *sessionToken)
{
    FILE *file = fopen(SESSION_FILE, "r");
    if (file == NULL)
    {
        perror("Failed to open session file");
        return 0;
    }

    char storedUsername[100], storedSessionToken[TOKEN_LENGTH + 1];
    while (fscanf(file, "%s %s", storedUsername, storedSessionToken) == 2)
    {
        if (strcmp(username, storedUsername) == 0)
        {
            strcpy(sessionToken, storedSessionToken);
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

char *getUsername(const char *sessionToken)
{
    FILE *file = fopen(SESSION_FILE, "r");
    if (file == NULL)
    {
        perror("Failed to open session file");
        return 0;
    }

    char storedUsername[100], storedSessionToken[TOKEN_LENGTH + 1];
    while (fscanf(file, "%s %s", storedUsername, storedSessionToken) == 2)
    {
        if (strcmp(sessionToken, storedSessionToken) == 0)
        {
            char *username = malloc(strlen(storedUsername) + 1);
            if (username == NULL)
            {
                perror("Failed to allocate memory for username");
                fclose(file);
                return NULL;
            }
            strcpy(username, storedUsername);
            fclose(file);
            return username;
        }
    }

    fclose(file);
    return NULL;
}

// login function
void handleLogin(int clientSocket, const char *username, const char *password)
{
    // Open the credentials file
    FILE *file = fopen("credentials.txt", "r");
    if (file == NULL)
    {
        perror("Failed to open credentials file");
        return;
    }

    // Read credentials from the file and create a linked list
    Node *head = NULL;
    char storedUsername[100], storedPassword[100];
    while (fscanf(file, "%s %s", storedUsername, storedPassword) == 2)
    {
        addCredential(&head, storedUsername, storedPassword);
    }

    // Check if the provided credentials are valid
    int isAuthenticatedResult = isAuthenticated(head, username, password);
    // Generate a session token
    char sessionToken[TOKEN_LENGTH + 1];
    generateToken(sessionToken);
    // Close the credentials file
    fclose(file);

    // Prepare the HTTP response
    char response[MAX_RESPONSE_SIZE];
    memset(response, 0, sizeof(response));

    // Include the session token in the response
    if (isAuthenticatedResult)
    {
        // Check if the session token exists for the user in the session file
        char storedSessionToken[TOKEN_LENGTH + 1];
        if (getSessionToken(username, storedSessionToken))
        {
            // Include the stored session token in the response
            // addSession(username, storedSessionToken);
            // Add the session to the activeSessions array
            Session session;
            strcpy(session.username, username);
            strcpy(session.sessionToken, sessionToken);
            activeSessions[numSessions++] = session;
            sprintf(response, "HTTP/1.1 302 Found\r\nSet-Cookie: session_token=%s; Path=/\r\nLocation: /question_navigation.html\r\n\r\n", storedSessionToken);
        }
        else
        {
            // Add a session for the authenticated user
            addSession(username, sessionToken);
            memset(response, 0, sizeof(response));
            sprintf(response, "HTTP/1.1 302 Found\r\nSet-Cookie: session_token=%s; Path=/\r\nLocation: /question_navigation.html\r\n\r\n", sessionToken);
        }
    }
    else
    {
        char *htmlResponse = "<html><body><h1>Login Failed!</h1></body></html>";
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %lu\r\n\r\n%s",
                strlen(htmlResponse), htmlResponse);
    }

    // Send the response to the client
    if (send(clientSocket, response, strlen(response), 0) == -1)
    {
        perror("Failed to send response");
    }

    // Close the client socket
    close(clientSocket);
}

// logout function
void handleLogout(int clientSocket, const char *sessionToken)
{
    // Remove the session from active sessions
    printf("Logging out user with session token from auth.c: %s\n", sessionToken);

    removeSession(sessionToken);

    // Prepare the HTTP response
    char response[MAX_RESPONSE_SIZE];
    memset(response, 0, sizeof(response));

    // Prepare the logout confirmation page
    char *htmlResponse = "<html><body><h1>Logout Successful!</h1></body></html>";
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %lu\r\n\r\n%s",
            strlen(htmlResponse), htmlResponse);

    // Send the response to the client
    if (send(clientSocket, response, strlen(response), 0) == -1)
    {
        perror("Failed to send response");
    }

    // Close the client socket
    close(clientSocket);
}

int main()
{
    start_server();

    return 0;
}
