#include "../include/Atheon/timegate.h"
#include "../include/Atheon/oracle.h"
#include "../include/Atheon/conflux.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <stack>
#include <unordered_map>
#include <cstdio>
#include <vector>

#define KB_128 (128*1024)

namespace atheon {
    void timegate(std::string commit_hash, std::string location) {
        if (!std::filesystem::exists(".atheon/")) {
            std::cout << "Project isn't Initialized\n";
            std::exit(EXIT_FAILURE);
        }
        std::string buffer;
        std::string pre;
        std::string suf;

        //Opens a miniature version of Oracle to check if the project is clean
        if (!atheon::mini_oracle()) {
            std::cout << "There are still unstaged files or uncommited files in the Project\nPress 0 to Force Checkout (Deletes current changes) or Press 1 to exit\n";
            int x;
            std::cin >> x;
            std::cout << "\033[1A\033[2K\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (x!=0) {
                std::exit(EXIT_SUCCESS);
            }
        }

        //Opens Commit Object
        std::ifstream file(location);
        getline(file,buffer);
        file.close();
        buffer = buffer.substr(5);
        std::unordered_map<std::string,std::string> commit_files;
        //Loads all the commited files data
        atheon::load_commited(buffer, location, commit_files);
        buffer.clear();

        //Deletes indexed files which aren't in commit
        std::unordered_map<std::string, atheon::file_data> index;
        atheon::load_index(index,buffer);
        for (auto &[loc,data] : index) {
            if (commit_files.find(loc) == commit_files.end()) {
                std::filesystem::remove(loc);
            }
        }

        //Replaces new files with older commit files
        for (auto &[loc,hash] : commit_files) {
            if (auto parent_p = std::filesystem::path(loc).parent_path(); !parent_p.empty()) {
                std::filesystem::create_directories(parent_p);
            }
            pre = hash.substr(0,2);
            suf = hash.substr(2);
            std::string hash_loc = ".atheon/objects/";
            hash_loc += pre;
            hash_loc += '/';
            hash_loc += suf;
            FILE *re_ptr = fopen(hash_loc.c_str(), "rb");
            if (re_ptr==nullptr) {
                std::cout << "ERROR in reading file at "<< hash_loc << "!\n";
                std::exit(EXIT_FAILURE);
            }
            FILE *wr_ptr = fopen(loc.c_str(), "wb");
            if (wr_ptr==nullptr) {
                std::cout << "ERROR in editing file at "<< loc << "!\n";
                std::exit(EXIT_FAILURE);
            }
            int ch;
            while ((ch = fgetc(re_ptr)) != '\0' && ch != EOF) {}
            std::vector<uint8_t> reader(KB_128);
            size_t bytes_read = 0;
            while ((bytes_read = fread(reader.data(), 1, reader.size(), re_ptr)) > 0) {
                fwrite(reader.data(), 1, bytes_read, wr_ptr);
            }
            fclose(re_ptr);
            fclose(wr_ptr);
        }

        //Rewrites index to match new project
        FILE* index_ptr = fopen(".atheon/index", "wb");
        if (index_ptr==nullptr) {
            std::cout << "ERROR in creating new Index File\n";
            std::exit(EXIT_FAILURE);
        }
        for (const auto &[path, blob_hash] : commit_files) {
            uint64_t size = std::filesystem::file_size(std::filesystem::path(path));
            int64_t timestamp = atheon::to_unix_timestamp(std::filesystem::last_write_time(std::filesystem::path(path)));
            std::string printer = blob_hash;
            printer += " "+std::to_string(size);
            printer += " "+std::to_string(timestamp);
            printer += " "+path+'\n';
            fwrite(printer.data(), 1, printer.size(), index_ptr);
        }
        fclose(index_ptr);

        //Changes HEAD to a de-referenced HEAD - Fixed by going back to top of branch
        FILE *head_ptr = fopen(".atheon/HEAD", "wb");
        if (head_ptr==nullptr) {
            std::cout << "ERROR in updating HEAD!\n";
            std::exit(EXIT_FAILURE);
        }
        fwrite(commit_hash.data(), 1, commit_hash.size(), head_ptr);
        fclose(head_ptr);
    }

    void load_commited(std::string &buffer, std::string &location, std::unordered_map<std::string,std::string> &commited) {
        //Loads Data of all commited files
        std::stack<std::pair<std::string,std::string>> trav;
        trav.emplace("",buffer);
        buffer.clear();
        do {
            std::string loc = trav.top().second;
            std::string part = trav.top().first;
            trav.pop();
            std::string pre = loc.substr(0,2);
            std::string suf = loc.substr(2,62);
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
}