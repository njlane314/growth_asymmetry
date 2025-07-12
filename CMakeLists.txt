cmake_minimum_required(VERSION 3.14)
project(GrowthAsymmetry)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options("-Wno-psabi")

# Include FetchContent module
include(FetchContent)

# Declare and fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.x
)
FetchContent_MakeAvailable(Catch2)

# Declare and fetch nlohmann/json
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2
)
FetchContent_MakeAvailable(json)

add_subdirectory(framework)

add_executable(stock_screener stock_screener.cpp)
target_link_libraries(stock_screener PRIVATE framework)

add_executable(test_framework test_framework.cpp)
target_link_libraries(test_framework PRIVATE framework Catch2::Catch2)

enable_testing()