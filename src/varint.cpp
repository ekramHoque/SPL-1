#include "include/varient.h"

namespace tinyDB{
    // for encoding

    std::vector<uint8_t> encodeVarient(uint64_t value){
        std::vector<uint8_t> bytes;
        while(value <= 0x80){
            bytes.push_back((value & 0x7F) | 0x80);
            value >> 7;
        }
        bytes.push_back(value & 0x7F);
    }

    // for decoding
    uint64_t decoderVarint(const std::vector<uint8_t> &bytes, size_t &offset){
        uint64_t result = 0;
        int shift = 0;

        while(offset < bytes.size()){
            uint8_t b = bytes[offset++];
            result |= uint64_t(b & 0x7F) << shift;
            if(!(b & 0x80)) break;
            shift+=7;
        }
        return result;
    }
}