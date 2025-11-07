#include "cuda_common.cuh"

/**
 * Compute Laplacian using neighbor averaging
 * ∇²ψ ≈ (2/k) Σ (ψ_j - ψ) / d_j²
 */
__global__ void compute_laplacian_kernel(
    const cuDoubleComplex* psi,
    cuDoubleComplex* laplacian,
    const int* neighbor_indices,
    const float* neighbor_distances,
    int n_active,
    int n_neighbors)
{
    int idx = get_global_thread_id();
    if (idx >= n_active) return;

    cuDoubleComplex lap = make_cuDoubleComplex(0.0, 0.0);
    cuDoubleComplex psi_center = psi[idx];

    for (int k = 0; k < n_neighbors; ++k) {
        int neighbor_idx = neighbor_indices[idx * n_neighbors + k];

        if (neighbor_idx < 0 || neighbor_idx >= n_active) continue;

        float dist = neighbor_distances[idx * n_neighbors + k];

        if (dist < 1e-6f) continue;  // Skip if too close

        cuDoubleComplex psi_neighbor = psi[neighbor_idx];
        cuDoubleComplex diff = complex_sub(psi_neighbor, psi_center);

        // Weight: 2 / d²
        double weight = 2.0 / (dist * dist);

        lap = complex_add(lap, complex_scale(diff, weight));
    }

    // Average over neighbors
    lap = complex_scale(lap, 1.0 / n_neighbors);

    laplacian[idx] = lap;
}

// Host wrapper
extern "C" {

void launch_laplacian_kernel(
    const cuDoubleComplex* psi,
    cuDoubleComplex* laplacian,
    const int* neighbor_indices,
    const float* neighbor_distances,
    int n_active,
    int n_neighbors)
{
    int block_size = 256;
    int grid_size = (n_active + block_size - 1) / block_size;

    compute_laplacian_kernel<<<grid_size, block_size>>>(
        psi, laplacian,
        neighbor_indices, neighbor_distances,
        n_active, n_neighbors
    );

    CUDA_CHECK(cudaGetLastError());
}

} // extern "C"
