# 3.25: FetchContent_Declare(... SYSTEM) keeps third-party headers out of our -Werror.
cmake_minimum_required(VERSION 3.25)
# C is needed for HdrHistogram_c.
project(MiniTub LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# compile_commands.json feeds clang-tidy and clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# All executables in one folder (build/<preset>/bin), so they are easy to find.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(MINITUB_CLANG_TIDY "Run clang-tidy on our sources while compiling" OFF)

# Sanitizers first: flags must also reach dependencies (TSan needs everything instrumented).
include(cmake/Sanitizers.cmake)
include(cmake/Warnings.cmake)
include(cmake/Dependencies.cmake)

enable_testing()
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(bench)
