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
)

set(SIM_HEADERS
    include/HypersphereBEC.h
    include/ShellGenerator.h
    include/NeighborFinder.h
    include/VortexDetector.h
    include/SimulationParams.h
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

# Compiler flags (only for Release builds)
if(MSVC)
    target_compile_options(bec_sim PRIVATE /W3 /MP)
    target_compile_options(bec_sim PRIVATE "$<$<CONFIG:RELEASE>:/O2;/Oi;/GL;/fp:fast;/arch:AVX2>")
else()
    target_compile_options(bec_sim PRIVATE "$<$<CONFIG:RELEASE>:-O3;-march=native;-ffast-math>")
endif()

# CUDA compiler flags
if(USE_CUDA)
    target_compile_options(bec_sim PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>")
    target_compile_options(bec_sim PRIVATE "$<$<AND:$<COMPILE_LANGUAGE:CUDA>,$<CONFIG:RELEASE>>:-O3>")
endif()

# Install
install(TARGETS bec_sim
    RUNTIME DESTINATION bin
)
