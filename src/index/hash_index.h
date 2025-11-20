#pragma once
#include<string>
#include<vector>
#include<cstdint>
#include<unordered_map>

class HashIndex{
    public:
        //for multi level indexing like " (column -> (value -> vector<offset>)) "
        std::unordered_map<std::string,std::unordered_map<std::string,std::vector<uint64_t>>> idx;

        void addRecord(const std::string &col, const std::string &value, uint64_t offset);
        std::vector<uint64_t> findRecord(const std::string &col, const std::string &value);
        void saveToDisk(const std::string &table);
        void loadFromDisk(const std::string &table);
};