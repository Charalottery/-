cmake_minimum_required(VERSION 3.28)

project(Compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# By default we exclude example/test sources that define their own `main()`.
option(EXCLUDE_EXAMPLES "Exclude example/test sources that define their own main()" ON)

# Directories to search for .cpp files (relative to this CMakeLists)
set(SRCDIRS
    backend
    error
    frontend
    midend
    optimize
    tools
    utils
)

# Collect all .cpp files under SRCDIRS (do per-directory glob to avoid
# issues with expanding multiple dir patterns in one call)
set(ALL_SRCS)
foreach(dir IN LISTS SRCDIRS)
    # ensure directory exists before globbing to avoid warnings
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        file(GLOB_RECURSE _tmp RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*.cpp")
        list(APPEND ALL_SRCS ${_tmp})
    endif()
endforeach()
list(REMOVE_DUPLICATES ALL_SRCS)

# Filter out any files accidentally under build or CMakeFiles (in-case an
# in-source build was performed). Also ensure top-level main.cpp is included
# if present (but we avoid recursing the top-level to prevent picking up
# build/CMakeFiles/CompilerIdCXX.cpp which defines its own main()).
set(_tmp_filtered)
foreach(f IN LISTS ALL_SRCS)
    if(f MATCHES "(^|/)(CMakeFiles|build)(/|$)")
        # skip files under build or CMakeFiles
    else()
        list(APPEND _tmp_filtered ${f})
    endif()
endforeach()
set(ALL_SRCS ${_tmp_filtered})

# If top-level main.cpp exists, include it explicitly
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    list(REMOVE_ITEM ALL_SRCS main.cpp)
    list(INSERT ALL_SRCS 0 main.cpp)
endif()

# Filter out example mains (e.g. ir_example*.cpp or *_main.cpp), but always
# keep the project's top-level `main.cpp`.
set(SOURCES)
foreach(f IN LISTS ALL_SRCS)
    if(EXCLUDE_EXAMPLES)
        if(f MATCHES "ir_example" OR (f MATCHES "_main\\.cpp" AND NOT f MATCHES "^main\\.cpp$"))
            # skip example/main test files
        else()
            list(APPEND SOURCES ${f})
        endif()
    else()
        list(APPEND SOURCES ${f})
    endif()
endforeach()

if(NOT SOURCES)
    message(FATAL_ERROR "No source files found. Check SRCDIRS or disable EXCLUDE_EXAMPLES.")
endif()

list(LENGTH SOURCES SRC_COUNT)
message(STATUS "Compiler: Found ${SRC_COUNT} source files.")

## Create the single executable (target named `Compiler`)
add_executable(Compiler ${SOURCES})

## Ensure produced executable is named `Compiler` (no extension)
set_target_properties(Compiler PROPERTIES OUTPUT_NAME "Compiler")

## Compiler flags (use C++17 by default; evaluator uses clang++ 12 with C++17)
if(MSVC)
    target_compile_options(Compiler PRIVATE /W3 /EHsc)
else()
    target_compile_options(Compiler PRIVATE -Wall -Wextra -O2 -g)
endif()

## Include project root for headers
target_include_directories(Compiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "Build with: cmake -S . -B build -DCMAKE_CXX_COMPILER=/path/to/clang++ -DCMAKE_BUILD_TYPE=Release")

message(STATUS "Note: For submission include this CMakeLists.txt at archive root and do NOT include the 'build' directory or other CMake temporary files.")