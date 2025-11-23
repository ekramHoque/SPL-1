#include "insert.h"
#include "file_manager.h"
#include "varint.h"
#include "hash_index.h"
#include<cstring>

static HashIndex globalHash;
//b+

static std::vector<uint8_t> encodeRecord(
    const std::vector<std::pair<std::string,std::string>> &metaInfo,std::vector<std::string> &metaValues)
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

}