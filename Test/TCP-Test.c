#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 22
#define BUFFER_SIZE 1024

void runServer() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    int clientAddressLength = sizeof(clientAddress);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        exit(1);
    }

    // Create the server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Failed to create socket. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    // Set up the server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket to the specified address and port
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }

    // Listen for client connections
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        printf("Listen failed. Error Code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    while (1) {
        // Accept a client connection
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed. Error Code: %d\n", WSAGetLastError());
            closesocket(serverSocket);
            WSACleanup();
            exit(1);
        }

        // Print the client's IP address
        char clientIP[INET6_ADDRSTRLEN];
        DWORD clientIPLength = sizeof(clientIP);
        if (WSAAddressToString((struct sockaddr*)&clientAddress, sizeof(clientAddress), NULL, clientIP, &clientIPLength) != 0) {
            printf("Failed to convert client address. Error Code: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            closesocket(serverSocket);
            WSACleanup();
            exit(1);
        }

        printf("New connection from: %s\n", clientIP);

        // Send the client's IP address as the response
        send(clientSocket, clientIP, strlen(clientIP), 0);

        // Close the client socket
        closesocket(clientSocket);
    }

    // Close the server socket
    closesocket(serverSocket);

    // Cleanup Winsock
    WSACleanup();
}

void runClient() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddress;
    char serverIP[INET6_ADDRSTRLEN] = "127.0.0.1"; // Replace with the server's IP address

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        exit(1);
    }

    // Create the client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Failed to create socket. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    // Set up the server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    // Convert server IP address from string to binary form
    InetPtonFunc inetPton = (InetPtonFunc)GetProcAddress(GetModuleHandle("ws2_32.dll"), "InetPtonA");
    if (inetPton == NULL) {
        printf("Failed to retrieve InetPton function. Error Code: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }

    if (inetPton(AF_INET, serverIP, &(serverAddress.sin_addr)) <= 0) {
        printf("Invalid server IP address.\n");
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }
    

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("Failed to connect to server. Error Code: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }

    // Receive the server's IP address
    char serverIPBuffer[INET6_ADDRSTRLEN];
    memset(serverIPBuffer, 0, sizeof(serverIPBuffer));
    recv(clientSocket, serverIPBuffer, sizeof(serverIPBuffer) - 1, 0);

    // Print the server's IP address
    printf("Server IP: %s\n", serverIPBuffer);

    // Close the client socket
    closesocket(clientSocket);

    // Cleanup Winsock
    WSACleanup();
}

int main() {
    runServer();
    runClient();

    return 0;
}
