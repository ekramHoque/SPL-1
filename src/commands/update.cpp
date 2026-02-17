#include "update.h"
#include "file_manager.h"
#include "utils.h"
#include "varint.h"
#include "hash_index.h"
#include "bplusTree_index.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <map>

using namespace std;

static HashIndex globalHashUpdate;
static BPlusTreeIndex globalBPTreeUpdate;

static vector<uint8_t> encodeRecord(
    const vector<pair<string,string>> &metaInfo,
    const vector<string> &metaValues)
{
    vector<uint8_t> bufferSpace;

    for (int i = 0; i < metaInfo.size(); i++)
    {
        string dataType = metaInfo[i].second;
        for (auto &c : dataType)
        {
            c = toupper(c);
        }

        string metaValue;
        if(i < metaValues.size()){
            metaValue = metaValues[i];
        }else{
            metaValue = "";
        }

        if(dataType == "INT"){
            bufferSpace.push_back('I');
            uint64_t intValue = 0;

            try{
                if(!metaValue.empty()){
                    intValue = stoull(metaValue);
                }
            }catch(...){}

            auto varInt = Varint::encode(intValue);
            bufferSpace.insert(bufferSpace.end(),varInt.begin(),varInt.end());
            
        }else if(dataType == "FLOAT"){
            bufferSpace.push_back('F');
            float floatValue = 0.0f;

            try{
                if(!metaValue.empty()){
                    floatValue = stof(metaValue);
                }
            }catch(...){}

            uint8_t temp[4];
            memcpy(temp,&floatValue,4);
            bufferSpace.insert(bufferSpace.end(),temp,temp+4);

        }else if(dataType == "BOOL"){
            bufferSpace.push_back('B');
            bufferSpace.push_back((metaValue == "true" || metaValue == "1") ? 1 : 0);
        }else{
            bufferSpace.push_back('S');

            uint64_t strLength = metaValue.size();
            auto strLenBin = Varint::encode(strLength);
            bufferSpace.insert(bufferSpace.end(),strLenBin.begin(),strLenBin.end());

            bufferSpace.insert(bufferSpace.end(),metaValue.begin(),metaValue.end());
        }
    }
    return bufferSpace;
}


