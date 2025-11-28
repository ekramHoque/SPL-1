#include "commands.h"
#include "parser.h"

#include <iostream>
#include <algorithm>

using namespace std;

int main(){
    cout << "=============================================\n";
    cout << "              PicoDB - File Database         \n";
    cout << "=============================================\n";

    cout << "Choose indexing method:\n";
    cout << "  1. Hashing \n";
    cout << "  2. B+ Tree \n";
    cout << "Enter option (1 or 2): ";

    int choice = 0;
    cin >> choice;

    string dummy;
    getline(cin, dummy); // Clear the newline character from the input buffer

    if(choice == 1){
        Commands::setIndexMode(Commands::IndexMode::HASH);
        cout << "[INFO] Hashing index selected.\n";
    }else if(choice == 2){
        Commands::setIndexMode(Commands::IndexMode::BPLUSTREE);
        cout << "[INFO] B+ Tree index selected.\n";
    }else{
        cout << "[INFO] Invalid choice. Defaulting to Hashing index.\n";
        Commands::setIndexMode(Commands::IndexMode::HASH);  
    }

    Commands::initIndex();

    string inputLine;
    while(true){
        cout << "PicoDB> ";
        getline(cin, inputLine);
        
        if(inputLine.empty()) continue;

        string upperInput = inputLine;
        transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);

        if(upperInput == "EXIT" || upperInput == "QUIT" || upperInput == "EXIT;" || upperInput == "QUIT;"){
            cout << "Exiting PicoDB. Goodbye!\n";
            break;
        }

        auto cmd = Parser::parse(inputLine);

        if(cmd.isValid == false){
            cout << "Error: " << cmd.error << "\n";
            continue;
        }

        Commands::execute(cmd);
    }

    cout << "=============================================\n";
    cout << "              Thank you for using PicoDB     \n";
    cout << "=============================================\n";
    return 0;

}