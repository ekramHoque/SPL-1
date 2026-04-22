#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class ServerSocket {
private:
    int serverFd;
    int port;
    struct sockaddr_in address;
    bool running;
    
public:
    ServerSocket(int port = 8080);
    ~ServerSocket();
    
    bool initializeServer();
    int acceptClientConnection();
    void closeClientConnection(int clientFd);
    bool sendDataToClient(int clientFd, const std::string& data);
    std::string receiveDataFromClient(int clientFd);
    int getPort() const { 
        return port;
    }
    bool isRunning() const { 
        return running;
    }
    void shutdownServer();
};