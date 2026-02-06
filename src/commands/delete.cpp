#include "delete.h"
#include "file_manager.h"
#include "utils.h"
#include "varint.h"
#include "hash_index.h"
#include "bplusTree_index.h"
#include <iostream>
#include <cstring>

using namespace std;

static HashIndex globalHashDelete;
static BPlusTreeIndex globalBPTreeDelete;

static vector<string> decodeRecordValues(const vector<uint8_t> &recordData, const vector<pair<string,string>> &metaInfo){
    vector<string> values;
    size_t pos = 0;
    
    for (auto &col : metaInfo) {
        if (pos >= recordData.size()) { 
            values.push_back("");
            continue; 
        }
        
        char flagType = recordData[pos++];
        
        if (flagType == 'I') {
            size_t r = 0;
            uint64_t intValue = Varint::decode(recordData, pos, r);
            pos += r;
            values.push_back(to_string(intValue));
        } else if (flagType == 'F') {
            float floatValue = 0.0f;
            if (pos + 4 <= recordData.size()) {
                memcpy(&floatValue, recordData.data() + pos, 4);
                pos += 4;   
            }
            values.push_back(to_string(floatValue));
        } else if (flagType == 'B') {
            uint8_t boolValue = recordData[pos++]; 
            values.push_back(boolValue ? "true" : "false");
        } else if (flagType == 'S') {
            size_t r = 0;
            uint64_t strLength = Varint::decode(recordData, pos, r);
            pos += r;
            string strValue;
            if (pos + strLength <= recordData.size()) {
                strValue.assign((char*)(recordData.data() + pos), strLength);
            }
            pos += strLength;
            values.push_back(strValue);
        } else {
            values.push_back("");
        }
    }
    
    return values;
}

void deleteCmdExecute(const ParsedCommand &cmd, Commands::IndexMode mode){
    vector<pair<string,string>> metaInfo;
    string primaryColName;

    if(!FileManager::readMeta(cmd.table, metaInfo, primaryColName)){
        cout << "[ERROR] Table not found: " << cmd.table << "\n";
        return;
    }

    if(mode == Commands::IndexMode::HASH){
        globalHashDelete.loadFromDisk(cmd.table);
    } else if(mode == Commands::IndexMode::BPLUSTREE){
        globalBPTreeDelete.loadFromDisk(cmd.table);
    }

    //for where cluse
    vector<uint64_t> offsetsToDelete;
    
    if(cmd.op == "="){
        cout << "[INFO] Deleting records where " << cmd.whereColumn << " = " << cmd.whereValue1 << "\n";
        
        if(mode == Commands::IndexMode::HASH){
            offsetsToDelete = globalHashDelete.findRecord(cmd.whereColumn, cmd.whereValue1);
        } else if(mode == Commands::IndexMode::BPLUSTREE){
            string key = cmd.whereColumn + "##" + cmd.whereValue1;
            offsetsToDelete = globalBPTreeDelete.search(key);
        }
    } else if(cmd.op == "BETWEEN"){
        if(mode == Commands::IndexMode::BPLUSTREE){
            cout << "[INFO] Deleting records where " << cmd.whereColumn 
                 << " BETWEEN " << cmd.whereValue1 << " AND " << cmd.whereValue2 << "\n";
            
            string keyLow = cmd.whereColumn + "##" + cmd.whereValue1;
            string keyHigh = cmd.whereColumn + "##" + cmd.whereValue2;
            offsetsToDelete = globalBPTreeDelete.rangeSearch(keyLow, keyHigh);
        } else {
            cout << "[ERROR] BETWEEN operator is only supported with B+Tree indexing\n";
            return;
        }
    } else {
        cout << "[ERROR] Unsupported operator for DELETE: " << cmd.op << "\n";
        return;
    }

    if(offsetsToDelete.empty()){
        cout << "[INFO] No matching records found to delete.\n";
        return;
    }

    // For each record to delete:
    // 1. Read the record to get all column values
    // 2. Mark the record as deleted in the data file
    // 3. Remove from all index entries

    int deletedCount = 0;
    for(auto offset : offsetsToDelete){
        auto recordData = FileManager::readRecord(cmd.table, offset);
        if(recordData.empty()){
            continue;
        }

        vector<string> recordValues = decodeRecordValues(recordData, metaInfo);
        FileManager::markDeleted(cmd.table, offset);
        
        for(size_t i = 0; i < metaInfo.size() && i < recordValues.size(); i++){
            string colName = metaInfo[i].first;
            string colValue = recordValues[i];
            
            if(mode == Commands::IndexMode::HASH){
                globalHashDelete.deleteRecord(colName, colValue, offset);
            } else if(mode == Commands::IndexMode::BPLUSTREE){
                string key = colName + "##" + colValue;
                globalBPTreeDelete.deleteRecord(key, offset);
            }
        }
        
        deletedCount++;
    }

    if(mode == Commands::IndexMode::HASH){
        globalHashDelete.saveToDisk(cmd.table);
    } else if(mode == Commands::IndexMode::BPLUSTREE){
        globalBPTreeDelete.saveToDisk(cmd.table);
    }

    cout << "[SUCCESS] Deleted " << deletedCount << " record(s).\n";
}
