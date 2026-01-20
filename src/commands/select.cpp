#include "select.h"
#include "file_manager.h"
#include "utils.h"
#include "varint.h"
#include "hash_index.h"
#include "bplusTree_index.h"
#include <iostream>
#include <cstring>

using namespace std;

static HashIndex globalHashSelect;
static BPlusTreeIndex globalBPTreeSelect;

static void printSingleRecord(const vector<uint8_t> &recordData, const vector<pair<string,string>> &metaInfo){

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

void selectCmdExecute(const ParsedCommand &cmd, Commands::IndexMode mode){
    vector<pair<string,string>> metaInfo;
    string primaryColName;

    if(!FileManager::readMeta(cmd.table,metaInfo,primaryColName)){
        cout << "error to read meta(table not found)\n";
        return;
    }
    
    
    if(mode == Commands::IndexMode::HASH){
        globalHashSelect.loadFromDisk(cmd.table);
    }else if(mode == Commands::IndexMode::BPLUSTREE){
        globalBPTreeSelect.loadFromDisk(cmd.table);
    }

    if(cmd.op == "="){
        cout << "[INFO] Search for " << cmd.whereColumn << " = " << cmd.whereValue1 << "\n";
        
        vector<uint64_t> offsets;
        if(mode == Commands::IndexMode::HASH){
            offsets = globalHashSelect.findRecord(cmd.whereColumn, cmd.whereValue1);
        }else if(mode == Commands::IndexMode::BPLUSTREE){
            string key = cmd.whereColumn + "##" + cmd.whereValue1;
            offsets = globalBPTreeSelect.search(key);
        }
        
        if(offsets.empty()){
            cout << "[INFO] 0 matching records.\n";
            return;
        }

        
        cout << "-------------------------------------------------\n";
        for (auto &c: metaInfo){
             cout << "| " << c.first << " ";
        }   
        cout << "\n-------------------------------------------------\n";

        for(auto &off : offsets){
            auto recordData = FileManager::readRecord(cmd.table,off);
            printSingleRecord(recordData,metaInfo);
        }

    }else if(cmd.op == "BETWEEN"){
        cout << "[INFO] Range search for " << cmd.whereColumn << " BETWEEN " 
             << cmd.whereValue1 << " AND " << cmd.whereValue2 << "\n";
        
        vector<uint64_t> offsets;
        if(mode == Commands::IndexMode::HASH){
            cout << "[ERROR] BETWEEN not supported with Hash index. Use B+ Tree.\n";
            return;
        }else if(mode == Commands::IndexMode::BPLUSTREE){
            string keyLow = cmd.whereColumn + "##" + cmd.whereValue1;
            string keyHigh = cmd.whereColumn + "##" + cmd.whereValue2;
            offsets = globalBPTreeSelect.rangeSearch(keyLow, keyHigh);
        }
        
        if(offsets.empty()){
            cout << "[INFO] 0 matching records.\n";
            return;
        }

        cout << "-------------------------------------------------\n";
        for (auto &c: metaInfo){
             cout << "| " << c.first << " ";
        }   
        cout << "\n-------------------------------------------------\n";

        for(auto &off : offsets){
            auto recordData = FileManager::readRecord(cmd.table,off);
            printSingleRecord(recordData,metaInfo);
        }

    }else{
        cout << "[ERROR] Invalid SELECT operation\n";
    }

}

