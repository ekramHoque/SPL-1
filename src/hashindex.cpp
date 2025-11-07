#include "include/hashindex.h"
#include<fstream>

namespace tinyDB{

    void HashIndex::addEntry(const std::string &key,size_t &offset){
        indexTable[key] = offset;
    }

    size_t HashIndex::getPosition(const std::string &key){
        return indexTable.count(key) ? indexTable[key] : (size_t)-1;
    }

    void HashIndex::saveToFile(const std::string &fileName){
        std::ofstream outFile(fileName);
        for(auto &[key,offset] : indexTable){
            outFile << key <<" " << offset << "\n";
        }

    }

    void HashIndex::loadFromFile(const std::string &fileName){
        std::ifstream inFile(fileName);
        std:: string key;
        size_t offset;

        while(inFile >> key >> offset){
            indexTable[key] = offset;
        }
    }
}