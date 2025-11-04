cmake_minimum_required(VERSION 3.10)

# Project name must be Compiler per submission requirements
project(Compiler)

# Allow selecting C++ standard; default to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SRC_FILES
    lexer.cpp
    main.cpp
)

# Add include directories if needed (current dir)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Create executable named 'Compiler'
add_executable(Compiler ${SRC_FILES})

# Optional: set compile warnings
if (MSVC)
    target_compile_options(Compiler PRIVATE /W3)
else()
    target_compile_options(Compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()
