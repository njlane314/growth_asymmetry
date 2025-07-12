cmake_minimum_required(VERSION 3.10)
project(GrowthAsymmetry)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options("-Wno-psabi")

include_directories(/usr/local/include/json/include)
include_directories(/usr/local/include/Catch2/src)

add_subdirectory(framework)

add_executable(stock_screener stock_screener.cpp)
target_link_libraries(stock_screener PRIVATE framework)

add_executable(test_framework test_framework.cpp)
target_link_libraries(test_framework PRIVATE framework)