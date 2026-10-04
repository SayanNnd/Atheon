#pragma once
#include <filesystem>

namespace atheon {
    void load_ignore_rules();
    bool should_ignore(const std::filesystem::path &filepath);
}
