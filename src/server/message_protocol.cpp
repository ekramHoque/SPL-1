#include "message_protocol.h"
#include <sstream>
#include <iostream>
#include <iomanip>


Message Message::createQueryMessage(const std::string& sqlQuery) {
    Message msg;
    msg.type = MSG_QUERY;
    msg.text = sqlQuery;
    return msg;
}

Message Message::createSuccessMessage(const std::string& successText, int timeMs) {
    Message msg;
    msg.type = MSG_RESPONSE_OK;
    msg.text = successText;
    msg.timeMs = timeMs;
    return msg;
}

Message Message::createErrorMessage(const std::string& error_text) {
    Message msg;
    msg.type = MSG_RESPONSE_ERROR;
    msg.text = error_text;
    return msg;
}

Message Message::createDataMessage(const std::vector<std::string>& rows, int timeMs) {
    Message msg;
    msg.type = MSG_RESPONSE_DATA;
    msg.rows = rows;
    msg.timeMs = timeMs;
    return msg;
}

Message Message::createPingMessage() {
    Message msg;
    msg.type = MSG_PING;
    msg.text = "PING";
    return msg;
}


std::string MessageProtocol::serializeMessage(const Message& msg) {
    std::ostringstream outputStream;
    
    //Format: TYPE|COMMAND|TIME|ROWS
    outputStream << messageTypeToString(msg.type) << "|";
    outputStream << msg.text << "|";
    outputStream << msg.timeMs << "|";
    
    if (!msg.rows.empty()) {
        outputStream << joinStrings(msg.rows, '\n');
    }
    
    return outputStream.str();
}

Message MessageProtocol::deserializeMessage(const std::string& rawData) {
    Message msg;
    
    if (rawData.empty()) {
        msg.type = MSG_UNKNOWN;
        return msg;
    }
    
    std::vector<std::string> parts;
    size_t start = 0;
    size_t pos = 0;
    int pipe_count = 0;
    
    while (pos < rawData.length() && pipe_count < 3) {
        if (rawData[pos] == '|') {
            parts.push_back(rawData.substr(start, pos - start));
            start = pos + 1;
            pipe_count++;
        }
        pos++;
    }
    
    if (start < rawData.length()) {
        parts.push_back(rawData.substr(start));
    }
    
    if (parts.size() < 3) {
        std::cerr << "WARNING: Invalid message format (not enough parts)" << std::endl;
        msg.type = MSG_UNKNOWN;
        return msg;
    }
    
    msg.type = stringToMessageType(parts[0]);

    msg.text = parts[1];
    
    try {
        msg.timeMs = std::stoi(parts[2]);
    } catch (...) {
        msg.timeMs = 0;
    }
    
    if (parts.size() > 3 && !parts[3].empty()) {
        msg.rows = splitString(parts[3], '\n');
    }
    
    return msg;
}

std::string MessageProtocol::formatResultsAsTable(const std::vector<std::string>& rows) {
    if (rows.empty()) {
        return "No results";
    }
    
    std::ostringstream table;
    table << "\n";
    table << "+----------------------------------------+\n";
    
    for (size_t i = 0; i < rows.size(); i++) {
        table << "| " << std::left << std::setw(38) << rows[i] << " |\n";
    }
    
    table << "+----------------------------------------+\n";
    table << rows.size() << " row(s) returned\n";
    
    return table.str();
}


std::string MessageProtocol::messageTypeToString(MessageType type) {
    switch (type) {
        case MSG_QUERY:          return "QUERY";
        case MSG_RESPONSE_OK:    return "OK";
        case MSG_RESPONSE_ERROR: return "ERROR";
        case MSG_RESPONSE_DATA:  return "DATA";
        case MSG_PING:           return "PING";
        case MSG_DISCONNECT:     return "DISCONNECT";
        default:                 return "UNKNOWN";
    }
}

MessageType MessageProtocol::stringToMessageType(const std::string& typeStr) {
    if (typeStr == "QUERY")      return MSG_QUERY;
    if (typeStr == "OK")         return MSG_RESPONSE_OK;
    if (typeStr == "ERROR")      return MSG_RESPONSE_ERROR;
    if (typeStr == "DATA")       return MSG_RESPONSE_DATA;
    if (typeStr == "PING")       return MSG_PING;
    if (typeStr == "DISCONNECT") return MSG_DISCONNECT;
    return MSG_UNKNOWN;
}

std::vector<std::string> MessageProtocol::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    
    return result;
}

std::string MessageProtocol::joinStrings(const std::vector<std::string>& strings, char delimiter) {
    if (strings.empty()) {
        return "";
    }
    
    std::ostringstream result;
    for (size_t i = 0; i < strings.size(); i++) {
        result << strings[i];
        if (i < strings.size() - 1) {
            result << delimiter;
        }
    }
    
    return result.str();
}
