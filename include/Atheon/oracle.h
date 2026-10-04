#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>

namespace atheon {
    struct file_data {
        std::string hash;
        std::string time;
        std::string size;
    };
    struct file_data_short {
        std::string time;
        std::string size;
    };

    void oracle();
    void load_index(std::unordered_map<std::string, atheon::file_data> &staged, std::string &buffer);
    void load_dir(std::unordered_map<std::string, atheon::file_data_short> &unstaged, const std::filesystem::path &base_dir);
    void load_head(std::unordered_map<std::string, std::string> &commited, std::string &buffer);
    bool mini_oracle();
}