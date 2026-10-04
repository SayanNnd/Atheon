#include <cstdio>
#include <filesystem>
#include <iostream>
#include "../include/atheon/vault.h"

#ifdef _WIN32
    #include <windows.h>
#endif

namespace fs = std::filesystem;

namespace atheon {
    void vault() {
        //Checks if project is already initialized
        if (fs::exists(".atheon")) {
            std::cout << "Project is already initialized!\n";
            return;
        }

        //Creates directories and checks for errors
        if (!fs::create_directories(".atheon") || !fs::create_directories(".atheon/objects") || !fs::create_directories(".atheon/refs/heads")) {
            std::cout << "Error creating directory.\n";
            return;
        }
        //Hides the directory on windows
        #ifdef _WIN32
                SetFileAttributes(".atheon", FILE_ATTRIBUTE_HIDDEN);
        #endif

        //Makes the HEAD file
        FILE *ptr = fopen(".atheon/HEAD", "wb");
        const std::string ref = "ref: refs/heads/alpha";
        fwrite(ref.c_str(), sizeof(char), ref.length(), ptr);
        fclose(ptr);
    }

}