#include <iostream>
#include <filesystem>
#include <fstream>
#include "../include/Atheon/vault.h"
#include "../include/Atheon/conflux.h"
#include "../include/Atheon/anchor.h"
#include "../include/Atheon/timestream.h"
#include "../include/Atheon/oracle.h"
#include "../include/Atheon/timegate.h"

static void checkout_helper(const std::string& commit_hash);
static void print_help();

//----------------------------------------------------------------------------------------------------------------------

int main(const int argc ,const char **argv) {
    //CLI Interface
    if (argc > 1) {
        std::vector<std::string> args;
        for (int i{1}; i<argc; i++) {
            args.emplace_back(argv[i]);
        }
        if (args.size() == 1 && (args[0] == "help" || args[0] == "--help" || args[0] == "-h")) {
            print_help();
            std::cout << "\n";
            return 0;
        }
        else if (args.size() == 1 && args[0] == "vault") {
            atheon::vault();
        }
        else if (args.size() == 1 && args[0] == "timestream") {
            atheon::timestream();
        }
        else if (args.size() == 1 && args[0] == "oracle") {
            atheon::oracle();
        }
        else if (args[0] == "conflux") {
            if (args.size() == 1) {
                std::cout << "Enter an argument to conflux with..\n";
                std::exit(EXIT_FAILURE);
            }
            else if (args.size() == 2) {
                atheon::conflux(args[1]);
            }
            else {
                std::cout << "Enter a valid command..\n";
                std::exit(EXIT_FAILURE);
            }
        }
        else if (args[0] == "anchor") {
            if (args.size() == 1) {
                std::cout << "Enter an argument to anchor with..\n";
                std::exit(EXIT_FAILURE);
            }
            else if (args.size() == 2) {
                if (args[1]=="-m") {
                    std::cout << "Please Enter a message to anchor with..\n";
                    std::exit(EXIT_FAILURE);
                }
                else {
                    std::cout << "Enter a valid command..\n";
                    std::exit(EXIT_FAILURE);
                }
            }
            else if (args.size() == 3) {
                if (args[1]=="-m") {
                    atheon::anchor(args[2]);
                }
                else {
                    std::cout << "Enter a valid command..\n";
                    std::exit(EXIT_FAILURE);
                }
            }
            else {
                std::cout << "Enter a valid command..\n";
                std::exit(EXIT_FAILURE);
            }
        }
        else if (args[0] == "timegate") {
            if (args.size() == 1) {
                std::cout << "Enter an argument to timegate to..\n";
                std::exit(EXIT_FAILURE);
            }
            else if (args.size() == 2) {
                checkout_helper(args[1]);
            }
            else {
                std::cout << "Enter a valid command..\n";
                std::exit(EXIT_FAILURE);
            }
        }
    }

    //Interactive Menu
    else {
        int x;
        std::cout << "Welcome to Atheon!!\nPress 1 to Initialize the Repo\nPress 2 to Stage current file changes\nPress 3 to Commit staged files\nPress 4 to View Logs\n"
                     "Press 5 to check the status of the Repo\nPress 6 to Checkout older commits\nPress 0 to Exit\n";
        std::cin >> x;
        std::cout << "\033[1A\033[2K\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (x==1) {
            atheon::vault();
        }
        else if (x==2) {
            std::string stager;
            std::cout << "Enter the name of the file you want to stage (. for staging all files) :- ";
            getline(std::cin,stager);
            atheon::conflux(stager);
        }
        else if (x==3) {
            std::string message;
            std::cout << "Enter the message for commit :- ";
            getline(std::cin,message);
            atheon::anchor(message);
        }
        else if (x==4) {
            atheon::timestream();
        }
        else if (x==5) {
            atheon::oracle();
        }
        else if (x==6) {
            std::string commit_hash;
            std::cout << "Enter the commit hash or branch name:- ";
            std::cin >> commit_hash;
            std::cout << "\033[1A\033[2K\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            checkout_helper(commit_hash);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void checkout_helper(const std::string& commit_hash) {
    std::string pre = commit_hash.substr(0,2);
    std::string suf = commit_hash.substr(2);
    if (std::string location = ".atheon/objects/"+pre+'/'+suf; std::filesystem::exists(location)) {
        std::string buffer;
        std::ifstream commit(location);
        getline(commit,buffer);
        if (buffer.substr(0,4) != "tree") {
            std::cout << "Specified Hash is not a Commit file\n";
            std::exit(EXIT_FAILURE);
        }
        atheon::timegate(commit_hash,location);
    }
    else {
        if (std::filesystem::exists(".atheon/refs/heads/"+commit_hash)) {
            std::string buffer;
            std::ifstream HEAD(".atheon/refs/heads/"+commit_hash);
            getline(HEAD,buffer);
            pre = buffer.substr(0,2);
            suf = buffer.substr(2);
            location = ".atheon/objects/"+pre+'/'+suf;
            atheon::timegate(buffer,location);

            FILE *head_ptr = fopen(".atheon/HEAD", "wb");
            if (head_ptr==nullptr) {
                std::cout << "ERROR in updating HEAD!\n";
                std::exit(EXIT_FAILURE);
            }
            const std::string ref = "ref: refs/heads/"+commit_hash;
            fwrite(ref.data(), 1, ref.size(), head_ptr);
            fclose(head_ptr);
        }
        else {
            std::cout << "Please enter a valid branch name or commit hash\n";
            std::exit(EXIT_FAILURE);
        }
    }
}

static void print_help() {
    std::cout << R"(
Atheon - a minimal version control system
Made by Sayan Nandi

Usage:
  atheon                          Start the interactive menu
  atheon <command> [arguments]

Commands:
  vault                           Initialize a repository in the current directory
  conflux <file | .>              Stage one file, or everything with "."
  anchor -m "<message>"           Commit the staged files
  timestream                      Show commit history
  oracle                          Show staged and unstaged changes
  timegate <commit | branch>      Restore the working tree to a commit or branch
)";
}
