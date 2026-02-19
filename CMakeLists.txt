cmake_minimum_required(VERSION 3.16)
project(lsm_project LANGUAGES CXX)

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Build type (default Debug if not set) ----
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---- Options ----
option(ENABLE_ASAN "Enable AddressSanitizer for tests" ON)

# ---- GTest (FetchContent: builds Linux gtest inside WSL; avoids Windows gtest.lib) ----
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Safe on Linux; keeps gtest from trying to do MSVC CRT stuff if you ever build on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---- Auto-discover source files ----
file(GLOB_RECURSE LSM_SOURCES "src/*.cpp")
file(GLOB_RECURSE LSM_HEADERS "src/include/*.hpp")

# ---- Main LSM Library ----
add_library(lsm ${LSM_SOURCES} ${LSM_HEADERS})
target_include_directories(lsm PUBLIC
  src/include          # For direct header names: #include "mem_table.hpp"
  ${CMAKE_SOURCE_DIR}  # For full paths: #include "src/include/mem_table.hpp"
)

# Common compile options for all targets
set(DEBUG_COMPILE_OPTIONS -O0 -g3 -fno-omit-frame-pointer)
target_compile_options(lsm PRIVATE $<$<CONFIG:Debug>:${DEBUG_COMPILE_OPTIONS}>)

# ---- Test Discovery ----
file(GLOB_RECURSE TEST_SOURCES "test/*_test.cpp")

# Function to create tests automatically
function(create_test test_file)
  # Get relative path from test directory to avoid name conflicts
  file(RELATIVE_PATH rel_path "${CMAKE_SOURCE_DIR}/test" ${test_file})

  # Remove .cpp and replace / with _ to create unique test names
  string(REPLACE ".cpp" "" test_name ${rel_path})
  string(REPLACE "/" "_" test_name ${test_name})

  add_executable(${test_name} ${test_file})
  target_link_libraries(${test_name} PRIVATE lsm GTest::gtest_main pthread)
  target_compile_options(${test_name} PRIVATE $<$<CONFIG:Debug>:${DEBUG_COMPILE_OPTIONS}>)

  # Add the source directory as include path so tests can use "src/include/header.hpp"
  target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR})

  # Add ASan if enabled
  if(ENABLE_ASAN)
    target_compile_options(${test_name} PRIVATE -fsanitize=address)
    target_link_options(${test_name} PRIVATE -fsanitize=address)
  endif()

  # Register with CTest
  string(REPLACE "_test" "Test" test_display_name ${test_name})
  string(REPLACE "_" "" test_display_name ${test_display_name})
  add_test(NAME ${test_display_name} COMMAND ${test_name})
endfunction()

# Create all tests automatically
foreach(test_file ${TEST_SOURCES})
  create_test(${test_file})
endforeach()

# ---- CTest integration ----
enable_testing()
