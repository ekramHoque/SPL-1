#include "show.h"
#include "file_manager.h"
#include "utils.h"
#include "varint.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>

using namespace std;

void showCmdExecute(const ParsedCommand &cmd){

    std::vector<std::pair<std::string,std::string>> metaInfo;
    std::string primaryColName;

    if(!FileManager::readMeta(cmd.table,metaInfo,primaryColName)){
        std::cout << "error to read meta(table not found)\n";
        return;
    }

    string filePath = "data/" + cmd.table + "/" + cmd.table + ".data";

    if(!filesystem::exists(filePath)){
        std::cout << "[INFO] No records.\n";
        return;
    }

    cout << "-------------------------------------------------\n";
    for (auto &c: metaInfo){
         cout << "| " << c.first << " ";
    }   
    cout << "\n-------------------------------------------------\n";

    ifstream tableData(filePath, ios::binary);

    while (tableData.peek() != EOF) {
        // read varint length into vector  
        vector<uint8_t> varIntFormatLen;
        for (int i=0;i<10;i++) {
            char ch;
            tableData.get(ch);
            if (!tableData) break;
            varIntFormatLen.push_back(static_cast<uint8_t>(ch));
            if (!(varIntFormatLen.back() & 0x80)) break;
        }

        if (!tableData) break;
        size_t readBytes = 0;
        uint64_t recordLength = Varint::decode(varIntFormatLen, 0, readBytes);

        /* 
        Tombstone format: [0x00][skip_bytes_varint][remaining_old_data]
        If length is 0, read the skip varint and jump. If length > 0, read the record as normal.
        */

        if (recordLength == 0) {
            vector<uint8_t> skipVarint;
            for (int i=0; i<10; i++) {
                char ch;
                tableData.get(ch);
                if (!tableData) break;
                skipVarint.push_back(static_cast<uint8_t>(ch));
                if (!(skipVarint.back() & 0x80)) break;
            }
            
            if (!skipVarint.empty()) {
                size_t skipReadBytes = 0;
                uint64_t bytesToSkip = Varint::decode(skipVarint, 0, skipReadBytes);
                tableData.seekg(bytesToSkip, ios::cur);
            }
            continue;
        }

        vector<uint8_t> recordData(recordLength);
        if (recordLength>0) {
            tableData.read(reinterpret_cast<char*>(recordData.data()), recordLength);
        }

        size_t pos = 0;
        for (auto &col : metaInfo) {
            if (pos >= recordData.size()) { 
                cout << "| ? "; 
                continue; 
            }
            char flagType = recordData[pos++];
            if (flagType == 'I') {
                size_t r=0;
                uint64_t intValue = Varint::decode(recordData, pos, r);
                pos += r;
                cout << "| " << intValue << " ";
            } else if (flagType == 'F') {
                float floatValue=0.0f;
                if (pos + 4 <= recordData.size()) {
                    memcpy(&floatValue, recordData.data()+pos, 4);
                    pos += 4;   
                }
                cout << "| " << floatValue << " ";
            } else if (flagType == 'B') {
                uint8_t boolValue = recordData[pos++]; 
                cout << "| " << (boolValue ? "true":"false") << " ";
            } else if (flagType == 'S') {
                size_t r=0;
                uint64_t strLength = Varint::decode(recordData, pos, r);
                pos += r;
                string strValue;
                if (pos + strLength <= recordData.size()) {
                    strValue.assign((char*)(recordData.data()+pos), strLength);
                }
                pos += strLength;
                cout << "| " << strValue << " ";
            } else {
                cout << "| ? ";
            }
        }
        cout << "\n";
    }
    cout << "-------------------------------------------------\n";
}
