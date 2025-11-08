#pragma once
#include"hashindex.h"
#include"record.h"

namespace tinyDB{

    class Table{
    public:
        std::string tableName;
        std::vector<Feild> tableColumnName;
        HashIndex recordIndex;

        Table(const std::string &tableName);

        void createTable(const std::vector<Feild> &tableColumnName);
        void insertData(const Record &record);
        void findRecordByKey(const std::string &key);

    };
}