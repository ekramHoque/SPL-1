#pragma once
#include<vector>
#include "record.h"

namespace tinyDB{
    class storage{
        public:
        static void writeBinary(const std::string &fileName,std::vector<uint8_t> &data);
        static std::vector<uint8_t> readBinary(const std::string &fileName);
    };
}