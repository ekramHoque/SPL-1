#include "client_handler.h"
#include "commands.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cctype>

static std::string shortQueryText(const std::string& query) {
    std::string text = query;
    for (char& ch : text) {
        if (ch == '\n' || ch == '\r' || ch == '\t') {
            ch = ' ';
        }
    }

    size_t start = text.find_first_not_of(' ');
    if (start == std::string::npos) {
        return "";
    }
    text = text.substr(start);

    if (text.size() > 60) {
        text = text.substr(0, 60) + "...";
    }
    return text;
}

ClientHandler::ClientHandler(int clientFd, 
                             const std::string& clientId,
                             ServerSocket* serverSocket,
                             LockManager* lockManager,
                             Parser* parser) {
    this->clientFd = clientFd;
    this->clientId = clientId;
    connected = true;
    serverSocketPtr = serverSocket;
    this->lockManager = lockManager;
    parserPtr = parser;
}

ClientHandler::~ClientHandler() {
    if (connected && serverSocketPtr) {
        serverSocketPtr->closeClientConnection(clientFd);
    }
}

void ClientHandler::handleClientCommunication() {
    sendWelcomeMessage();
    while (connected) {
        std::string receivedData = serverSocketPtr->receiveDataFromClient(clientFd);
        
        if (receivedData.empty()) {
            connected = false;
            break;
        }

        Message receivedMessage = MessageProtocol::deserializeMessage(receivedData);
        
        if (receivedMessage.type == MSG_DISCONNECT) {
            Message goodbyeMsg = Message::createSuccessMessage("Goodbye! Connection closed.");
            std::string serialized = MessageProtocol::serializeMessage(goodbyeMsg);
            serverSocketPtr->sendDataToClient(clientFd, serialized);
            
            connected = false;
            break;
        }
        
        if (receivedMessage.type == MSG_PING) {
            Message pong = Message::createSuccessMessage("PONG");
            std::string serialized = MessageProtocol::serializeMessage(pong);
            serverSocketPtr->sendDataToClient(clientFd, serialized);
            continue;
        }
        
        if (receivedMessage.type == MSG_QUERY) {
            std::cout << clientId << " query: " << shortQueryText(receivedMessage.text) << std::endl;
            Message response = processSQLCommand(receivedMessage.text);
            
            std::string serializedResponse = MessageProtocol::serializeMessage(response);
            serverSocketPtr->sendDataToClient(clientFd, serializedResponse);
        } else {
            Message error_msg = Message::createErrorMessage("Unknown message type");
            std::string serialized = MessageProtocol::serializeMessage(error_msg);
            serverSocketPtr->sendDataToClient(clientFd, serialized);
        }
    }

    std::cout << clientId << " disconnected" << std::endl;
}

Message ClientHandler::processSQLCommand(const std::string& sqlCommand) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        std::string comandType = getCommandType(sqlCommand);
        Message response;
        ParsedCommand parsedCmd = parserPtr->parse(sqlCommand);
        
        if (!parsedCmd.isValid) {
            if (parsedCmd.error == "comment line") {
                response = Message::createSuccessMessage("Comment ignored");
            } else {
                response = Message::createErrorMessage("Parse error: " + parsedCmd.error);
            }
        } else {
            if (comandType == "SELECT" || comandType == "SHOW") {
                response = executeSelectQuery(parsedCmd);
            } 
            else if (comandType == "INSERT" || comandType == "UPDATE" || comandType == "DELETE" || comandType == "CREATE") {
                response = executeWriteCommand(parsedCmd);
            }
            else {
                response = Message::createErrorMessage("Unknown SQL command: " + comandType);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        response.timeMs = duration.count();
        
        return response;
        
    } catch (const std::exception& e) {
        std::cerr << "[" << clientId << "] " << e.what() << std::endl;
        return Message::createErrorMessage("Error: " + std::string(e.what()));
    }
}

Message ClientHandler::executeSelectQuery(const ParsedCommand& parsedCmd) {
    LockGuard lockGuard(lockManager, clientId, LOCK_TYPE_READ);
    
    if (!lockGuard.isLocked()) {
        return Message::createErrorMessage("Failed to acquire READ lock");
    }
    
    std::stringstream captured_output;
    std::streambuf* old_cout = std::cout.rdbuf(captured_output.rdbuf());
    std::streambuf* old_cerr = std::cerr.rdbuf(captured_output.rdbuf());
    
    Commands::execute(parsedCmd);
    std::cout.rdbuf(old_cout);
    std::cerr.rdbuf(old_cerr);
    
    std::string output = captured_output.str();
    std::vector<std::string> results;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            results.push_back(line);
        }
    }
    
    if (results.empty()) {
        results.push_back("Query executed successfully (no output)");
    }
    
    return Message::createDataMessage(results);
}

Message ClientHandler::executeWriteCommand(const ParsedCommand& parsedCmd) {
    LockGuard lockGuard(lockManager, clientId, LOCK_TYPE_WRITE);
    
    if (!lockGuard.isLocked()) {
        return Message::createErrorMessage("Failed to acquire WRITE lock");
    }
    
    std::stringstream captured_output;
    std::streambuf* old_cout = std::cout.rdbuf(captured_output.rdbuf());
    
    Commands::execute(parsedCmd);
    std::cout.rdbuf(old_cout);
    std::string output = captured_output.str();
    
    if (output.empty()) {
        return Message::createSuccessMessage("Command executed successfully");
    } else {
        return Message::createSuccessMessage(output);
    }
}


std::string ClientHandler::getCommandType(const std::string& sql) {
    std::string trimmed = sql;
    size_t start = trimmed.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
        trimmed = trimmed.substr(start);
    }
    
    std::istringstream iss(trimmed);
    std::string firstWord;
    iss >> firstWord;
    
    std::transform(firstWord.begin(), firstWord.end(), firstWord.begin(), ::toupper); 
    return firstWord;
}

void ClientHandler::sendWelcomeMessage() {
    std::string indexMode = (Commands::getIndexMode() == Commands::IndexMode::HASH) 
                            ? "HASH INDEXING" 
                            : "B+ TREE INDEXING";
    
    Message welcome = Message::createSuccessMessage(
        "Welcome to PicoDB Server!\n"
        "Connected as: " + clientId + "\n"
        "Index Mode: " + indexMode + "\n"
        "Type your SQL commands and press enter."
    );
    
    std::string serialized = MessageProtocol::serializeMessage(welcome);
    serverSocketPtr->sendDataToClient(clientFd, serialized);
}

