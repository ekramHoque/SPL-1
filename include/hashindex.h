#pragma once
#include<unordered_map>
#include<string>

namespace tinyDB{

    class HashIndex{
    public:

        std::unordered_map<std::string,size_t> indexTable;

        void addEntry(const std::string &key,size_t &offset);
        size_t getPosition(const std::string &key);
        void saveToFile(const std::string &fileName);
        void loadFromFile(const std::string &fileName);
    };
}