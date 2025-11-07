cmake_minimum_required(VERSION 3.18)
project(BEC_Tests)

# Simple test for now
add_executable(test_file_io
    test_file_io.cpp
)

target_link_libraries(test_file_io PRIVATE BEC_Core)

target_include_directories(test_file_io PRIVATE
    ${CMAKE_SOURCE_DIR}/BEC_Core/include
)

# Add test
enable_testing()
add_test(NAME FileIOTest COMMAND test_file_io)
