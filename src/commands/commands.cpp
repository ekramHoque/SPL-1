// #include "commands.h"
// #include "create.h"
// #include "insert.h"
// #include "show.h"
// #include "select.h"
// #include "utils.h"


// using namespace Commands;

// //default mode
// static IndexMode globalMode = IndexMode::HASH;

// void Commands::setIndexMode(IndexMode type){
//     globalMode = type;
// }
// void Commands::initIndex(){};//later need
// void Commands::execute(const ParsedCommand &cmd){

//     if(!cmd.isValid){
//         std::cout << "Invalid Command\n";
//         return;
//     }
//     if(cmd.type == "CREATE") return createCmdExecute(cmd);
//     if(cmd.type == "INSERT") return insertCmdExecute(cmd,globalMode);
//     if(cmd.type == "CREATE") return createCmdExecute();
//     if(cmd.type == "CREATE") return createCmdExecute();

//     std::cout << "Not found this command\n";
// }