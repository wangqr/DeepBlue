cmake_minimum_required(VERSION 2.6)
project(ai)

# Build ai
add_executable (ai ai.cpp)
target_link_libraries (ai ai_driver)
