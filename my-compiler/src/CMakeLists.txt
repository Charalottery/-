cmake_minimum_required(VERSION 3.10)

# Project name must be Compiler per submission requirements
project(Compiler VERSION 0.1 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files (explicit + discover other frontend sources automatically)
set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/lexer/lexer.cpp
)

# Automatically pick up any additional .cpp files under frontend/ (helpful as parser/ast are added)
file(GLOB_RECURSE EXTRA_FRONTEND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/*.cpp
)
list(REMOVE_ITEM EXTRA_FRONTEND_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/frontend/lexer/lexer.cpp)
list(APPEND SRC_FILES ${EXTRA_FRONTEND_SOURCES})

# Create executable target
add_executable(Compiler ${SRC_FILES})

# Target include directories (private)
target_include_directories(Compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/error
)

# Compile options
if (MSVC)
    target_compile_options(Compiler PRIVATE /W3 /permissive-)
else()
    target_compile_options(Compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Ensure the built exe is placed in the src directory for easy testing
set_target_properties(Compiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
