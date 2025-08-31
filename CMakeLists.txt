cmake_minimum_required(VERSION 3.21)
project(Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Catch2 REQUIRED)
find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

add_library(my_library lib.cpp)
target_include_directories(my_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(my_cli_tool cli.cpp)
target_link_libraries(my_cli_tool PRIVATE my_library)

add_executable(my_gui_tool gui.cpp)
target_link_libraries(my_gui_tool PRIVATE my_library Qt6::Core Qt6::Widgets)

# Catch2 test runner. Compiles tests.cpp and links Catch2's main.
add_executable(my_catch2_tests tests.cpp)
target_link_libraries(my_catch2_tests PRIVATE my_library Catch2::Catch2WithMain)

# GoogleTest test runner. Compiles tests2.cpp and links GTest main.
add_executable(my_gtest_tests tests2.cpp)
target_link_libraries(my_gtest_tests PRIVATE my_library GTest::gtest_main)