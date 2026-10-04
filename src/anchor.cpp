#include "../include/Atheon/anchor.h"
#include "../include/Atheon/sha256.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdio>

#define KB_128 (128*1024)

namespace atheon {
    void anchor(const std::string &message) {
        //Checks if the project is initialized
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

        //Creates Initial DAG Tree
        atheon::Tree root;
        treeCreation(root);

        //Reverse reads the Tree to Inject hashes into Branches
        const std::string root_hash = write_tree_object(root);

        //Opens Head to read last commit
        std::ifstream HEAD(".atheon/HEAD");
        std::string buffer;
        getline(HEAD, buffer);
        HEAD.close();
        std::string HEAD_LOC = ".atheon/"+buffer.substr(5);
        std::string parent_hash;
        if (std::ifstream ref(HEAD_LOC); ref.is_open()) {
            std::getline(ref, parent_hash);
            if (!parent_hash.empty() && parent_hash.back() == '\r') parent_hash.pop_back();
            ref.close();
        }

        //Checks if the commit has changes
        if (!parent_hash.empty()) {
            std::string pre = parent_hash.substr(0,2);
            std::string suf = parent_hash.substr(2,62);
            std::ifstream file(".atheon/objects/"+pre+"/"+suf);
            if (std::string line; getline(file, line)) {
                if (line.substr(5) == root_hash) {
                    std::cout << "Nothing to commit, working tree clean.\n";
                    return;
                }
            }
        }

        //Creates Commit Object
        const std::string commit_hash = commit_object_creation(parent_hash, root_hash,message);
        FILE* ptr = fopen(HEAD_LOC.c_str(), "wb");
        if (ptr==nullptr) {
            std::exit(EXIT_FAILURE);
        }
        fwrite(commit_hash.data(), 1, commit_hash.size(), ptr);
        fclose(ptr);
    }

    void treeCreation(atheon::Tree &root) {
        std::ifstream file(".atheon/index");
        if (!file.is_open()) {
            std::cout << "Please stage the current changes first.\n";
            std::exit(EXIT_FAILURE);
        }
        std::string line;
        //Reads Index File to create a tree
        while (getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            const size_t p1 = line.find(' ');
            if (p1 == std::string::npos) continue;
            const size_t p2 = line.find(' ', p1 + 1);
            if (p2 == std::string::npos) continue;
            const size_t p3 = line.find(' ', p2 + 1);
            if (p3 == std::string::npos) continue;

            const std::string hash = line.substr(0, p1);
            const std::string location = line.substr(p3 + 1);

            std::vector<std::string> tokens;
            std::string token;
            for (char c : location) {
                if (c=='/') {
                    tokens.push_back(token);
                    token.clear();
                }
                else {
                    token+=c;
                }
            }
            tokens.push_back(token);

            //Starts Tree creation with root
            Tree *current = &root;
            current->metadata = "040000";
            current->type = 'T';
            int size = static_cast<int>(tokens.size());
            for (int j = 0; j < size; j++) {
                //Keeps adding folder layers until the token reaches the end(filename)
                if (j!=size-1) {
                    Tree &folder = current->subtree[tokens[j]];
                    if (folder.type == '\0') {
                        folder.type = 'T';
                        folder.metadata = "040000";
                    }
                    current = &folder;
                }
                else {
                    Tree &leaf = current->subtree[tokens[j]];
                    leaf.metadata = "100644";
                    leaf.type = 'L';
                    leaf.hash = hash;
                }
            }
        }
    }

    std::string write_tree_object(Tree& node) {
        //Returns for files(Leaf)
        if (node.type == 'L') {
            return node.hash;
        }

        //Creates Tree object and generates Hash
        std::string payload;
        for (auto &[name,child] : node.subtree) {
            child.hash = write_tree_object(child);
            payload += child.metadata+' '+name+'\0'+child.hash+'\n';
        }
        SHA256 hasher;
        hasher.update(reinterpret_cast<const uint8_t *>(payload.data()), payload.size());
        node.hash=hasher.final();

        const std::string pre = node.hash.substr(0,2);
        const std::string suf = node.hash.substr(2,62);
        std::string location = ".atheon/objects/";
        location += pre;
        if (!std::filesystem::exists(location)) {
            std::filesystem::create_directory(location);
        }
        location += '/';
        location += suf;
        if (!std::filesystem::exists(location)) {
            FILE* ptr = fopen(location.c_str(), "wb");
            if (ptr==nullptr) {
                std::cout << "ERROR in creating file at "<< std::filesystem::path(location).generic_string() << "!\n";
                std::filesystem::remove(location);
                std::exit(EXIT_FAILURE);
            }
            fwrite(payload.data(), 1, payload.size(), ptr);
            fclose(ptr);
        }
        return node.hash;
    }

    std::string commit_object_creation(const std::string &parent, const std::string &current_hash, const std::string &message) {
        //Creates Commit object with all required data
        //TODO - ADD USER INFO
        const std::string time = std::to_string(std::time(nullptr));
        std::string payload="tree "+current_hash;
        if (!parent.empty()) {
            payload+="\nparent "+parent;
        }
        payload+="\ntimestamp "+time+"\nmessage "+message;
        SHA256 hasher;
        hasher.update(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());
        const std::string hash = hasher.final();
        const std::string pre = hash.substr(0,2);
        const std::string suf = hash.substr(2,62);

        std::string location = ".atheon/objects/"+pre;
        if (!std::filesystem::exists(location)) {
            std::filesystem::create_directory(location);
        }
        location += '/';
        location += suf;
        FILE* ptr = fopen(location.c_str(), "wb");
        if (ptr==nullptr) {
            std::exit(EXIT_FAILURE);
        }
        fwrite(payload.data(), 1, payload.size(), ptr);
        fclose(ptr);
        return hash;
    }
}