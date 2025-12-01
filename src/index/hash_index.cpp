#include "hash_index.h"
#include <filesystem>
#include<fstream>
#include<iostream>
#include "utils.h"

using namespace std;
namespace fs = std::filesystem;

void HashIndex::addRecord(const string &col,const string &value,uint64_t offset){

    string trimmedValue = trimSpaceC(value);
    string trimmedCol = trimSpaceC(col);

    idx[trimmedCol][trimmedValue].push_back(offset);

    //save like this

  /*{
  "dept" : {
      "IIT" : [0]
   },
  "name" : {
      "Ekram" : [0]
     }
   }
     */
}

vector<uint64_t> HashIndex::findRecord(const string &col, const string &value){

    //check column name and value exist or not
    string trimmedValue = trimSpaceC(value);
    string trimmedCol = trimSpaceC(col);
    
    if(idx.count(trimmedCol) && idx[trimmedCol].count(trimmedValue)){
        //if exist return offset
        return idx[trimmedCol][trimmedValue]; 
    }

    return {}; //return empty vector if not exist

}

void HashIndex::saveToDisk(const string &table){

    fs::create_directories("data/" + table);
    string filePath = "data/" + table +"/" + table + ".hashidx";

    ofstream indexRecord(filePath,ios::binary);
    if(!indexRecord){
        cerr << "ERROR to open indexRecord write\n";
        return;
    }

    //indexing part: columnName -> (columnValue -> offset list)

    for(auto &columnEntry : idx){
        //take column name like"name,dept"
        const string &columnName = columnEntry.first;

        //for second unordered map(columnValue -> offsetLIst)
        for(auto &columnValue : columnEntry.second){

            const string &value = columnValue.first;
            const vector<uint64_t> &offsetList = columnValue.second;

            indexRecord.write(columnName.c_str(),columnName.size()+1);  //write column name with terminator null
            indexRecord.write(value.c_str(),value.size()+1);

            uint64_t offestCount = offsetList.size();
            indexRecord.write(reinterpret_cast<const char*>(&offestCount),sizeof(offestCount));

            for(auto &offset: offsetList){
                indexRecord.write(reinterpret_cast<const char*>(&offset),sizeof(offset));
            }


        }
    }

    indexRecord.close();
}

void HashIndex::loadFromDisk(const string &table){

    //we can rewrite it from file, so delete old data
    idx.clear();

    string filePath = "data/" + table +"/" + table + ".hashidx";

    if(!fs::exists(filePath)){
        cerr << "No hash index file found\n";
        return;
    }

    ifstream indexRecordFile(filePath,ios::binary);
    if(!indexRecordFile){
        cerr <<"Error to open hash Index file\n";
        return;
    }

    //read file
    while(indexRecordFile.peek() != EOF){

        //for columnName
        string coulumnName;
        getline(indexRecordFile,coulumnName,'\0');  //'\0'  read null terminator

        string columnValue;
        getline(indexRecordFile,columnValue,'\0');

        uint64_t offsetCount = 0;
        indexRecordFile.read(reinterpret_cast<char*>(&offsetCount),sizeof(offsetCount));

        vector<uint64_t> offsetList(offsetCount);

        for(uint64_t i=0;i<offsetCount;i++){
            
            indexRecordFile.read(reinterpret_cast<char*>(&offsetList[i]),sizeof(uint64_t));
            
        }

        //store in idx
        idx[coulumnName][columnValue] = offsetList;


    }
    indexRecordFile.close();
}