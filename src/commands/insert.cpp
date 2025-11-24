#include "insert.h"
#include "file_manager.h"
#include "varint.h"
#include "hash_index.h"
#include <cstring>
#include <iostream>

static HashIndex globalHash;
//b+

static std::vector<uint8_t> encodeRecord(
    const std::vector<std::pair<std::string,std::string>> &metaInfo,const std::vector<std::string> &metaValues)
{
    std::vector<uint8_t> bufferSpace;

    for (int i = 0; i < metaInfo.size(); i++)
    {
        std::string dataType = metaInfo[i].second;
        for ( auto &c : dataType)
        {
            c = toupper(c);
        }

        std::string metaValue;
        if(i < metaValues.size()){
            metaValue = metaValues[i];
        }else{
            metaValue = "";
        }

        if(dataType == "INT"){
            bufferSpace.push_back('I');//flag for integer
            uint64_t intValue = 0;

            try{
                if(!metaValue.empty()){
                    intValue = std::stoull(metaValue);
                }
            }catch(...){}

            auto varInt = Varint::encode(intValue);
            bufferSpace.insert(bufferSpace.end(),varInt.begin(),varInt.end());
            
        }else if(dataType == "FLOAT"){
            bufferSpace.push_back('F');
            float floatValue = 0.0f;

            try{
                if(!metaValue.empty()){
                    floatValue = std::stof(metaValue);
                }
            }catch(...){}

            uint8_t temp[4];//binary for float value;
            memcpy(temp,&floatValue,4);

            bufferSpace.insert(bufferSpace.end(),temp,temp+4);

        }else if(dataType == "BOOL"){
            bufferSpace.push_back('B');
            bufferSpace.push_back((metaValue == "true" || metaValue == "1") ? 1 : 0);
        }else{
            bufferSpace.push_back('S');

            std::uint64_t strLength = metaValue.size();
            auto strLenBin = Varint::encode(strLength);
            bufferSpace.insert(bufferSpace.end(),strLenBin.begin(),strLenBin.end());

            bufferSpace.insert(bufferSpace.end(),metaValue.begin(),metaValue.end());

        }
        
    }
    return bufferSpace;
}

void insertCmdExecute(const ParsedCommand &cmd,Commands::IndexMode mode){

    std::vector<std::pair<std::string,std::string>> metaInfo;
    std::string primaryColName;

    if(!FileManager::readMeta(cmd.table,metaInfo,primaryColName)){
        std::cout << "error to read meta(table not found)\n";
        return;
    }

    if(cmd.values.size() != metaInfo.size()){
        std::cout << "GIVE TABLE VALUE " << metaInfo.size() << "\n";
    }

    //load index from disk
    globalHash.loadFromDisk(cmd.table);
    //b+

    if(!primaryColName.empty()){
        int primaryIdx = -1;

        for(size_t i =0 ; i< metaInfo.size();i++){
            if(metaInfo[i].first == primaryColName){
                primaryIdx = i;
            }
        }

        if(primaryIdx >= 0){
            std::string primaryKValue = cmd.values[primaryIdx];
            auto checkExist = globalHash.findRecord(primaryColName,primaryKValue);
            if(!checkExist.empty()){
                std::cout << "Duplicate entry for primary key\n";
                return;
            }
        }
    }

    auto buffer = encodeRecord(metaInfo,cmd.values);
    uint64_t offset = FileManager::appendRecord(cmd.table,buffer);

    //update index

    for(size_t i=0;i<metaInfo.size();i++){
        std::string colName = metaInfo[i].first;
        std::string colType = metaInfo[i].second;
        std::string colValue = cmd.values[i];

        globalHash.addRecord(colName,colValue,offset);
        //b+
        
    }

    globalHash.saveToDisk(cmd.table);
    //b+

    std::cout << "Insertes succesfully at offset " <<offset <<"\n";


}