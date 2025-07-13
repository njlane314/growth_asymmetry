cmake_minimum_required(VERSION 3.14)
project(GrowthAsymmetry)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options("-Wno-psabi")

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.x
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2
)
FetchContent_MakeAvailable(json)

add_subdirectory(framework)

add_executable(stock_screener stock_screener.cpp)
target_link_libraries(stock_screener PRIVATE framework sqlite3 minizip)

add_executable(test_framework test_framework.cpp)
target_link_libraries(test_framework PRIVATE framework Catch2::Catch2 sqlite3 minizip)

add_executable(database_processor database_processor.cpp)
target_link_libraries(database_processor PRIVATE framework sqlite3 minizip)

add_executable(investable_universe investable_universe.cpp)
target_link_libraries(investable_universe PRIVATE framework sqlite3 minizip)

enable_testing()