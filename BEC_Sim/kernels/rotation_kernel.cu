#include "cuda_common.cuh"

/**
 * Compute rotation term: Ω L_z ψ
 * where L_z = w * ∂_x - x * ∂_w
 *
 * Approximation using finite differences with neighbors
 */
__global__ void compute_rotation_term_kernel(
    const cuDoubleComplex* psi,
    const float4_custom* coords,
    const int* neighbor_indices,
    const float* neighbor_distances,
    cuDoubleComplex* rotation_term,
    float omega,
    int n_active,
    int n_neighbors)
{
    int idx = get_global_thread_id();
    if (idx >= n_active) return;

    float4_custom pos = coords[idx];

    // Compute gradients ∂ψ/∂x and ∂ψ/∂w using neighbors
    cuDoubleComplex grad_x = make_cuDoubleComplex(0.0, 0.0);
    cuDoubleComplex grad_w = make_cuDoubleComplex(0.0, 0.0);

    cuDoubleComplex psi_center = psi[idx];

    double sum_weight_x = 0.0;
    double sum_weight_w = 0.0;

    for (int k = 0; k < n_neighbors; ++k) {
        int neighbor_idx = neighbor_indices[idx * n_neighbors + k];

        if (neighbor_idx < 0 || neighbor_idx >= n_active) continue;

        float4_custom neighbor_pos = coords[neighbor_idx];
        cuDoubleComplex psi_neighbor = psi[neighbor_idx];

        float dx = neighbor_pos.x - pos.x;
        float dw = neighbor_pos.w - pos.w;
        float dist = neighbor_distances[idx * n_neighbors + k];

        if (dist < 1e-6f) continue;

        // Inverse distance weighting
        float weight = 1.0f / (dist + 1e-6f);

        // Gradient in x direction
        cuDoubleComplex dpsi = complex_sub(psi_neighbor, psi_center);
        grad_x = complex_add(grad_x, complex_scale(dpsi, weight * dx));
        sum_weight_x += weight * dx * dx;

        // Gradient in w direction
        grad_w = complex_add(grad_w, complex_scale(dpsi, weight * dw));
        sum_weight_w += weight * dw * dw;
    }

    // Normalize gradients
    if (sum_weight_x > 1e-10) {
        grad_x = complex_scale(grad_x, 1.0 / sum_weight_x);
    }

    if (sum_weight_w > 1e-10) {
        grad_w = complex_scale(grad_w, 1.0 / sum_weight_w);
    }

    // L_z = w * ∂_x - x * ∂_w
    cuDoubleComplex Lz_psi = complex_sub(
        complex_scale(grad_x, pos.w),
        complex_scale(grad_w, pos.x)
    );

    // Multiply by -iΩ (rotation term)
    // -i * Lz_psi = -i * (a + ib) = b - ia
    cuDoubleComplex result = make_cuDoubleComplex(
        -omega * cuCimag(Lz_psi),
        omega * cuCreal(Lz_psi)
    );

    rotation_term[idx] = result;
}

// Host wrapper
extern "C" {

void launch_rotation_kernel(
    const cuDoubleComplex* psi,
    const float4_custom* coords,
    const int* neighbor_indices,
    const float* neighbor_distances,
    cuDoubleComplex* rotation_term,
    float omega,
    int n_active,
    int n_neighbors)
{
    int block_size = 256;
    int grid_size = (n_active + block_size - 1) / block_size;

    compute_rotation_term_kernel<<<grid_size, block_size>>>(
        psi, coords,
        neighbor_indices, neighbor_distances,
        rotation_term, omega,
        n_active, n_neighbors
    );

    CUDA_CHECK(cudaGetLastError());
}

} // extern "C"
