#include "include/table.h"
#include "include/storage.h"
#include "include/config.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace tinyDB{
    Table::Table(const std::string &tableName) : tableName(tableName){}

    void Table::createTable(const std::vector<Feild> &columns){
        tableColumnName = columns;
        std::filesystem::create_directories(data_Dir);
        std::ofstream meta(data_Dir + tableName + ".meta");

        for(auto &column : tableColumnName){
            meta << column.name << " " << column.type << "\n";
        }
        std::cout << "Table created " << tableName <<"\n";
    }

    void  Table::insertData(const Record &record){

        auto encodedRecord = serializedRecord(record);

        std::string filePath = data_Dir + tableName + ".data";
        size_t offset = std::filesystem::exists(filePath) ? std::filesystem::file_size(filePath) : 0;

        storage::writeBinary(filePath,encodedRecord);

        recordIndex.addEntry(record.values[0],offset);
        recordIndex.saveToFile(data_Dir + tableName + ".idx");

    }
}