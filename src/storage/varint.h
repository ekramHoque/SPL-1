#pragma once

#include<vector>
#include<cstdint>
#include<cstddef>

namespace Varint{
    std::vector<uint8_t> encode(uint64_t value);  //Encode 64 bit to smaller 8 bit vector array

    //Decode 8 bit vector to original value
    uint64_t decode(const std::vector<uint8_t> &bufer, size_t startIndex, size_t &readByte);
}