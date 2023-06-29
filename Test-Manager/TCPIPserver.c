#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_PAYLOAD_SIZE 1024

// Function to send a message
void sendMessage(int socket, const char* messageType, const char* payload) {
    char message[MAX_PAYLOAD_SIZE + 100];
    sprintf(message, "%s:%zu:%s", messageType, strlen(payload), payload);
    send(socket, message, strlen(message), 0);
}

int main() {
    // Create socket
    int socket_desc, new_socket, valread;
    struct sockaddr_in server;
    int addrlen = sizeof(server);

    socket_desc = socket

(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("socket creation failed");
        return -1;
    }

    // Server details
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    // Bind the socket to a specific address and port
    if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return -1;
    }

    // Listen for incoming connections
    if (listen(socket_desc, 3) < 0) {
        perror("listen failed");
        return -1;
    }

    // Accept incoming connections
    new_socket = accept(socket_desc, (struct sockaddr*)&server, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("accept failed");
        return -1;
    }

    // Receive and process messages
    char client_message[MAX_PAYLOAD_SIZE];
    char* token;
    char* message_type;
    char* payload;
    while (1) {
        memset(client_message, 0, sizeof(client_message));
        valread = recv(new_socket, client_message, sizeof(client_message), 0);
        if (valread == 0) {
            // Connection closed by the client
            break;
        }
        printf("Received message from client: %s\n", client_message);

        // Parse the message
        token = strtok(client_message, ":");
        message_type = token;
        token = strtok(NULL, ":");
        size_t payload_length = atoi(token);
        token = strtok(NULL, ":");
        payload = token;

        // Process the message based on the message type
        if (strcmp(message_type, "QUERY") == 0) {
            // Handle QUERY message
            printf("Processing QUERY message...\n");
            // ... process the query and generate a response

            // Send RESPONSE message
            sendMessage(new_socket, "RESPONSE", "Hello Client!");
        }
        else if (strcmp(message_type, "FILE_REQUEST") == 0) {
            // Handle FILE_REQUEST message
            printf("Processing FILE_REQUEST message...\n");
            // ... validate the request and read the requested file

            // Send FILE_RESPONSE message
            FILE* file = fopen("filename.txt", "r");
            if (file == NULL) {
                perror("File open failed");
                break;
            }
            char file_buffer[MAX_PAYLOAD_SIZE];
            int bytes_read;
            while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
                send(new_socket, file_buffer, bytes_read, 0);
            }
            fclose(file);
        }
    }

    // Close the connection
    close(new_socket);
    close(socket_desc);

    return 0;
}
