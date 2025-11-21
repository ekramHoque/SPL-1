#pragma once
#include <string>
#include <vector>
#include<utility>

struct ParsedCommand{

    bool isValid = true;
    std::string type;
    std::string table;

    //for create table (column name, column type)
    std::vector< std::pair<std::string,std::string> > columns; 
    //for insert record
    std::vector<std::string> values;
    std::string whereColumn;
    std::string whereValue1;
    std::string whereValue2;//for between condition(later implement) 
    std::string op; // =, <, >, between(later implement)
    std::string error;
};

class Parser{
    public:
        ParsedCommand parse(const std::string &input);
};