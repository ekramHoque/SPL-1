#pragma once
#include "parser.h"

namespace Commands{

    enum class IndexMode{
        HASH,
        BPLUSTREE
    };

    void setIndexMode(IndexMode type);
    IndexMode getIndexMode();
    void initIndex();
    void execute(const ParsedCommand &cmd);

}