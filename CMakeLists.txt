cmake_minimum_required(VERSION 3.10)
project(SevenWondersDuel)

set(CMAKE_CXX_STANDARD 17)

# Headers
include_directories(include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable
add_executable(SevenWondersDuel main.cpp ${SOURCES})

# Include paths for the target
target_include_directories(SevenWondersDuel PUBLIC include)
