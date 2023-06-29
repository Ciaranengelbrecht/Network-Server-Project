#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_PAYLOAD_SIZE 1024

// Function to send a message
void sendMessage(int socket, const char* messageType, const char* payload) {
    char message[MAX_PAYLOAD_SIZE + 100];
    sprintf(message, "%s:%zu:%s", messageType, strlen(payload), payload);
    send(socket, message, strlen(message), 0);
}

int main(int argc, char const* argv[]) {
    // Create socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("socket creation failed");
        return -1;
    }

    // Server details
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);

    // Connect to server
    if (connect(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connection failed");
        return -1;
    }

    // Send a QUERY message
    sendMessage(socket_desc, "QUERY", "Hello Server!");

    // Receive and process RESPONSE
    char server_response[MAX_PAYLOAD_SIZE];
    memset(server_response, 0, sizeof(server_response));
    recv(socket_desc, server_response, sizeof(server_response), 0);
    printf("Server response: %s\n", server_response);

    // Send a FILE_REQUEST message
    sendMessage(socket_desc, "FILE_REQUEST", "filename.txt");

    // Receive and save the file
    FILE* file = fopen("received_file.txt", "w");
    char file_buffer[MAX_PAYLOAD_SIZE];
    memset(file_buffer, 0, sizeof(file_buffer));
    int bytes_received;
    while ((bytes_received = recv(socket_desc, file_buffer, sizeof(file_buffer), 0)) > 0) {
        fwrite(file_buffer, 1, bytes_received, file);
        memset(file_buffer, 0, sizeof(file_buffer));
    }
    fclose(file);

    // Close the socket
    close(socket_desc);

    return 0;
}
