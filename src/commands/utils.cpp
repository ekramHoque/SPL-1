#include "utils.h"
#include <cctype>

std::string trimSpaceC(const std::string &s){
    std::string sCopy = s;

    //remove space in the last
    while(!sCopy.empty() && isspace((unsigned char)sCopy.back())){
        sCopy.pop_back();
    }

    //remove space at first
    while(!sCopy.empty() && isspace((unsigned char)sCopy.front())){
        sCopy.erase(sCopy.begin());
    }

    return sCopy;

}