static vector<string> decodeRecordValues(
    const vector<uint8_t> &recordData,
    const vector<pair<string,string>> &metaInfo)
{
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

static int findColumnIndex(
    const vector<pair<string,string>> &metaInfo,
    const string &colName)
{
    for(size_t i = 0; i < metaInfo.size(); i++){
        if(metaInfo[i].first == colName){
            return i;
        }
    }
    return -1;
}

/*
  Main UPDATE command executor
 
  Algorithm (Copy-on-Write + Index Synchronization):

  1. Find all records matching WHERE condition using index
  2. For each matching record:
     a. Read current record data
     b. Update specified columns
     c. Check if indexed columns changed
     d. Write updated record back to same offset (in-place update)
     e. Update indices (remove old entries, add new entries)
  3. Persist indices to disk
 
*/
void updateCmdExecute(const ParsedCommand &cmd, Commands::IndexMode mode)
{
    vector<pair<string,string>> metaInfo;
    string primaryColName;

    if(!FileManager::readMeta(cmd.table, metaInfo, primaryColName)){
        cout << "[ERROR] Table not found: " << cmd.table << "\n";
        return;
    }

    map<string, string> updateMap;
    if(cmd.columns.size() != cmd.values.size()){
        cout << "[ERROR] SET clause malformed: column count != value count\n";
        return;
    }
    
    for(size_t i = 0; i < cmd.columns.size(); i++){
        updateMap[cmd.columns[i].first] = cmd.values[i];
    }

    for(auto &upd : updateMap){
        if(findColumnIndex(metaInfo, upd.first) == -1){
            cout << "[ERROR] Column not found: " << upd.first << "\n";
            return;
        }
    }

    if(mode == Commands::IndexMode::HASH){
        globalHashUpdate.loadFromDisk(cmd.table);
    } else if(mode == Commands::IndexMode::BPLUSTREE){
        globalBPTreeUpdate.loadFromDisk(cmd.table);
    }

    vector<uint64_t> offsetsToUpdate;
    
    if(cmd.op == "="){
        cout << "[INFO] UPDATE: Finding records where " << cmd.whereColumn << " = " << cmd.whereValue1 << "\n";
        
        if(mode == Commands::IndexMode::HASH){
            offsetsToUpdate = globalHashUpdate.findRecord(cmd.whereColumn, cmd.whereValue1);
        } else if(mode == Commands::IndexMode::BPLUSTREE){
            string key = cmd.whereColumn + "##" + cmd.whereValue1;
            offsetsToUpdate = globalBPTreeUpdate.search(key);
        }
    } else if(cmd.op == "BETWEEN"){
        if(mode == Commands::IndexMode::BPLUSTREE){
            cout << "[INFO] UPDATE: Finding records where " << cmd.whereColumn 
                 << " BETWEEN " << cmd.whereValue1 << " AND " << cmd.whereValue2 << "\n";
            
            string keyLow = cmd.whereColumn + "##" + cmd.whereValue1;
            string keyHigh = cmd.whereColumn + "##" + cmd.whereValue2;
            offsetsToUpdate = globalBPTreeUpdate.rangeSearch(keyLow, keyHigh);
        } else {
            cout << "[ERROR] BETWEEN operator is only supported with B+Tree indexing\n";
            return;
        }
    } else {
        cout << "[ERROR] Unsupported operator for UPDATE: " << cmd.op << "\n";
        return;
    }

    if(offsetsToUpdate.empty()){
        cout << "[INFO] No matching records found to update.\n";
        return;
    }

    int updatedCount = 0;
    for(auto offset : offsetsToUpdate){
        auto recordData = FileManager::readRecord(cmd.table, offset);
        if(recordData.empty()){
            continue;
        }

        vector<string> currentValues = decodeRecordValues(recordData, metaInfo);
        
        vector<string> updatedValues = currentValues;
        for(auto &upd : updateMap){
            int colIdx = findColumnIndex(metaInfo, upd.first);
            if(colIdx != -1 && colIdx < (int)updatedValues.size()){
                updatedValues[colIdx] = upd.second;
            }
        }
        map<string, pair<string, string>> indexChanges; // colName -> (oldVal, newVal)
        for(size_t i = 0; i < metaInfo.size(); i++){
            if(currentValues[i] != updatedValues[i]){
                indexChanges[metaInfo[i].first] = {currentValues[i], updatedValues[i]};
            }
        }
        auto newRecordData = encodeRecord(metaInfo, updatedValues);
        
        // Try to write updated record in-place
        // If it doesn't fit, append and mark old as deleted
        bool inPlaceSuccess = FileManager::overwriteRecord(cmd.table, offset, newRecordData);
        
        if(!inPlaceSuccess){
            // Record is larger, need to append and tombstone
            // Append new record
            uint64_t newOffset = FileManager::appendRecord(cmd.table, newRecordData);
            FileManager::markDeleted(cmd.table, offset);

            for(size_t i = 0; i < metaInfo.size(); i++){
                string colName = metaInfo[i].first;
                string oldVal = currentValues[i];
                string newVal = updatedValues[i];
                
                if(mode == Commands::IndexMode::HASH){
                    globalHashUpdate.deleteRecord(colName, oldVal, offset);
                    globalHashUpdate.addRecord(colName, newVal, newOffset);
                } else if(mode == Commands::IndexMode::BPLUSTREE){
                    string oldKey = colName + "##" + oldVal;
                    globalBPTreeUpdate.deleteRecord(oldKey, offset);
                    string newKey = colName + "##" + newVal;
                    globalBPTreeUpdate.insert(newKey, newOffset);
                }
            }
        } else {
            for(auto &change : indexChanges){
                string colName = change.first;
                string oldVal = change.second.first;
                string newVal = change.second.second;

                if(mode == Commands::IndexMode::HASH){
                    globalHashUpdate.deleteRecord(colName, oldVal, offset);
                    globalHashUpdate.addRecord(colName, newVal, offset);
                } else if(mode == Commands::IndexMode::BPLUSTREE){
                    string oldKey = colName + "##" + oldVal;
                    globalBPTreeUpdate.deleteRecord(oldKey, offset);
                    string newKey = colName + "##" + newVal;
                    globalBPTreeUpdate.insert(newKey, offset);
                }
            }
        }

        updatedCount++;
    }

    if(mode == Commands::IndexMode::HASH){
        globalHashUpdate.saveToDisk(cmd.table);
    } else if(mode == Commands::IndexMode::BPLUSTREE){
        globalBPTreeUpdate.saveToDisk(cmd.table);
    }

    cout << "[SUCCESS] Updated " << updatedCount << " record(s).\n";
}
