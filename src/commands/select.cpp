#include "select.h"
#include "file_manager.h"
#include "utils.h"
#include "varint.h"
#include "hash_index.h"
#include <iostream>
#include <cstring>

using namespace std;

static HashIndex globalHashSelect;
//b+

static void printSingleRecord(const vector<uint8_t> &recordData, const vector<pair<string,string>> &metaInfo){

    cout << "-------------------------------------------------\n";
    for (auto &c: metaInfo){
         cout << "| " << c.first << " ";
    }   
    cout << "\n-------------------------------------------------\n";

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

void selectCmdExecute(const ParsedCommand &cmd){
    vector<pair<string,string>> metaInfo;
    string primaryColName;

    if(!FileManager::readMeta(cmd.table,metaInfo,primaryColName)){
        cout << "error to read meta(table not found)\n";
        return;
    }
    globalHashSelect.loadFromDisk(cmd.table);
    //b+

    if(cmd.op == "="){
        cout << "[INFO] Search for " << cmd.whereColumn << "\n";
        auto offsets = globalHashSelect.findRecord(cmd.whereColumn,cmd.whereValue1);
        if(offsets.empty()){
            cout << "[INFO] 0 matching records.\n";
            return;
        }

        for(auto &off : offsets){
            auto recordData = FileManager::readRecord(cmd.table,off);
            printSingleRecord(recordData,metaInfo);
        }

    }else{
        cout << "[ERROR] Invalid SELECT operation\n";
    }

    //b+
}

