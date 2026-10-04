#include "../include/Atheon/oracle.h"
#include "../include/Atheon/ignore.h"
#include "../include/Atheon/conflux.h"
#include "../include/Atheon/sha256.h"
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <stack>
#include <iostream>
#include <algorithm>
#include <iomanip>

#define KB_128 (128*1024)

namespace atheon {
    void oracle() {
        if (!std::filesystem::exists(".atheon/")) {
            std::cout << "Project isn't Initialized\n";
            std::exit(EXIT_FAILURE);
        }
        load_ignore_rules();
        std::filesystem::path base_dir = std::filesystem::current_path();
        std::string buffer;

        //Load Index Status
        std::unordered_map<std::string, atheon::file_data> staged;
        atheon::load_index(staged, buffer);

        //Load Directory Status
        std::unordered_map<std::string, atheon::file_data_short> unstaged;
        atheon::load_dir(unstaged, base_dir);

        //Load HEAD Status
        std::unordered_map<std::string, std::string> commited;
        atheon::load_head(commited,buffer);

        std::vector<std::string> staged_new;
        std::vector<std::string> staged_modified;
        std::vector<std::string> staged_deleted;
        std::vector<std::string> unstaged_new;
        std::vector<std::string> unstaged_modified;
        std::vector<std::string> unstaged_deleted;

        //Staged to Commited
        for (auto &[location, data] : staged) {
            if (auto it = commited.find(location); it == commited.end()) {
                staged_new.emplace_back(location);
            }
            else if (it->second != data.hash) {
                staged_modified.emplace_back(location);
            }
        }
        //Commited to Staged
        for (auto &[location, hash] : commited) {
            if (staged.find(location) == staged.end()) {
                staged_deleted.emplace_back(location);
            }
        }

        //Un-Staged to Staged
        for (auto &[location, data] : unstaged) {
            if (auto it = staged.find(location); it == staged.end()) {
                unstaged_new.emplace_back(location);
            }
            else {
                uint64_t staged_time = std::stoull(it->second.time);
                uint64_t disk_time = std::stoull(data.time);
                if (it->second.size != data.size || staged_time != disk_time) {
                    FILE* file_ptr = fopen(location.c_str(), "rb");
                    if (!file_ptr) {
                        std::cout << "ERROR in reading files!\n";
                        std::exit(EXIT_FAILURE);
                    }
                    SHA256 hasher;
                    std::vector<uint8_t> initializer(KB_128);
                    size_t bytes_read = 0;
                    while ((bytes_read=fread(initializer.data(), 1, initializer.size(), file_ptr)) > 0) {
                        hasher.update(initializer.data(), bytes_read);
                    }
                    fclose(file_ptr);
                    if (std::string file_hash=hasher.final(); file_hash != it->second.hash) {
                        unstaged_modified.emplace_back(location);
                    }
                }
            }
        }
        //Staged to Commited
        for (auto &[location, data] : staged) {
            if (unstaged.find(location) == unstaged.end()) {
                unstaged_deleted.emplace_back(location);
            }
        }

        //Sorts the oracle data
        std::sort(staged_new.begin(), staged_new.end());
        std::sort(staged_modified.begin(), staged_modified.end());
        std::sort(staged_deleted.begin(), staged_deleted.end());
        std::sort(unstaged_new.begin(), unstaged_new.end());
        std::sort(unstaged_modified.begin(), unstaged_modified.end());
        std::sort(unstaged_deleted.begin(), unstaged_deleted.end());

        //Prints the whole oracle status
        std::cout << "\n===ATHEON ORACLE===\n" <<"\n\n";
        std::cout << "Changes to be Commited :\n";
        for (std::string &s : staged_new) {
            std::cout << std::left << "          " << std::setw(15) << "new file : " << s << "\n";
        }
        for (std::string &s : staged_modified) {
            std::cout << std::left << "          " << std::setw(15) << "modified : " << s << "\n";
        }
        for (std::string &s : staged_deleted) {
            std::cout << std::left << "          " << std::setw(15) << "deleted : " << s << "\n";
        }
        std::cout << "\nChanges not Staged for Commit :\n";
        for (std::string &s : unstaged_new) {
            std::cout << std::left << "          " << std::setw(15) << "new file : " << s << "\n";
        }
        for (std::string &s : unstaged_modified) {
            std::cout << std::left << "          " << std::setw(15) << "modified : " << s << "\n";
        }
        for (std::string &s : unstaged_deleted) {
            std::cout << std::left << "          " << std::setw(15) << "deleted : " << s << "\n";
        }
    }

    void load_index(std::unordered_map<std::string, atheon::file_data> &staged, std::string &buffer) {
        //Loads the last staged data
        if (std::ifstream index(".atheon/index"); !index.is_open()) {
            std::cout << "Index File Not found. Possibly never staged files yet.\n";
            index.close();
        }
        else {
            while (getline(index, buffer)) {
                std::stringstream ss(buffer);
                std::string hash, size, time, path;
                ss >> hash >> size >> time;
                std::getline(ss, path);
                if (!path.empty() && path.front() == ' ') path.erase(0, 1);
                if (path.empty()) continue;

                staged[path] = {
                    .hash = hash,
                    .time = time,
                    .size = size
                };
            }
        }
    }

