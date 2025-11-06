#include "include/record.h"
#include "include/varint.h"

namespace tinyDB{

    std::vector<uint8_t> serializedRecord(const Record &record){
        std::vector<uint8_t> out;
        out.push_back((uint8_t)record.values.size());

        for(auto &val:record.values){
            auto bytelen = encodeVarint(val.size());
            out.insert(out.end(),bytelen.begin(),bytelen.end());
            out.insert(out.end(),val.begin(),val.end());
        }

        return out;
    }

    Record deserializeRecord(const std::vector<uint8_t> &data, size_t &offset){
        Record rec;

        if(offset >= data.size()) return rec;

        uint8_t feildCount = data[offset++];
        for(int i = 0; i< feildCount;i++){
            uint64_t len = decoderVarint(data,offset);
            std::string value(data.begin() + offset,data.begin() + offset + len);
            offset += len;
        }

        return rec;
    }
    
}