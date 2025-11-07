#include "cuda_common.cuh"

/**
 * Compute GPE right-hand side: F(ψ) for time evolution
 * i∂ψ/∂t = F(ψ) = [-∇²/2 + g|ψ|² - Ω L_z]ψ
 *
 * Returns -i * F(ψ) for RK4 integration
 */
__global__ void gpe_rhs_kernel(
    const cuDoubleComplex* psi,
    const cuDoubleComplex* laplacian,
    const cuDoubleComplex* rotation_term,
    cuDoubleComplex* rhs,
    float g,
    int n_active)
{
    int idx = get_global_thread_id();
    if (idx >= n_active) return;

    cuDoubleComplex psi_val = psi[idx];
    cuDoubleComplex lap = laplacian[idx];
    cuDoubleComplex rot = rotation_term[idx];

    // Kinetic term: -∇²ψ/2
    cuDoubleComplex kinetic = complex_scale(lap, -0.5);

    // Interaction term: g|ψ|²ψ
    double density = complex_abs_sq(psi_val);
    cuDoubleComplex interaction = complex_scale(psi_val, g * density);

    // Total: F(ψ) = kinetic + interaction + rotation
    cuDoubleComplex F = complex_add(kinetic, interaction);
    F = complex_add(F, rot);

    // Multiply by -i: -i*F = -i*(a+ib) = b - ia
    cuDoubleComplex result = make_cuDoubleComplex(
        -cuCimag(F),
        cuCreal(F)
    );

    rhs[idx] = result;
}

/**
 * RK4 step: y_new = y + (k1 + 2*k2 + 2*k3 + k4) * dt/6
 */
__global__ void rk4_combine_kernel(
    const cuDoubleComplex* psi_old,
    const cuDoubleComplex* k1,
    const cuDoubleComplex* k2,
    const cuDoubleComplex* k3,
    const cuDoubleComplex* k4,
    cuDoubleComplex* psi_new,
    float dt,
    int n_active)
{
    int idx = get_global_thread_id();
    if (idx >= n_active) return;

    // Combine: dt/6 * (k1 + 2*k2 + 2*k3 + k4)
    cuDoubleComplex sum = k1[idx];
    sum = complex_add(sum, complex_scale(k2[idx], 2.0));
    sum = complex_add(sum, complex_scale(k3[idx], 2.0));
    sum = complex_add(sum, k4[idx]);

    sum = complex_scale(sum, dt / 6.0);

    psi_new[idx] = complex_add(psi_old[idx], sum);
}

/**
 * Helper: compute psi + dt*k
 */
__global__ void axpy_kernel(
    const cuDoubleComplex* psi,
    const cuDoubleComplex* k,
    cuDoubleComplex* result,
    float dt,
    int n_active)
{
    int idx = get_global_thread_id();
    if (idx >= n_active) return;

    result[idx] = complex_add(psi[idx], complex_scale(k[idx], dt));
}

/**
 * Initialize wavefunction: ψ = 1 + noise
 */
__global__ void initialize_wavefunction_kernel(
    cuDoubleComplex* psi,
    const float* noise_real,
    const float* noise_imag,
    float noise_amplitude,
    int n_active)
{
    int idx = get_global_thread_id();
    if (idx >= n_active) return;

    // Skip poles - set to 1
    if (idx == 0 || idx == 1) {
        psi[idx] = make_cuDoubleComplex(1.0, 0.0);
        return;
    }

    // Regular points: 1 + noise
    double real = 1.0 + noise_amplitude * noise_real[idx];
    double imag = noise_amplitude * noise_imag[idx];

    psi[idx] = make_cuDoubleComplex(real, imag);
}

// Host wrappers
extern "C" {

void launch_gpe_rhs_kernel(
    const cuDoubleComplex* psi,
    const cuDoubleComplex* laplacian,
    const cuDoubleComplex* rotation_term,
    cuDoubleComplex* rhs,
    float g,
    int n_active)
{
    int block_size = 256;
    int grid_size = (n_active + block_size - 1) / block_size;

    gpe_rhs_kernel<<<grid_size, block_size>>>(
        psi, laplacian, rotation_term, rhs, g, n_active
    );

    CUDA_CHECK(cudaGetLastError());
}

void launch_rk4_combine_kernel(
    const cuDoubleComplex* psi_old,
    const cuDoubleComplex* k1,
    const cuDoubleComplex* k2,
    const cuDoubleComplex* k3,
    const cuDoubleComplex* k4,
    cuDoubleComplex* psi_new,
    float dt,
    int n_active)
{
    int block_size = 256;
    int grid_size = (n_active + block_size - 1) / block_size;

    rk4_combine_kernel<<<grid_size, block_size>>>(
        psi_old, k1, k2, k3, k4, psi_new, dt, n_active
    );

    CUDA_CHECK(cudaGetLastError());
}

void launch_axpy_kernel(
    const cuDoubleComplex* psi,
    const cuDoubleComplex* k,
    cuDoubleComplex* result,
    float dt,
    int n_active)
{
    int block_size = 256;
    int grid_size = (n_active + block_size - 1) / block_size;

    axpy_kernel<<<grid_size, block_size>>>(
        psi, k, result, dt, n_active
    );

    CUDA_CHECK(cudaGetLastError());
}

void launch_initialize_wavefunction_kernel(
    cuDoubleComplex* psi,
    const float* noise_real,
    const float* noise_imag,
    float noise_amplitude,
    int n_active)
{
    int block_size = 256;
    int grid_size = (n_active + block_size - 1) / block_size;

    initialize_wavefunction_kernel<<<grid_size, block_size>>>(
        psi, noise_real, noise_imag, noise_amplitude, n_active
    );

    CUDA_CHECK(cudaGetLastError());
}

} // extern "C"
