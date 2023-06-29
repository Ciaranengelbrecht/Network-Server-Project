#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

void start_server();
void handleLogin(int clientSocket, const char* username, const char* password);
void handleLogout(int clientSocket, const char* sessionToken);
char* getUsername(const char* sessionToken);
int isSessionValid(const char* sessionToken);

#endif // HTTP_SERVER_H
