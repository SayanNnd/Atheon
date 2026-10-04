#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stack>
#include <thread>
#include <vector>
#include "../include/Atheon/conflux.h"
#include "../include/Atheon/sha256.h"
#include "../include/Atheon/ignore.h"


#define KB_128 (128*1024)

namespace atheon {
    //Initializing atomic number and base directory
    static std::atomic<size_t> global_index(0);
    static std::filesystem::path base_dir = std::filesystem::current_path();

    void conflux(const std::string &stager) {
        if (!std::filesystem::exists(".atheon")) {
            std::cout << "Project not initialized.\n";
            std::exit(EXIT_FAILURE);
        }

        //Check if user is in latest commit of branch
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
        std::string parent;
        if (head_content.rfind("ref: ", 0) != 0) {
            std::cout << "Please go back to the current commit of the branch before making more confluxes!!\n";
            std::exit(EXIT_FAILURE);
        }

        //Loading the older index file into map if it alr exists
        std::map<std::string,atheon::Metadata> data;
        if (std::filesystem::exists(".atheon/index")) {
            atheon::load_index(data);
        }
        global_index.store(0);
        std::vector<std::string> HashList(1);
        std::vector<std::filesystem::directory_entry> files;
        size_t thread_count = std::thread::hardware_concurrency()/2;
        if (thread_count == 0) thread_count = 1;

        //Loading the ignore rules
        atheon::load_ignore_rules();

        //Staging area
        if (stager==".") {
            //Generating a file list
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
                        files.push_back(entry);
                    }
                }
            }
            HashList.resize(files.size());

            //Hashing the file list using multiple threads
            std::vector<std::thread> working_threads;
            for (size_t i = 1; i <= thread_count; i++) {
                working_threads.emplace_back(atheon::worker_assign,std::ref(files),std::ref(HashList));
            }
            for (auto& t : working_threads) {
                t.join();
            }
        }
        else {
            //Staging single file
            if (std::filesystem::exists(stager) && std::filesystem::is_regular_file(stager)) {
                files.emplace_back(stager);
                std::thread t1(atheon::worker_assign, std::ref(files),std::ref(HashList));
                t1.join();
            }
            else {
                std::cout << "File does not exist: " << stager << std::endl;
            }
        }

        //Adding the file list and hash into map using multiple threads
        global_index.store(0);
        std::vector<std::thread> working_threads;
        for (size_t i{0}; i<thread_count; i++) {
            working_threads.emplace_back(atheon::worker_map, ref(files),ref(HashList), ref(data));
        }
        for (auto& t : working_threads) {
            t.join();
        }

        //Stores the data in index for commits
        std::ofstream output(".atheon/index.temp");
        for (auto &[key,value] : data) {
            output << value.Hash << " " << value.Filesize << " " << value.time << " " << key << std::endl;
        }
        output.close();

        //Renames the temp index back to original index
        std::filesystem::rename(".atheon/index.temp", ".atheon/index");
    }

    void worker_assign(const std::vector<std::filesystem::directory_entry> &entries,std::vector<std::string> &HashList) {
        const size_t n = entries.size();
        while (true) {
            //Grabs the number from atomic
            const size_t curr_no = global_index.fetch_add(1);
            if (curr_no >= n) break;

            //Loads pointer to he file
            FILE* file_ptr = fopen(entries[curr_no].path().string().c_str(), "rb");
            if (!file_ptr) {
                std::cout << "ERROR in reading files!\n";
                std::exit(EXIT_FAILURE);
            }

            //Initializes the hasher variables
            SHA256 hasher;
            std::vector<uint8_t> buffer(KB_128);
            size_t bytes_read = 0;

            //Reads at a buffer of 128KB
            while ((bytes_read=fread(buffer.data(), 1, buffer.size(), file_ptr)) > 0) {
                hasher.update(buffer.data(), bytes_read);
            }
            fclose(file_ptr);

            //Pushes the Hash into the vector
            HashList[curr_no]=hasher.final();
        }
    }

    void worker_map(const std::vector<std::filesystem::directory_entry> &files, const std::vector<std::string> &HashList, std::map<std::string,atheon::Metadata> &data) {
        const size_t n = files.size();
        while (true) {
            //Grabs the number from atomic
            const size_t curr_no = global_index.fetch_add(1);
            if (curr_no >= n) break;

            //Checks for pre-existing blob
            const std::string pre = HashList[curr_no].substr(0,2);
            const std::string suf = HashList[curr_no].substr(2,62);
            std::string location = ".atheon/objects/";
            location += pre;
            location += '/';
            location += suf;
            auto it = data.find(std::filesystem::relative(files[curr_no].path(),base_dir).generic_string());
            if (std::filesystem::exists(std::filesystem::path(location)) && it != data.end() && it->second.Hash == HashList[curr_no]) continue;
            std::filesystem::create_directory(".atheon/objects/"+pre);

            //Feeds data into the map
            data[std::filesystem::relative(files[curr_no].path(),base_dir).generic_string()] = {
                .Hash = HashList[curr_no],
                .Filesize = atheon::create_blob(location,files[curr_no].path()),                              //Creates blob and returns blob size
                .time = atheon::to_unix_timestamp(std::filesystem::last_write_time(files[curr_no].path()))         //Converter to generate standard time in both UNIX and WINDOWS systems
            };
        }
    }

    uint64_t create_blob(const std::string &writeName, const std::filesystem::path &read_dir) {
        //Opens both files to Read from and Write to
        FILE *re_ptr = fopen(read_dir.string().c_str(), "rb");
        if (re_ptr==nullptr) {
            std::cout << "ERROR in reading file at "<< read_dir.generic_string() << "!\n";
            std::exit(EXIT_FAILURE);
        }
        FILE *wr_ptr = fopen(writeName.c_str(), "wb");
        if (wr_ptr==nullptr) {
            std::cout << "ERROR in creating file at "<< std::filesystem::path(writeName).generic_string() << "!\n";
            std::filesystem::remove(writeName);
            std::exit(EXIT_FAILURE);
        }

        //Initializes variables and creates the blob header
        uint64_t total_bytes_read = 0;
        std::string x = "blob " + std::to_string(std::filesystem::file_size(read_dir));
        x.push_back('\0');
        fwrite (x.data(),1,x.size(),wr_ptr);

        //Writes the blob at a buffer of 128 KB
        std::vector<uint8_t> buffer(KB_128);
        size_t bytes_read = 0;
        while ((bytes_read = fread(buffer.data(), 1, buffer.size(), re_ptr)) > 0) {
            fwrite(buffer.data(), 1, bytes_read, wr_ptr);
            total_bytes_read += bytes_read;
        }
        fclose(re_ptr);
        fclose(wr_ptr);
        return total_bytes_read;     //Returns the blob size back
    }

    int64_t to_unix_timestamp(const std::filesystem::file_time_type &time) {
        //Returns standard times for both UNIX and WINDOWS systems
        const auto sys_tp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - std::filesystem::file_time_type::clock::now()
            + std::chrono::system_clock::now());
        return std::chrono::duration_cast<std::chrono::seconds>(sys_tp.time_since_epoch()).count();
    }

    void load_index(std::map<std::string, atheon::Metadata> &data) {
        //Reads old index file to update the Map
        std::ifstream read(".atheon/index");
        std::string line;
        while (getline(read, line)) {
            int count{0};

            //Metadata variables
            std::string path;
            std::string hash;
            uint64_t size{0};
            int64_t time{0};
            for (const char c : line) {
                if (c == ' ' && count<3) {
                    count++;
                    continue;
                }
                if (count == 0) {
                    hash+=c;
                }
                if (count == 1) {
                    size=(size*10)+(c-'0');
                }
                if (count == 2) {
                    time=(time*10)+(c-'0');
                }
                if (count == 3) {
                    path+=c;
                }
            }
            if (std::filesystem::exists(std::filesystem::path(path))) {
                data[path] = {
                    .Hash = hash,
                    .Filesize = size,
                    .time = time,
                };
            }
        }
    }
}
