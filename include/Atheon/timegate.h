#pragma once
#include <unordered_map>
#include <string>

namespace atheon {
    void timegate(std::string commit_hash, std::string location);
    void load_commited(std::string &buffer, std::string &location, std::unordered_map<std::string,std::string> &commited);
}