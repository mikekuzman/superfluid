#include "cuda_common.cuh"

/**
 * Compute gradient of wavefunction using weighted least squares
 * ∂ψ/∂x_i ≈ Σ w_j (ψ_j - ψ)(x_j - x_i) / Σ w_j (x_j - x_i)²
 */
__global__ void compute_gradient_kernel(
    const cuDoubleComplex* psi,
    const float4_custom* coords,
    const int* neighbor_indices,
    const float* neighbor_distances,
    cuDoubleComplex* grad_w,
    cuDoubleComplex* grad_x,
    cuDoubleComplex* grad_y,
    cuDoubleComplex* grad_z,
    int n_active,
    int n_neighbors)
{
    int idx = get_global_thread_id();
    if (idx >= n_active) return;

    float4_custom pos = coords[idx];
    cuDoubleComplex psi_center = psi[idx];

    cuDoubleComplex gw = make_cuDoubleComplex(0.0, 0.0);
    cuDoubleComplex gx = make_cuDoubleComplex(0.0, 0.0);
    cuDoubleComplex gy = make_cuDoubleComplex(0.0, 0.0);
    cuDoubleComplex gz = make_cuDoubleComplex(0.0, 0.0);

    double sum_weight_w = 0.0;
    double sum_weight_x = 0.0;
    double sum_weight_y = 0.0;
    double sum_weight_z = 0.0;

    for (int k = 0; k < n_neighbors; ++k) {
        int neighbor_idx = neighbor_indices[idx * n_neighbors + k];

        if (neighbor_idx < 0 || neighbor_idx >= n_active) continue;

        float4_custom neighbor_pos = coords[neighbor_idx];
        cuDoubleComplex psi_neighbor = psi[neighbor_idx];

        float dw = neighbor_pos.w - pos.w;
        float dx = neighbor_pos.x - pos.x;
        float dy = neighbor_pos.y - pos.y;
        float dz = neighbor_pos.z - pos.z;

        float dist = neighbor_distances[idx * n_neighbors + k];

        if (dist < 1e-6f) continue;

        // Inverse distance weighting
        float weight = 1.0f / (dist + 1e-6f);

        cuDoubleComplex dpsi = complex_sub(psi_neighbor, psi_center);

        // Accumulate weighted gradients
        gw = complex_add(gw, complex_scale(dpsi, weight * dw));
        gx = complex_add(gx, complex_scale(dpsi, weight * dx));
        gy = complex_add(gy, complex_scale(dpsi, weight * dy));
        gz = complex_add(gz, complex_scale(dpsi, weight * dz));

        sum_weight_w += weight * dw * dw;
        sum_weight_x += weight * dx * dx;
        sum_weight_y += weight * dy * dy;
        sum_weight_z += weight * dz * dz;
    }

    // Normalize
    if (sum_weight_w > 1e-10) gw = complex_scale(gw, 1.0 / sum_weight_w);
    if (sum_weight_x > 1e-10) gx = complex_scale(gx, 1.0 / sum_weight_x);
    if (sum_weight_y > 1e-10) gy = complex_scale(gy, 1.0 / sum_weight_y);
    if (sum_weight_z > 1e-10) gz = complex_scale(gz, 1.0 / sum_weight_z);

    grad_w[idx] = gw;
    grad_x[idx] = gx;
    grad_y[idx] = gy;
    grad_z[idx] = gz;
}

// Host wrapper
extern "C" {

void launch_gradient_kernel(
    const cuDoubleComplex* psi,
    const float4_custom* coords,
    const int* neighbor_indices,
    const float* neighbor_distances,
    cuDoubleComplex* grad_w,
    cuDoubleComplex* grad_x,
    cuDoubleComplex* grad_y,
    cuDoubleComplex* grad_z,
    int n_active,
    int n_neighbors)
{
    int block_size = 256;
    int grid_size = (n_active + block_size - 1) / block_size;

    compute_gradient_kernel<<<grid_size, block_size>>>(
        psi, coords,
        neighbor_indices, neighbor_distances,
        grad_w, grad_x, grad_y, grad_z,
        n_active, n_neighbors
    );

    CUDA_CHECK(cudaGetLastError());
}

} // extern "C"
