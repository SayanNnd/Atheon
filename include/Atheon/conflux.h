#pragma once
#include <vector>
#include <filesystem>
#include <map>

namespace atheon {
    struct Metadata {
        std::string Hash;
        uint64_t Filesize{0};
        int64_t time{0};
    };
    void conflux(const std::string &stager);
    void worker_assign(const std::vector<std::filesystem::directory_entry> &entries,std::vector<std::string> &HashList);
    void worker_map(const std::vector<std::filesystem::directory_entry> &files, const std::vector<std::string> &HashList, std::map<std::string,atheon::Metadata> &data);
    uint64_t create_blob(const std::string &writeName, const std::filesystem::path &read_dir);
    int64_t to_unix_timestamp(const std::filesystem::file_time_type &time);
    void load_index(std::map<std::string, Metadata> &data);
}