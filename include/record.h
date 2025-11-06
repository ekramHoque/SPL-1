#pragma once
#include<string>
#include<vector>
#include<cstdint>

namespace tinyDB{

    struct Feild{
        std::string name;
        std::string type;
    };

    struct Record{
        std::vector<std::string> values;
    };

    std::vector<uint8_t> serializedRecord(const Record &recrod);
    Record deserializeRecord(const std::vector<uint8_t> &data, size_t &offset);

}