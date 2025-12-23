#include "file_manager.h"
#include"varint.h"
#include<fstream>
#include<filesystem>
#include<iostream>
#include<sstream>

using namespace std;
namespace fs = std::filesystem;

uint64_t FileManager::appendRecord(const string &table,vector<uint8_t> &records){

    fs::create_directories("data/" + table);
    string filePath = "data/" + table +'/' + table +".data";

    uint64_t offset = 0; //if file emplty
    if(fs::exists(filePath)) offset = fs::file_size(filePath);

    //open file binary and append mode
    ofstream out(filePath,ios::binary | ios::app);
    if(!out){

        //cerr for output error, it almost same like cout
        cerr << "ERROR opening data fle" << endl;
        return 0;
    }

    auto compressInt = Varint::encode(records.size()); 

    //write length first uing varint
    out.write(reinterpret_cast<const char*>(compressInt.data()), compressInt.size());

    if(!(records.empty())){
        //write data to the file
        out.write(reinterpret_cast<const char*> (records.data()),records.size());
    }

    out.close();
    return offset;

}

vector<uint8_t> FileManager::readRecord(const string &table,uint64_t offset){

    vector<uint8_t> recordData;
    string filePath = "data/" + table + '/' + table +".data";

    //check file exists
    if(!fs::exists(filePath)){
        cerr << "Data file not found" << endl;
        return recordData;
    }

    ifstream in(filePath, ios::binary);
    if(!in){
        cerr << "ERROR opening data file" << endl;
        return recordData;  
    }

    //move to offset
    in.seekg(offset);

    //read varint length
    vector<uint8_t> lengthBuffer;
    
    //max 10 bytes for varint 64 bit
    for(size_t i = 0; i < 10; ++i){ 
        
        char c;
        in.get(c);
        lengthBuffer.push_back(static_cast<uint8_t>(c));
        if((lengthBuffer.back() & 0x80) == 0){
            break;  
        }
    }

    size_t readBytes = 0;
    uint64_t recordLength = Varint::decode(lengthBuffer, 0, readBytes);
    recordData.resize(recordLength); //resize vector to hold record data
    if(recordLength > 0){
        in.read(reinterpret_cast<char*>(recordData.data()), recordLength);
    }
    in.close();
    return recordData;
    

}

void FileManager::writeMeta(const string &table, vector<pair<string,string>> &cols, const string &primaryCol){

    fs::create_directories("data/" + table);
    string filePath = "data/" + table +"/" + table + ".meta";

    ofstream columnRecord(filePath);
    if(!columnRecord){
        cerr << "Error to write meta" << "\n";
        return;
    }

    columnRecord << "COLUMN: " << cols.size() << "\n";

    //write meta
    for(auto &c : cols){
        columnRecord << c.first << " " << c.second;

        //for primary key
        if(!primaryCol.empty() && c.first == primaryCol){
            columnRecord << " PRIMARY";
        }

        columnRecord << "\n";
    }
}

bool FileManager::readMeta(const string &table, vector <pair<string,string>> &cols,string &primaryCol){
    //clear those
    cols.clear();
    primaryCol ="";

    string filePath = "data/" + table +"/" + table + ".meta";
    if(!fs::exists(filePath)) return false;

    ifstream columnRecord(filePath);
    if(!columnRecord){
        cerr << "Error to Read meta" << "\n";
        return false;
    }

    string line;

    //read first line for column count
    getline(columnRecord,line);

    while(getline(columnRecord,line)){

        if(line.empty()) continue;

        istringstream iss (line);
        string colName, colType, primaryK;
        iss >> colName >> colType >> primaryK;

        cols.push_back({colName,colType});

        if(primaryK == "PRIMARY"){
            primaryCol = colName;
        }
    }

    return true;

}

