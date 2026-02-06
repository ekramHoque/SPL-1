#pragma once
#include<vector>
#include<cstdint>
#include<string>

class FileManager{

    public:

        //appending record and return offset for indexing
        static uint64_t appendRecord(const std::string &table, std::vector<uint8_t> &records);

        //Reading record form table
        static std::vector<uint8_t> readRecord(const std::string &table, uint64_t offset);

        //Mark record as deleted (tombstone approach)
        static void markDeleted(const std::string &table, uint64_t offset);

        //for write meta
        static void writeMeta(const std::string &table, std::vector<std::pair<std::string,std::string>> &cols, const std::string &primaryCol = ""); 

        //for read meta
        static bool readMeta(const std::string &table, std::vector<std::pair<std::string,std::string>> &cols, std::string &primaryCol);

};