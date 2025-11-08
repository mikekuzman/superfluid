cmake_minimum_required(VERSION 3.18)
project(BEC_Sim LANGUAGES CXX)

if(USE_CUDA)
    enable_language(CUDA)
endif()

# Simulation sources
set(SIM_SOURCES
    src/main.cpp
    src/HypersphereBEC.cpp
    src/ShellGenerator.cpp
    src/NeighborFinder.cpp
    src/VortexDetector.cpp
    src/SpatialResampler.cpp
)

set(SIM_HEADERS
    include/HypersphereBEC.h
    include/ShellGenerator.h
    include/NeighborFinder.h
    include/VortexDetector.h
    include/SimulationParams.h
    include/SpatialResampler.h
)

# CUDA kernels
if(USE_CUDA)
    set(CUDA_SOURCES
        kernels/laplacian_kernel.cu
        kernels/gpe_evolution_kernel.cu
        kernels/rotation_kernel.cu
        kernels/gradient_kernel.cu
    )
endif()

# Create executable
if(USE_CUDA)
    add_executable(bec_sim ${SIM_SOURCES} ${SIM_HEADERS} ${CUDA_SOURCES})
    set_target_properties(bec_sim PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

    # CUDA settings
    set_target_properties(bec_sim PROPERTIES
        CUDA_ARCHITECTURES "61;75;86"  # SM 6.1 (GTX 1070), 7.5, 8.6
    )

    # Define USE_CUDA preprocessor macro so C++ code knows CUDA is available
    target_compile_definitions(bec_sim PRIVATE USE_CUDA)

    target_link_libraries(bec_sim PRIVATE CUDA::cudart CUDA::curand)
else()
    add_executable(bec_sim ${SIM_SOURCES} ${SIM_HEADERS})
    target_compile_definitions(bec_sim PRIVATE NO_CUDA)
endif()

# Include directories
target_include_directories(bec_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link core library
target_link_libraries(bec_sim PRIVATE BEC_Core)

# Platform-specific
if(UNIX)
    target_link_libraries(bec_sim PRIVATE m pthread)
endif()

# Compiler flags
if(MSVC)
    # C++ flags only (not for CUDA files)
    target_compile_options(bec_sim PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/W3;/MP>")
    # Release optimization for C++ only
    target_compile_options(bec_sim PRIVATE "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:RELEASE>>:/O2;/Oi;/GL;/fp:fast;/arch:AVX2>")
else()
    # GCC/Clang flags
    target_compile_options(bec_sim PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:RELEASE>:-O3;-march=native;-ffast-math>>")
endif()

# CUDA compiler flags (separate from C++ flags)
if(USE_CUDA)
    # Always use fast math for CUDA
    target_compile_options(bec_sim PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>")
    # Optimization for CUDA in Release builds
    target_compile_options(bec_sim PRIVATE "$<$<AND:$<COMPILE_LANGUAGE:CUDA>,$<CONFIG:RELEASE>>:-O3>")
    # Suppress deprecated GPU target warning
    target_compile_options(bec_sim PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:-Wno-deprecated-gpu-targets>")
endif()

# BEC file inspector tool
add_executable(inspect_bec src/inspect_bec.cpp)
target_include_directories(inspect_bec PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(inspect_bec PRIVATE BEC_Core)

# Parameter sweep tool
set(SWEEP_SOURCES
    src/bec_sweep.cpp
    src/HypersphereBEC.cpp
    src/ShellGenerator.cpp
    src/NeighborFinder.cpp
    src/VortexDetector.cpp
    src/SpatialResampler.cpp
)

if(USE_CUDA)
    add_executable(bec_sweep ${SWEEP_SOURCES} ${SIM_HEADERS} ${CUDA_SOURCES})
    set_target_properties(bec_sweep PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
    set_target_properties(bec_sweep PROPERTIES CUDA_ARCHITECTURES "61;75;86")
    target_compile_definitions(bec_sweep PRIVATE USE_CUDA)
    target_link_libraries(bec_sweep PRIVATE CUDA::cudart CUDA::curand)

    # CUDA compiler flags
    target_compile_options(bec_sweep PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>")
    target_compile_options(bec_sweep PRIVATE "$<$<AND:$<COMPILE_LANGUAGE:CUDA>,$<CONFIG:RELEASE>>:-O3>")
    target_compile_options(bec_sweep PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:-Wno-deprecated-gpu-targets>")
else()
    add_executable(bec_sweep ${SWEEP_SOURCES} ${SIM_HEADERS})
    target_compile_definitions(bec_sweep PRIVATE NO_CUDA)
endif()

target_include_directories(bec_sweep PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bec_sweep PRIVATE BEC_Core)

if(UNIX)
    target_link_libraries(bec_sweep PRIVATE m pthread)
endif()

# Compiler flags for bec_sweep
if(MSVC)
    target_compile_options(bec_sweep PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/W3;/MP>")
    target_compile_options(bec_sweep PRIVATE "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:RELEASE>>:/O2;/Oi;/GL;/fp:fast;/arch:AVX2>")
else()
    target_compile_options(bec_sweep PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:RELEASE>:-O3;-march=native;-ffast-math>>")
endif()

# Install
install(TARGETS bec_sim inspect_bec bec_sweep
    RUNTIME DESTINATION bin
)
