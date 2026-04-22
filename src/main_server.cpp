#include <iostream>
#include <thread>
#include <vector>
#include <signal.h>
#include <memory>
#include "server/server_socket.h"
#include "server/client_handler.h"
#include "server/lock_manager.h"
#include "parser/parser.h"
#include "commands/commands.h"

bool serverShouldKeepRunning = true;
ServerSocket* globalServer = nullptr;

void handleShutdownSignal(int signalNumber) {
    std::cout << "\nStopping server (signal " << signalNumber << ")" << std::endl;
    
    serverShouldKeepRunning = false;
    
    if (globalServer != nullptr) {
        globalServer->shutdownServer();
    }
}

void printServerBanner(int port) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════╗\n";
    std::cout << "║                                            ║\n";
    std::cout << "║         PicoDB Socket Server v1.0          ║\n";
    std::cout << "║        A lightweight database server       ║\n";
    std::cout << "║                                            ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Server Port: " << port << "\n";
    std::cout << "Press Ctrl+C to shutdown\n";
    std::cout << "\n";
}

void printServerUsage() {
    std::cout << "Usage: ./picodb_server [port]" << std::endl;
    std::cout << "Example: ./picodb_server 8080" << std::endl;
    std::cout << "If no port is given, default port 8080 is used." << std::endl;
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    int serverPort = 8080;  
    
    if (argc > 1) {
        try {
            serverPort = std::stoi(argv[1]);
            if (serverPort < 1024 || serverPort > 65535) {
                std::cerr << "ERROR: Port must be between 1024 and 65535" << std::endl;
                return 1;
            }
        } catch (...) {
            std::cerr << "ERROR: Invalid port number: " << argv[1] << std::endl;
            return 1;
        }
    }

    printServerBanner(serverPort);
    printServerUsage();
    std::cout << "Choose index mode:" << std::endl;
    std::cout << "1. Hash" << std::endl;
    std::cout << "2. B+Tree" << std::endl;
    std::cout << "Enter option (1 or 2): ";
    
    int choice = 0;
    std::cin >> choice;
    
    std::string dummy;
    std::getline(std::cin, dummy); 
    
    if(choice == 1){
        Commands::setIndexMode(Commands::IndexMode::HASH);
        std::cout << "[INFO] Hashing index selected." << std::endl;
    }else if(choice == 2){
        Commands::setIndexMode(Commands::IndexMode::BPLUSTREE);
        std::cout << "[INFO] B+ Tree index selected." << std::endl;
    }else{
        std::cout << "[WARNING] Invalid choice. Defaulting to Hashing index." << std::endl;
        Commands::setIndexMode(Commands::IndexMode::HASH);  
    }
    
    Commands::initIndex();
    std::cout << "Index mode ready." << std::endl;
    
    signal(SIGINT, handleShutdownSignal);  
    signal(SIGTERM, handleShutdownSignal);  
    
    ServerSocket server(serverPort);
    globalServer = &server;
    
    if (!server.initializeServer()) {
        std::cerr << "FATAL ERROR: Failed to initialize server!" << std::endl;
        return 1;
    }
    
    LockManager lockManager;

    Parser sqlParser;
    // Main loop: accept one client and run one thread
    std::cout << "Server started." << std::endl;
    
    std::vector<std::thread> clientThreads;
    int clientCounter = 0;
    
    while (serverShouldKeepRunning) {
        int clientFd = server.acceptClientConnection();
        if (clientFd < 0) {
            if (serverShouldKeepRunning) {
                std::cerr << "WARNING: Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        clientCounter++;
        std::string clientId = "client-" + std::to_string(clientCounter);
        std::cout << clientId << " connected" << std::endl;
        
        clientThreads.emplace_back([clientFd, clientId, &server, &lockManager, &sqlParser]() {
            try {
                ClientHandler handler(clientFd, clientId, &server, &lockManager, &sqlParser);
                handler.handleClientCommunication();
                
            } catch (const std::exception& e) {
                std::cerr << "[" << clientId << "] Exception: " << e.what() << std::endl;
            }
        });
        
    }
    

    std::cout << "\nStopping server..." << std::endl;
    std::cout << "Waiting for " << clientThreads.size() << " client threads..." << std::endl;
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    std::cout << "All client threads finished." << std::endl;
    std::cout << "\nServer summary:" << std::endl;
    std::cout << "Total clients served: " << clientCounter << std::endl;
    lockManager.printLockStatus();
    
    std::cout << "\nServer shutdown complete." << std::endl;
    
    return 0;
}
