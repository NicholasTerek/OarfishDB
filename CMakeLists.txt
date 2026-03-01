cmake_minimum_required(VERSION 3.16)
project(lsm_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
option(ENABLE_ASAN "Enable AddressSanitizer for tests" OFF)

file(GLOB_RECURSE LSM_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_library(lsm ${LSM_SOURCES})
target_include_directories(lsm PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src/include
)
target_compile_options(lsm PRIVATE $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>)

if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(googletest)

  include(GoogleTest)
  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "test/*_test.cpp")
  foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} PRIVATE lsm GTest::gtest_main pthread)
    if(ENABLE_ASAN)
      target_compile_options(${test_name} PRIVATE -fsanitize=address)
      target_link_options(${test_name} PRIVATE -fsanitize=address)
    endif()
    gtest_discover_tests(${test_name})
  endforeach()
endif()
