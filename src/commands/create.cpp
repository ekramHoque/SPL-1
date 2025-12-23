#include "create.h"
#include "file_manager.h"
#include "utils.h"
#include <iostream>
#include <filesystem>

void createCmdExecute(const ParsedCommand &cmd){

    // Check if table already exists
    std::string metaPath = "data/" + cmd.table + "/" + cmd.table + ".meta";
    if(std::filesystem::exists(metaPath)){
        std::cout << "[ERROR] Table '" << cmd.table << "' already exists\n";
        return;
    }

    std::string primaryKey = "";
    std::vector<std::pair<std::string,std::string>> cols = cmd.columns;

    for(auto &colsInfo : cols){
        if(colsInfo.second.find("PRIMARY") != std::string::npos){//rfind return npos if not found
            primaryKey = colsInfo.first;
            colsInfo.second = colsInfo.second.substr(0,colsInfo.second.find("PRIMARY"));
            colsInfo.second = trimSpaceC(colsInfo.second);
        }

        for(auto &colsType: colsInfo.second){
            colsType = toupper(colsType);
        }
    }

    FileManager::writeMeta(cmd.table,cols,primaryKey);

    std::cout << "[OK] Table " << cmd.table << " created sucessFully\n";
    if(!primaryKey.empty()){
        std::cout << "[INFO] : Primary Key -> " << primaryKey << "\n";
    }

}