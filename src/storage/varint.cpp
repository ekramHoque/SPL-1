#include "varint.h"

namespace Varint{

    std::vector<uint8_t> encode(uint64_t value){
        std::vector<uint8_t> out;
        while(value >= 0X80){
            out.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
            value >>= 7;
        }

        out.push_back(static_cast<uint8_t>(value));
        return out;
    }

    uint64_t decode(const std::vector<uint8_t> &bufer, size_t startIndex, size_t &readByte){
        uint64_t value = 0;
        size_t shift = 0;
        readByte = 0;

        for(size_t i = startIndex; i < bufer.size(); ++i){
            uint8_t byte = bufer[i];
            value |= static_cast<uint64_t>(byte & 0x7F) << shift;
            readByte++;
            if((byte & 0x80) == 0){
                break;
            }
            shift += 7;
        }

        return value;
    }
}