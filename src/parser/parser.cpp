#include "parser.h"
#include <sstream>
#include <algorithm>
#include<regex>

static std::string trimSpace(std::string input){

    //remove space in the last
    while(input.empty() && isspace((unsigned char)input.back())){
        input.pop_back();
    }

    //remove space at first
    while(input.empty() && isspace((unsigned char)input.front())){
        input.erase(input.begin());
    }

    return input;

}

ParsedCommand Parser::parse(const std::string &input){
    ParsedCommand cmd;
    std::string inputWithoutSpace = trimSpace(input);

    if(inputWithoutSpace.empty()){
        cmd.isValid = "false";
        cmd.error = "empty string";
        return cmd;
    }

    std::string upperCaseInput = inputWithoutSpace;
    std::transform(upperCaseInput.begin(),upperCaseInput.end(),upperCaseInput.begin(), ::toupper);

    //cmd: CREATE TABLE student(id INT,name TEXT,dept TEXT); 
    if(upperCaseInput.rfind("CREATE TABLE",0) == 0){

        cmd.type = "CREATE";
        //regular expression for CREATE TABLE student(field etc)
        std::regex re(R"(CREATE\s+TABLE\s+(\w+)\s*\((.*)\)\s*;*)", std::regex::icase);
        
        //separate table name and column info
        std::smatch target;
        if(!regex_search(inputWithoutSpace,target,re)){
            cmd.isValid = "false";
            cmd.error = "CREATE Syntax";
            return cmd;
        }

        cmd.table = trimSpace(target[1].str()); //.str() make string
        std::string columnInfo = target[2].str();

        std::stringstream ss(columnInfo);
        std::string separteColumn;

        while(getline(ss,separteColumn,',')){

            separteColumn = trimSpace(separteColumn);

            //regex for meta info (w+) -> variable name, (w+) -> type, (primary) -> for key optional(?)
            std::regex metaInfo(R"((\w+)\s+(\w+)(\s+PRIMARY)?)",std::regex::icase);

            std::smatch metaNameType;

            if(std::regex_search(separteColumn,metaNameType,metaInfo)){

                std::string columnName = trimSpace(metaNameType[1].str());
                std::string columnType = trimSpace(metaNameType[2].str());

                std::string checkPrim;
                if(metaNameType[3].matched){
                    checkPrim = "PRIMARY";
                }else{
                    checkPrim = "";
                }
                if(!checkPrim.empty()) columnType += " PRIMARY";
                cmd.columns.push_back({columnName,columnType});

            }
        }
        return cmd;
    }

    //cmd: INSERT INTO tableName VALUES(1,"Ekram","IIT")

    if(upperCaseInput.rfind("INSERT INTO",0) == 0){

        cmd.type = "INSERT";

        //regex for insert command
        std::regex regXInsert(R"(INSERT\s+INTO\s+(\w+)\s*VALUES\s*\((.*)\)\s*;*)",std::regex::icase);

        std::smatch insertInfo;
        if(!std::regex_search(inputWithoutSpace,insertInfo,regXInsert)){
            cmd.isValid = false;
            cmd.error = "INSERT syntax";
            return cmd;
        }

        cmd.table = trimSpace(insertInfo[1].str());
        std::string valueInfo = insertInfo[2].str();

        std::stringstream allValues(valueInfo);
        std::string separateValue;

        while(getline(allValues,separateValue,',')){
            separateValue = trimSpace(separateValue);

            if(separateValue.size() >=2 && separateValue.front() == '/"' && separateValue.back() == '/"'){
                separateValue = separateValue.substr(1,separateValue.size()-2);
            }
            cmd.values.push_back(separateValue);
        }
        return cmd;
    }

    //cmd: SHOW TABLE tableName; OR SHOW tableName;

    if(upperCaseInput.rfind("SHOW TABLE",0)==0 || upperCaseInput.rfind("SHOW",0)==0){

        cmd.type = "SHOW";

        std::regex showRegex(R"(SHOW\s+(TABLE\s+)?(\w+)\s*;*)",std::regex::icase);
        std::smatch showInfo;

        if(!std::regex_search(inputWithoutSpace,showInfo,showRegex)){
            cmd.isValid = false;
            cmd.error = "SHOW syntax";
            return cmd;
        }

        cmd.table = trimSpace(showInfo[2].str());
        return cmd;

    }

    //cmd: SELECT * FROM tableName WHERE column op value;
    //cmd: SELECT * FROM tableName WHERE column between value1 AND value2;(later implement)

    if(upperCaseInput.rfind("SELECT",0) == 0){
        cmd.type = "SELECT";

        //Regxe for search with between keyword
        std::regex regXBetween(R"(SELECT\s+(.*)\s+FROM\s+(\w+)\s+WHERE\s+(\w+)\s+BETWEEN\s+(\"?.+\"?)\s+AND\s+(\"?.+\"?)\s*;*)",std::regex::icase);

        //Regex for search with equal sign
        std::regex regXEqual(R"(SELECT\s+(.*)\s+FROM\s+(\w+)\s+WHERE\s+(\w+)\s*=\s*(\"?.+\"?)\s*;*)",std::regex::icase);

        std::smatch searchInfoCmd;

        //between
        if(std::regex_search(inputWithoutSpace,searchInfoCmd,regXBetween)){

            cmd.table = trimSpace(searchInfoCmd[2].str());
            cmd.whereColumn = trimSpace(searchInfoCmd[3].str());
            std::string value1 = trimSpace(searchInfoCmd[4].str());
            std::string value2 = trimSpace(searchInfoCmd[5].str());

            if(value1.size() >= 2 && value1.front() =='/"' && value1.back() == '/"'){
                value1 = value1.substr(1,value1.size()-2);
            }

            if(value2.size() >= 2 && value2.front() =='/"' && value2.back() == '/"'){
                value2 = value2.substr(1,value2.size()-2);
            }

            cmd.whereValue1 = value1;
            cmd.whereValue2 = value2;
            cmd.op = "BETWEEN";
            return cmd;
        }else if(std::regex_search(inputWithoutSpace,searchInfoCmd,regXEqual)){//equal
            
            cmd.table = trimSpace(searchInfoCmd[2].str());
            cmd.whereColumn = trimSpace(searchInfoCmd[3].str());
            std::string value1 = trimSpace(searchInfoCmd[4].str());

            if(value1.size() >= 2 && value1.front() =='/"' && value1.back() == '/"'){
                value1 = value1.substr(1,value1.size()-2);
            }

            cmd.whereValue1 = value1;
            cmd.op = "=";
            return cmd;
            
        }else{

            cmd.isValid = false;
            cmd.error = "Inavalid search syntax";
            return cmd;
        }

    }

    cmd.isValid = false;
    cmd.error = "UNKHOWN COMMAND FOUND";
    return cmd;
}