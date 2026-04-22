#pragma once
#include <string>
#include <memory>
#include "server_socket.h"
#include "message_protocol.h"
#include "lock_manager.h"
#include "parser.h"

class ClientHandler {
private:
    int clientFd;
    std::string clientId;             
    bool connected;
    
    ServerSocket* serverSocketPtr;   
    LockManager* lockManager;
    Parser* parserPtr;                
    
public:
    ClientHandler(int clientFd, 
                  const std::string& clientId,
                  ServerSocket* serverSocket,
                  LockManager* lockManager,
                  Parser* parser);
    
    ~ClientHandler();

    void handleClientCommunication();
    std::string getClientId() const { return clientId; }
    
private:
    Message processSQLCommand(const std::string& sqlCommand);
    Message executeSelectQuery(const ParsedCommand& parsedCmd);
    Message executeWriteCommand(const ParsedCommand& parsedCmd);
    std::string getCommandType(const std::string& sql);
    void sendWelcomeMessage();
};

