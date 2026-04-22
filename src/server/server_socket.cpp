#include "server_socket.h"
#include <iostream>
#include <cstring>
#include <errno.h>

ServerSocket::ServerSocket(int port) {
    this->port = port;
    serverFd = -1;
    running = false;
    
    memset(&address, 0, sizeof(address));
}

ServerSocket::~ServerSocket() {
    shutdownServer();
}

bool ServerSocket::initializeServer() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (serverFd < 0) {
        std::cerr << "ERROR: Failed to create socket! " << strerror(errno) << std::endl;
        return false;
    }
    
    int optionValue = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR,
                   &optionValue, sizeof(optionValue)) < 0) {
        std::cerr << "WARNING: Could not set SO_REUSEADDR option" << std::endl;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(serverFd, (struct sockaddr*)&address,
             sizeof(address)) < 0) {
        std::cerr << "ERROR: Failed to bind to port " << port << "! "
                  << strerror(errno) << std::endl;
        close(serverFd);
        return false;
    }
    
    if (listen(serverFd, 10) < 0) {
        std::cerr << "ERROR: Failed to listen on socket! " << strerror(errno) << std::endl;
        close(serverFd);
        return false;
    }
    
    running = true;
    
    return true;
}

int ServerSocket::acceptClientConnection() {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    
    int clientFd = accept(serverFd,
                          (struct sockaddr*)&clientAddress,
                          &clientAddressLength);
    
    if (clientFd < 0) {
        if (running) {
            std::cerr << "ERROR: Failed to accept client connection! " << strerror(errno) << std::endl;
        }
        return -1;
    }

    return clientFd;
}

void ServerSocket::closeClientConnection(int clientFd) {
    if (clientFd >= 0) {
        close(clientFd);
    }
}

bool ServerSocket::sendDataToClient(int clientFd, const std::string& data) {
    if (clientFd < 0) {
        return false;
    }
    
    ssize_t byteSent = send(clientFd, data.c_str(), data.length(), 0);
    
    if (byteSent < 0) {
        std::cerr << "ERROR: Failed to send data to client (fd: " << clientFd << ")" << std::endl;
        return false;
    }

    return true;
}

std::string ServerSocket::receiveDataFromClient(int clientFd) {
    if (clientFd < 0) {
        return "";
    }
    
    const int BUFFER_SIZE = 4096;
    char receiveBuffer[BUFFER_SIZE];
    memset(receiveBuffer, 0, BUFFER_SIZE);
    
    ssize_t bytesReceived = recv(clientFd, receiveBuffer, BUFFER_SIZE - 1, 0);
    
    if (bytesReceived < 0) {
        std::cerr << "ERROR: Failed to receive data from client (fd: " << clientFd << ")" << std::endl;
        return "";
    }
    
    if (bytesReceived == 0) {
        return "";
    }
    
    receiveBuffer[bytesReceived] = '\0';
    
    return std::string(receiveBuffer);
}

void ServerSocket::shutdownServer() {
    if (running) {
        running = false;
        
        if (serverFd >= 0) {
            close(serverFd);
            serverFd = -1;
        }
    }
}
