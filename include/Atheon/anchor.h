#pragma once
#include <string>
#include <map>

namespace atheon {
    struct Tree {
        char type = 'T';
        std::string hash;
        std::string metadata = "040000";
        std::map<std::string,atheon::Tree> subtree;
    };

    void anchor(const std::string &message);
    void treeCreation(atheon::Tree &root);
    std::string write_tree_object(Tree& node);
    std::string commit_object_creation(const std::string &parent, const std::string &current_hash, const std::string &message);
}
