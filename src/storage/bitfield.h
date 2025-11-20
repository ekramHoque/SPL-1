#pragma once
#include <vector>
#include <cstdint>

namespace Bitfield{

    //pack to boolian
    uint8_t pack(std::vector<bool> &flags);

    //unpack bool
    std::vector<bool> unpack(uint8_t b, size_t n);
    
}