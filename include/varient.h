#pragma once
#include<vector>
#include<cstdint>

namespace tinyDB{
    std::vector<uint8_t> encodeVarint(uint64_t value);
    uint64_t decoderVarint(const std::vector<uint8_t> &bytes,size_t &offset);
    
}