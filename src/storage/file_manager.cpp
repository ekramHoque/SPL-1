#include"file_manager.h"
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

    uint64_t offset = 0;
    if(fs::exists(filePath)) offset = fs::file_size(filePath);

    ofstream out(filePath,ios::binary | ios::app);
    if(!out){
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
    
    if(recordLength == 0){
        in.close();
        return recordData;
    }
    
    recordData.resize(recordLength);
    if(recordLength > 0){
        in.read(reinterpret_cast<char*>(recordData.data()), recordLength);
    }
    in.close();
    return recordData;
    

}

void FileManager::markDeleted(const string &table, uint64_t offset){

    string filePath = "data/" + table + '/' + table +".data";
    if(!fs::exists(filePath)){
        cerr << "Data file not found" << endl;
        return;
    }

    fstream file(filePath, ios::binary | ios::in | ios::out);
    if(!file){
        cerr << "ERROR opening data file for deletion" << endl;
        return;  
    }

    file.seekg(offset);
    vector<uint8_t> lengthBuffer;
    
    for(size_t i = 0; i < 10; ++i){ 
        char c;
        file.get(c);
        lengthBuffer.push_back(static_cast<uint8_t>(c));
        if((lengthBuffer.back() & 0x80) == 0){
            break;  
        }
    }

    size_t readBytes = 0;
    uint64_t recordLength = Varint::decode(lengthBuffer, 0, readBytes);
    
    file.seekp(offset);
    uint8_t zero = 0x00;
    file.write(reinterpret_cast<const char*>(&zero), 1);

    /* 
    Calculate: after writing the skip varint, how many bytes until next record?
    Original space: readBytes + recordLength
    We've used: 1 (for 0x00)
    Remaining: readBytes - 1 + recordLength
    But skip varint itself takes space, so we need to account for that 
    */

    uint64_t remainingAfterTombstone = (readBytes - 1) + recordLength;
    auto skipVarint = Varint::encode(remainingAfterTombstone);
    uint64_t actualSkip = remainingAfterTombstone - skipVarint.size();
    skipVarint = Varint::encode(actualSkip);
    
    file.write(reinterpret_cast<const char*>(skipVarint.data()), skipVarint.size());  
    file.close();
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

