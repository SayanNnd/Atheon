cmake_minimum_required(VERSION 3.20)
project(atheon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

file(GLOB_RECURSE SOURCES
        "src/*.cpp"
        "include/Atheon/*.h"
)

include_directories(include)
add_executable(atheon ${SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(atheon PRIVATE -O3 -march=native)

elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(atheon PRIVATE -g -Wall -Wextra -Werror)
endif()