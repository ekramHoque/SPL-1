#include "include/storage.h"
#include<fstream>


namespace tinyDB{
    void storage::writeBinary(const std::string &fileName, std::vector<uint8_t> &data){
        std::ofstream out(fileName, std::ios::binary | std::ios::app);
        out.write((char*)data.data(),data.size());
    }

    std::vector<uint8_t> storage::readBinary(const std::string &fileName){
        std::ifstream in(fileName,std::ios::binary);
        return std::vector<uint8_t> ((std::istreambuf_iterator<char>(in)),{});
    }
}