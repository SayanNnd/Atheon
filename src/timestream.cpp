#include "../include/Atheon/timestream.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <ctime>
#include <iomanip>

namespace atheon {
    void timestream() {
        std::string head_content;
        std::ifstream HEAD_FILE(".atheon/HEAD");
        if (!HEAD_FILE.is_open()) {
            std::cout << "ERROR! Could not open .atheon/HEAD...\n";
            std::exit(EXIT_FAILURE);
        }
        std::getline(HEAD_FILE, head_content);
        HEAD_FILE.close();
        if (!head_content.empty() && head_content.back() == '\r') {
            head_content.pop_back();
        }

        //Opens Parent Commit
        std::string parent;
        if (head_content.rfind("ref: ", 0) == 0) {
            std::string ref_path = ".atheon/" + head_content.substr(5);
            std::ifstream REF_FILE(ref_path);
            if (!REF_FILE.is_open()) {
                std::cout << "ERROR! Could not open branch ref: " << ref_path << "\n";
                std::exit(EXIT_FAILURE);
            }
            std::getline(REF_FILE, parent);
            REF_FILE.close();
            if (!parent.empty() && parent.back() == '\r') {
                parent.pop_back();
            }
        }
        else {
            parent = head_content;
        }

        std::vector<atheon::log> Logs;
        //Continuously traverses the commit objects and its parents
        while (true) {
            const std::string pre = parent.substr(0,2);
            const std::string suf = parent.substr(2,62);
            std::string location = ".atheon/objects/";
            location += pre;
            location += '/';
            location += suf;
            std::ifstream COMMIT_FILE(location);
            if (!COMMIT_FILE.is_open()) {
                std::cout << "ERROR! Could not open commits...\n";
                std::exit(EXIT_FAILURE);
            }

            std::string buffer;
            std::time_t time;
            std::string message;
            std::string hash = parent;
            parent.clear();

            while (std::getline(COMMIT_FILE, buffer)) {
                if (!buffer.empty() && buffer.back() == '\r') {
                    buffer.pop_back();
                }
                if (buffer.rfind("parent ", 0) == 0) {
                    parent = buffer.substr(7);
                } else if (buffer.rfind("timestamp ", 0) == 0) {
                    time = static_cast<time_t>(std::stoll(buffer.substr(10)));
                } else if (buffer.rfind("message ", 0) == 0) {
                    message = buffer.substr(8);
                }
            }
            Logs.push_back({.Hash = hash, .Message = message, .Time = time});
            if (parent.empty()) {
                break;
            }
        }

        //Prints the logs
        int index{1};
        for (const auto&[Hash, Message, Time] : Logs) {
            std::string formatted_id = std::to_string(index) + ")";

            std::cout << std::left
                      << std::setw(4)  << formatted_id
                      << std::setw(10) << Hash
                      << " | "
                      << std::setw(20) << Message
                      << " | "
                      << std::ctime(&Time);
            index++;
        }
    }
}