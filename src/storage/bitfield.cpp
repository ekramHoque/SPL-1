#include "bitfield.h"

namespace Bitfield{

    uint8_t pack(std::vector<bool> &flags){
        uint8_t b = 0;

        for(size_t i= 0; i < flags.size() && i < 8; ++i ){
            if(flags[i]) b |= (1u << i);  // 1u signed integer 1
        }


        return b;
    }

    std::vector<bool> unpack(uint8_t b, size_t n){
        
        std::vector<bool> out;
        out.reserve(n);

        for(size_t i = 0; i < n && i < 8; ++i){
            uint8_t shifted = b >> i;
            uint8_t bit = shifted & 1;
            bool value = (bit != 0);
            out.push_back(value);
        }

        return out;

    }
}