    void load_dir(std::unordered_map<std::string, atheon::file_data_short> &unstaged, const std::filesystem::path &base_dir) {
        //Loads the current directory data
        std::stack<std::filesystem::path> dirs;
        dirs.push(base_dir);
        while (!dirs.empty()) {
            const std::filesystem::path dir = dirs.top();
            dirs.pop();
            for (auto &entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_directory() && !atheon::should_ignore(entry.path())) {
                    dirs.push(entry.path());
                }
                else if (entry.is_regular_file() && !atheon::should_ignore(entry.path())) {
                    unstaged[std::filesystem::relative(entry.path()).generic_string()] = {
                        .time = std::to_string((atheon::to_unix_timestamp(last_write_time(entry.path())))),
                        .size = std::to_string(std::filesystem::file_size(entry.path()))
                    };
                }
            }
        }
    }

    void load_head(std::unordered_map<std::string, std::string> &commited, std::string &buffer) {
        //Loads the last commit data
        std::ifstream HEAD(".atheon/HEAD");
        if (!HEAD.is_open()) {
            std::cout << "Error Opening Head file.\n";
            std::exit(EXIT_FAILURE);
        }
        buffer.clear();
        getline(HEAD, buffer);
        HEAD.close();
        if (!buffer.empty() && buffer.back() == '\r') {
            buffer.pop_back();
        }
        std::string commit_hash;
        std::string pre;
        std::string suf;
        if (buffer.substr(0,3) == "ref") {
            std::string HEAD_LOC;
            HEAD_LOC = ".atheon/"+buffer.substr(5);
            if (std::ifstream root_data(HEAD_LOC); !root_data.is_open()) {
                std::cout << "Root File doesn't exist. Possibly no Commits done yet.\n";
                root_data.close();
            }
            else {
                getline(root_data, buffer);
                root_data.close();
                if (!buffer.empty() && buffer.back() == '\r') {
                    buffer.pop_back();
                }
                commit_hash = buffer;
            }
        }
        else {
            commit_hash = buffer;
        }

        if (commit_hash.empty()) {
            return;
        }
        pre = commit_hash.substr(0,2);
        suf = commit_hash.substr(2,62);
        buffer.clear();
        std::string location = ".atheon/objects/";
        location += pre;
        location += '/';
        location += suf;
        std::ifstream root(location);
        getline(root, buffer);
        buffer = buffer.substr(5);
        std::stack<std::pair<std::string,std::string>> trav;
        trav.emplace("",buffer);
        buffer.clear();
        do {
            std::string loc = trav.top().second;
            std::string part = trav.top().first;
            trav.pop();
            pre = loc.substr(0,2);
            suf = loc.substr(2,62);
            location = ".atheon/objects/";
            location += pre;
            location += '/';
            location += suf;
            std::ifstream node(location);
            while (getline(node, buffer)) {
                if (!buffer.empty() && buffer.back() == '\r') {
                    buffer.pop_back();
                }
                if (buffer.empty()) continue;

                size_t space_pos = buffer.find(' ');
                size_t null_pos = buffer.find('\0');
                if (space_pos == std::string::npos || null_pos == std::string::npos) {
                    continue;
                }
                size_t length = null_pos - space_pos - 1;

                if (buffer.substr(0, 5) == "10064") {
                    commited[part + buffer.substr(space_pos + 1, length)] = buffer.substr(null_pos + 1);
                } else {
                    trav.emplace(part + buffer.substr(space_pos + 1, length) + '/', buffer.substr(null_pos + 1));
                }
            }
        } while (!trav.empty());
    }

    bool mini_oracle() {
        //Miniature version of the oracle to check if the project is clean
        load_ignore_rules();
        std::filesystem::path base_dir = std::filesystem::current_path();
        std::string buffer;
        std::unordered_map<std::string, atheon::file_data> staged;
        atheon::load_index(staged, buffer);
        std::unordered_map<std::string, atheon::file_data_short> unstaged;
        atheon::load_dir(unstaged, base_dir);
        std::unordered_map<std::string, std::string> commited;
        atheon::load_head(commited,buffer);
        std::vector<std::string> staged_new;
        std::vector<std::string> staged_modified;
        std::vector<std::string> staged_deleted;
        std::vector<std::string> unstaged_modified;
        std::vector<std::string> unstaged_new;
        std::vector<std::string> unstaged_deleted;
        for (auto &[location, data] : staged) {
            if (auto it = commited.find(location); it == commited.end()) {
                staged_new.emplace_back(location);
            }
            else if (it->second != data.hash) {
                staged_modified.emplace_back(location);
            }
        }
        for (auto &[location, hash] : commited) {
            if (staged.find(location) == staged.end()) {
                staged_deleted.emplace_back(location);
            }
        }
        for (auto &[location, data] : unstaged) {
            if (auto it = staged.find(location); it == staged.end()) {
                unstaged_new.emplace_back(location);
            }
            else {
                uint64_t staged_time = std::stoull(it->second.time);
                uint64_t disk_time = std::stoull(data.time);
                if (it->second.size != data.size || staged_time != disk_time) {
                    FILE* file_ptr = fopen(location.c_str(), "rb");
                    if (!file_ptr) {
                        std::cout << "ERROR in reading files!\n";
                        std::exit(EXIT_FAILURE);
                    }
                    SHA256 hasher;
                    std::vector<uint8_t> initializer(KB_128);
                    size_t bytes_read = 0;
                    while ((bytes_read=fread(initializer.data(), 1, initializer.size(), file_ptr)) > 0) {
                        hasher.update(initializer.data(), bytes_read);
                    }
                    fclose(file_ptr);
                    if (std::string file_hash=hasher.final(); file_hash != it->second.hash) {
                        unstaged_modified.emplace_back(location);
                    }
                }
            }
        }
        for (auto &[location, data] : staged) {
            if (unstaged.find(location) == unstaged.end()) {
                unstaged_deleted.emplace_back(location);
            }
        }
        if (!staged_new.empty()) return false;
        if (!staged_modified.empty()) return false;
        if (!staged_deleted.empty()) return false;
        if (!unstaged_new.empty()) return false;
        if (!unstaged_modified.empty()) return false;
        if (!unstaged_deleted.empty()) return false;

        return true;
    }
}