cmake_minimum_required(VERSION 3.18)
project(BEC_Project LANGUAGES CXX)

# C++17 standard required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build options
option(BUILD_SIMULATION "Build simulation executable" ON)
option(BUILD_VISUALIZATION "Build visualization executable" ON)
option(USE_CUDA "Enable CUDA support" ON)
option(USE_LZ4 "Enable LZ4 compression" OFF)

# Find packages
if(USE_CUDA)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
endif()

# Subdirectories
add_subdirectory(BEC_Core)

if(BUILD_SIMULATION)
    add_subdirectory(BEC_Sim)
endif()

if(BUILD_VISUALIZATION)
    # DirectX 12 is Windows-only
    if(WIN32)
        add_subdirectory(BEC_Viz)
    else()
        message(WARNING "Visualization requires Windows and DirectX 12. Skipping BEC_Viz.")
    endif()
endif()

# Testing
add_subdirectory(tests)
