cmake_minimum_required(VERSION 3.10)

# Project name must be Compiler per submission requirements
project(Compiler)

# Allow selecting C++ standard; default to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files (match Makefile SRCS)
set(SRC_FILES
    main.cpp
    frontend/FrontendDriver.cpp
    frontend/lexer/lexer.cpp
    frontend/parser/Parser.cpp
    frontend/parser/ParserHelper.cpp
    frontend/parser/ParserOutput.cpp
    frontend/parser/ParserWriter.cpp
    frontend/ast/CompUnit.cpp
    frontend/ast/Node.cpp
    frontend/ast/AST.cpp
)

# Include current source dir for headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Create executable named 'Compiler' (CMake will append .exe on Windows)
add_executable(Compiler ${SRC_FILES})

# Optional: set compile warnings and common flags
if (MSVC)
    target_compile_options(Compiler PRIVATE /W3)
else()
    target_compile_options(Compiler PRIVATE -Wall -Wextra -Wpedantic)
    # preserve debug symbols by default; allow overriding CMAKE_BUILD_TYPE
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()
endif()

# Ensure the executable name is exactly "Compiler" (without extra prefix/suffix)
set_target_properties(Compiler PROPERTIES OUTPUT_NAME "Compiler")
