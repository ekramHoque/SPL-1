#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "message_protocol.h"


class PicoDBClient {
private:
    int socketFd;                      
    std::string serverHost;            
    int port;
    bool connected;
    struct sockaddr_in address;
    
public:
    PicoDBClient(const std::string& host, int port) {
        serverHost = host;
        this->port = port;
        socketFd = -1;
        connected = false;
        memset(&address, 0, sizeof(address));
    }
    
    ~PicoDBClient() {
        disconnect();
    }
    
    bool connect() {
        std::cout << "Connecting to " << serverHost << ":" << port << "..." << std::endl;
        
        socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFd < 0) {
            std::cerr << "ERROR: Failed to create socket" << std::endl;
            return false;
        }
        
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        
        if (inet_pton(AF_INET, serverHost.c_str(), &address.sin_addr) <= 0) {
            std::cerr << "ERROR: Invalid server address: " << serverHost << std::endl;
            close(socketFd);
            return false;
        }
        
        if (::connect(socketFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "ERROR: Connection failed! Is the server running?" << std::endl;
            close(socketFd);
            return false;
        }
        
        connected = true;
        std::cout << "Connected." << std::endl;
        
        receiveResponse();
        
        return true;
    }
    
    void disconnect() {
        if (connected) {
            Message disconnectMsg;
            disconnectMsg.type = MSG_DISCONNECT;
            disconnectMsg.text = "DISCONNECT";
            
            std::string serialized = MessageProtocol::serializeMessage(disconnectMsg);
            send(socketFd, serialized.c_str(), serialized.length(), 0);
            
            close(socketFd);
            connected = false;
            std::cout << "Disconnected." << std::endl;
        }
    }
    
    bool sendQuery(const std::string& sql) {
        if (!connected) {
            std::cerr << "ERROR: Not connected to server" << std::endl;
            return false;
        }
        
        Message queryMsg = Message::createQueryMessage(sql);
        std::string serialized = MessageProtocol::serializeMessage(queryMsg);
        
        ssize_t bytes_sent = send(socketFd, serialized.c_str(), serialized.length(), 0);
        
        if (bytes_sent < 0) {
            std::cerr << "ERROR: Failed to send query" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool receiveResponse() {
        if (!connected) {
            return false;
        }
        
        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t bytesReceived = recv(socketFd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesReceived < 0) {
            std::cerr << "ERROR: Failed to receive response" << std::endl;
            return false;
        }
        
        if (bytesReceived == 0) {
            std::cout << "Server closed connection" << std::endl;
            connected = false;
            return false;
        }
        
        buffer[bytesReceived] = '\0';
        
        Message response = MessageProtocol::deserializeMessage(std::string(buffer));
        displayResponse(response);
        
        return true;
    }
    
    bool isConnected() const {
        return connected;
    }
    
private:
    void displayResponse(const Message& response) {
        switch (response.type) {
            case MSG_RESPONSE_OK:
                std::cout << response.text << std::endl;
                if (response.timeMs > 0) {
                    std::cout << "time: " << response.timeMs << "ms" << std::endl;
                }
                break;
                
            case MSG_RESPONSE_ERROR:
                std::cout << "Error: " << response.text << std::endl;
                break;
                
            case MSG_RESPONSE_DATA:
                if (response.rows.empty()) {
                    std::cout << "No rows returned." << std::endl;
                } else {
                    for (size_t i = 0; i < response.rows.size(); i++) {
                        std::cout << response.rows[i] << std::endl;
                    }
                }

                std::cout << response.rows.size() << " row(s)" << std::endl;
                if (response.timeMs > 0) {
                    std::cout << "time: " << response.timeMs << "ms" << std::endl;
                }
                break;
                
            default:
                std::cout << response.text << std::endl;
                break;
        }
    }
};

void printClientBanner() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════╗\n";
    std::cout << "║                                            ║\n";
    std::cout << "║         PicoDB Client v1.0                 ║\n";
    std::cout << "║                                            ║\n";
    std::cout << "║   Interactive SQL client for PicoDB        ║\n";
    std::cout << "║                                            ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void printClientUsage() {
    std::cout << "Usage: ./picodb_client [server_ip] [port]" << std::endl;
    std::cout << "Example: ./picodb_client 127.0.0.1 8080" << std::endl;
    std::cout << "Type SQL and press Enter. Type 'quit' to exit." << std::endl;
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    std::string serverHost = "127.0.0.1";  
    int serverPort = 8080;                  
    
    if (argc > 1) {
        serverHost = argv[1];
    }
    
    if (argc > 2) {
        try {
            serverPort = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "ERROR: Invalid port number: " << argv[2] << std::endl;
            return 1;
        }
    }

    printClientBanner();
    printClientUsage();
    PicoDBClient client(serverHost, serverPort);
    
    if (!client.connect()) {
        std::cerr << "FATAL: Could not connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "Type SQL command. Write 'quit' to exit." << std::endl;
    
    std::string userInput;
    
    while (client.isConnected()) {
        std::cout << "picodb> ";
        
        if (!std::getline(std::cin, userInput)) {
            break;  
        }
        
        size_t start = userInput.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            continue;  
        }
        userInput = userInput.substr(start);
        
        if (userInput == "quit" || userInput == "exit" || userInput == "\\q") {
            std::cout << "Exiting..." << std::endl;
            break;
        }
        
        if (client.sendQuery(userInput)) {
            client.receiveResponse();
        }
    }
    client.disconnect();
    
    std::cout << "Goodbye!" << std::endl;
    
    return 0;
}
