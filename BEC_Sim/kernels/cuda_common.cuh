#pragma once

#include <cuda_runtime.h>
#include <cuComplex.h>
#include <cstdio>

// CUDA error checking macro
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", \
                    __FILE__, __LINE__, cudaGetErrorString(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

// 4D vector for CUDA
struct float4_custom {
    float w, x, y, z;

    __host__ __device__ float4_custom() : w(0), x(0), y(0), z(0) {}
    __host__ __device__ float4_custom(float w_, float x_, float y_, float z_)
        : w(w_), x(x_), y(y_), z(z_) {}
};

// Complex number operations
__device__ inline cuDoubleComplex complex_add(cuDoubleComplex a, cuDoubleComplex b) {
    return cuCadd(a, b);
}

__device__ inline cuDoubleComplex complex_sub(cuDoubleComplex a, cuDoubleComplex b) {
    return cuCsub(a, b);
}

__device__ inline cuDoubleComplex complex_mul(cuDoubleComplex a, cuDoubleComplex b) {
    return cuCmul(a, b);
}

__device__ inline cuDoubleComplex complex_scale(cuDoubleComplex a, double s) {
    return make_cuDoubleComplex(cuCreal(a) * s, cuCimag(a) * s);
}

__device__ inline double complex_abs_sq(cuDoubleComplex a) {
    return cuCreal(a) * cuCreal(a) + cuCimag(a) * cuCimag(a);
}

__device__ inline double complex_abs(cuDoubleComplex a) {
    return cuCabs(a);
}

__device__ inline cuDoubleComplex complex_conj(cuDoubleComplex a) {
    return cuConj(a);
}

// Thread/block helpers
__device__ inline int get_global_thread_id() {
    return blockIdx.x * blockDim.x + threadIdx.x;
}

__device__ inline int get_total_threads() {
    return gridDim.x * blockDim.x;
}
