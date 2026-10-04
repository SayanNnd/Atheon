#include <vector>
#include <filesystem>
#include <fstream>
#include "../include/Atheon/ignore.h"

namespace atheon {
    //Stores the ignore rules in static memory
    static std::vector<std::string>& storage() {
        static std::vector<std::string> ignores;
        return ignores;
    }

    //Loads the lines for the .atheonignore file
    void load_ignore_rules(){
        auto& ignores = storage();
        ignores.emplace_back(".atheon/");

        std::ifstream file(".atheonignore");
        if (!file.is_open()) return;

        std::string line;
        while (getline(file, line)) {
            if (line.empty() || line[0]=='#' || (line[0]=='/' && line[1]=='/')) continue;
            if (line.back() == '\r') line.pop_back();
            ignores.push_back(line);
        }
    }

    //Checks the current file/directory for ignores
    bool should_ignore(const std::filesystem::path &filepath) {
        const auto &ignores = storage();
        const std::string curr_path = filepath.generic_string();
        for (std::string rule : ignores) {
            //Directory
            if (rule.back() == '/') {
                rule.pop_back();
                for (const auto &part : filepath) {
                    if (part.string() == rule) {
                        return true;
                    }
                }
            }
            //Extension
            else if (rule[0]=='*') {
                if (filepath.extension().string() == rule.substr(1)) {
                    return true;
                }
            }
            //File
            else {
                if (curr_path == rule || filepath.filename().string() == rule) {
                    return true;
                }
            }
        }
        return false;
    }
}