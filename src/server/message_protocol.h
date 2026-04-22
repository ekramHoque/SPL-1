#pragma once
#include <string>
#include <vector>
#include <map>

enum MessageType {
    MSG_QUERY,           
    MSG_RESPONSE_OK,     
    MSG_RESPONSE_ERROR,  
    MSG_RESPONSE_DATA,   
    MSG_PING,            
    MSG_DISCONNECT,      
    MSG_UNKNOWN         
};

class Message {
public:
    MessageType type;
    std::string text;
    std::vector<std::string> rows;
    int timeMs;
    
    Message() : type(MSG_UNKNOWN), timeMs(0) {}
    
    static Message createQueryMessage(const std::string& sqlQuery);
    static Message createSuccessMessage(const std::string& successText, int time_ms = 0);
    static Message createErrorMessage(const std::string& errorText);
    static Message createDataMessage(const std::vector<std::string>& rows, int time_ms = 0);
    static Message createPingMessage();
};

class MessageProtocol {
public:
    static std::string serializeMessage(const Message& msg);
    static Message deserializeMessage(const std::string& rawData);
    static std::string formatResultsAsTable(const std::vector<std::string>& rows);
    
private:
    static std::string messageTypeToString(MessageType type);
    static MessageType stringToMessageType(const std::string& typeStr);
    static std::vector<std::string> splitString(const std::string& str, char delimiter);
    static std::string joinStrings(const std::vector<std::string>& strings, char delimiter);
};


