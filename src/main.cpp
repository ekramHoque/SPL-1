#include "parser.h"
#include "table.h"
#include <iostream>

using namespace tinyDB;

int main() {
    std::cout << "=== TinyDB ===\n";

    while (true) {
        std::cout << "\nTinyDB> ";
        std::string input;
        std::getline(std::cin, input);
        Command cmd = Parser::parse(input);

        Table table(cmd.table);

        switch (cmd.type) {
            case CommandType::CREATE:
                table.createTable(cmd.fields);
                break;
            case CommandType::INSERT:
                table.insertData(cmd.record);
                break;
            case CommandType::EXIT:
                std::cout << "Bye!\n";
                return 0;
            default:
                std::cout << " Unknown or invalid command.\n";
        }
    }
}
