cmake_minimum_required(VERSION 3.10)

# Project name must be Compiler per submission requirements
project(Compiler)

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

# Automatically pick up any additional .cpp files under midend/
file(GLOB_RECURSE EXTRA_MID_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/midend/*.cpp
)
list(APPEND SRC_FILES ${EXTRA_MID_SOURCES})

# Create executable target
add_executable(Compiler ${SRC_FILES})

# Target include directories (private)
target_include_directories(Compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/error
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/ast
    ${CMAKE_CURRENT_SOURCE_DIR}/midend
    ${CMAKE_CURRENT_SOURCE_DIR}/midend/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/midend/symbol
)

# Compile options
if (MSVC)
    target_compile_options(Compiler PRIVATE /W3 /permissive-)
else()
    target_compile_options(Compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Force the runtime output name to 'Compiler' (on Windows this will produce Compiler.exe)
# Place the runtime output in the CMake binary directory so building in an out-of-source
# `build` folder produces `build/Compiler` as the platform expects.
set_target_properties(Compiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_NAME "Compiler"
)
