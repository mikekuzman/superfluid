cmake_minimum_required(VERSION 3.18)
project(BEC_Core)

# Core library sources
set(CORE_SOURCES
    src/BECFileWriter.cpp
    src/BECFileReader.cpp
)

set(CORE_HEADERS
    include/Vector4.h
    include/BECFileFormat.h
    include/BECFileWriter.h
    include/BECFileReader.h
    include/Projection.h
    include/Complex.h
    include/Statistics.h
)

# Create static library
add_library(BEC_Core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# Include directories
target_include_directories(BEC_Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link LZ4 if enabled
if(USE_LZ4)
    find_package(lz4 CONFIG)
    if(lz4_FOUND)
        target_link_libraries(BEC_Core PUBLIC lz4::lz4)
        target_compile_definitions(BEC_Core PUBLIC USE_LZ4)
    else()
        message(WARNING "LZ4 not found. Compression will be disabled (data will be stored uncompressed).")
    endif()
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(BEC_Core PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(UNIX)
    # Linux-specific settings
    target_link_libraries(BEC_Core PUBLIC pthread)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(BEC_Core PRIVATE /W3 /WX-)
else()
    target_compile_options(BEC_Core PRIVATE -Wall -Wextra)
endif()

# Install
install(TARGETS BEC_Core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${CORE_HEADERS}
    DESTINATION include/BEC
)
