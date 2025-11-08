#include "HypersphereBEC.h"
#include "ShellGenerator.h"
#include "VortexDetector.h"
#include "Statistics.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <random>

#ifdef USE_CUDA
#include <cuda_runtime.h>
#include <cuComplex.h>
#include <curand.h>

// Custom float4 for compatibility
struct float4_custom {
    float w, x, y, z;
};

// CUDA kernel declarations
extern "C" {
    void launch_laplacian_kernel(
        const cuDoubleComplex*, cuDoubleComplex*,
        const int*, const float*, int, int);

    void launch_rotation_kernel(
        const cuDoubleComplex*, const float4_custom*,
        const int*, const float*, cuDoubleComplex*,
        float, int, int);

    void launch_gpe_rhs_kernel(
        const cuDoubleComplex*, const cuDoubleComplex*,
        const cuDoubleComplex*, cuDoubleComplex*,
        float, int);

    void launch_rk4_combine_kernel(
        const cuDoubleComplex*, const cuDoubleComplex*,
        const cuDoubleComplex*, const cuDoubleComplex*,
        const cuDoubleComplex*, cuDoubleComplex*,
        float, int);

    void launch_axpy_kernel(
        const cuDoubleComplex*, const cuDoubleComplex*,
        cuDoubleComplex*, float, int);

    void launch_initialize_wavefunction_kernel(
        cuDoubleComplex*, const float*, const float*,
        float, int);
}

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error: " << cudaGetErrorString(err) << std::endl; \
            exit(1); \
        } \
    } while(0)

#else
// CPU fallback (stub for now)
#define NO_CUDA
#endif

namespace BEC {

HypersphereBEC::HypersphereBEC(const SimulationParams& params)
    : m_params(params)
    , m_n_active(0)
    , m_time(0.0f)
    , m_current_step(0)
{
    // Apply thickness ratio
    m_params.apply_thickness_ratio();

    std::cout << "=== Initializing 4D BEC Simulation ===" << std::endl;
    std::cout << "Parameters:" << std::endl;
    std::cout << "  R = " << m_params.R << " (healing lengths)" << std::endl;
    std::cout << "  delta = " << m_params.delta << " (healing lengths)" << std::endl;
    std::cout << "  g = " << m_params.g << std::endl;
    std::cout << "  omega = " << m_params.omega << std::endl;
    std::cout << "  dt = " << m_params.dt << std::endl;
    std::cout << "  n_neighbors = " << m_params.n_neighbors << std::endl;
    std::cout << "  random_seed = " << m_params.random_seed << std::endl;

    // Initialize geometry
    initialize_geometry();

    // Initialize wavefunction
    initialize_wavefunction();

#ifdef USE_CUDA
    // Initialize GPU
    initialize_gpu_memory();
#endif

    // Setup file writer
    setup_file_writer();

    std::cout << "Initialization complete!" << std::endl;
    std::cout << "========================================" << std::endl;
}

HypersphereBEC::~HypersphereBEC() {
#ifdef USE_CUDA
    free_gpu_memory();
#endif

    if (m_file_writer) {
        m_file_writer->finalize();
    }
}

void HypersphereBEC::initialize_geometry() {
    std::cout << "\n--- Initializing Geometry ---" << std::endl;

    // Generate shell points
    ShellGenerator::generate_shell_points(m_params, m_coords_cpu);
    m_n_active = m_coords_cpu.size();

    std::cout << "Generated " << m_n_active << " points (including 2 poles)" << std::endl;

    // Build neighbor tree
    NeighborFinder finder;
    finder.build(m_coords_cpu);

    // Find all neighbors
    finder.find_all_neighbors(
        m_coords_cpu,
        m_params.n_neighbors,
        m_neighbor_indices,
        m_neighbor_distances
    );

    std::cout << "Neighbor finding complete." << std::endl;
}

void HypersphereBEC::initialize_wavefunction() {
    std::cout << "\n--- Initializing Wavefunction ---" << std::endl;

    m_psi_cpu.resize(m_n_active);

    // Random number generator
    std::mt19937 rng(m_params.random_seed);
    std::normal_distribution<double> dist(0.0, 1.0);

    // Initialize with small noise around ψ = 1
    for (size_t i = 0; i < m_n_active; ++i) {
        if (i == 0 || i == 1) {
            // Poles: exactly 1
            m_psi_cpu[i] = Complex(1.0, 0.0);
        } else {
            // Regular points: 1 + noise
            double real = 1.0 + m_params.noise_amplitude * dist(rng);
            double imag = m_params.noise_amplitude * dist(rng);
            m_psi_cpu[i] = Complex(real, imag);
        }
    }

    std::cout << "Wavefunction initialized with noise amplitude = " << m_params.noise_amplitude << std::endl;
}

void HypersphereBEC::initialize_gpu_memory() {
#ifdef USE_CUDA
    std::cout << "\n--- Initializing GPU Memory ---" << std::endl;

    size_t n = m_n_active;
    size_t n_neighbors = m_params.n_neighbors;

    // Allocate GPU memory
    CUDA_CHECK(cudaMalloc(&m_coords_gpu, n * sizeof(float4_custom)));
    CUDA_CHECK(cudaMalloc(&m_psi_gpu, n * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMalloc(&m_psi_temp_gpu, n * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMalloc(&m_k1_gpu, n * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMalloc(&m_k2_gpu, n * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMalloc(&m_k3_gpu, n * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMalloc(&m_k4_gpu, n * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMalloc(&m_laplacian_gpu, n * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMalloc(&m_rotation_term_gpu, n * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMalloc(&m_neighbor_indices_gpu, n * n_neighbors * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&m_neighbor_distances_gpu, n * n_neighbors * sizeof(float)));

    // Copy data to GPU
    upload_to_gpu();

    std::cout << "GPU memory allocated and data uploaded." << std::endl;
    std::cout << "  Memory usage: ~" << (n * sizeof(cuDoubleComplex) * 9) / (1024*1024) << " MB" << std::endl;
#else
    std::cout << "GPU support disabled (USE_CUDA not defined)" << std::endl;
#endif
}

void HypersphereBEC::setup_file_writer() {
    // Create header
    BECHeader header;
    header.R = m_params.R;
    header.delta = m_params.delta;
    header.thickness_ratio = m_params.thickness_ratio;
    header.g = m_params.g;
    header.omega = m_params.omega;
    header.N = m_params.N;
    header.dt = m_params.dt;
    header.n_neighbors = m_params.n_neighbors;
    header.random_seed = m_params.random_seed;
    header.n_points_per_snapshot = m_n_active;
    header.north_pole_4d = Vector4(0, 0, 0, m_params.R);
    header.south_pole_4d = Vector4(0, 0, 0, -m_params.R);

    // Create file writer
    m_file_writer = std::make_unique<BECFileWriter>(m_params.output_file, header);

    std::cout << "\nOutput file: " << m_params.output_file << std::endl;
}

void HypersphereBEC::upload_to_gpu() {
#ifdef USE_CUDA
    // Convert coordinates
    std::vector<float4_custom> coords_gpu(m_n_active);
    for (size_t i = 0; i < m_n_active; ++i) {
        coords_gpu[i].w = m_coords_cpu[i].w;
        coords_gpu[i].x = m_coords_cpu[i].x;
        coords_gpu[i].y = m_coords_cpu[i].y;
        coords_gpu[i].z = m_coords_cpu[i].z;
    }

    CUDA_CHECK(cudaMemcpy(m_coords_gpu, coords_gpu.data(),
                          m_n_active * sizeof(float4_custom), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_psi_gpu, m_psi_cpu.data(),
                          m_n_active * sizeof(cuDoubleComplex), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_neighbor_indices_gpu, m_neighbor_indices.data(),
                          m_neighbor_indices.size() * sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_neighbor_distances_gpu, m_neighbor_distances.data(),
                          m_neighbor_distances.size() * sizeof(float), cudaMemcpyHostToDevice));
#endif
}

void HypersphereBEC::download_from_gpu() {
#ifdef USE_CUDA
    CUDA_CHECK(cudaMemcpy(m_psi_cpu.data(), m_psi_gpu,
                          m_n_active * sizeof(cuDoubleComplex), cudaMemcpyDeviceToHost));
#endif
}

void HypersphereBEC::free_gpu_memory() {
#ifdef USE_CUDA
    if (m_coords_gpu) cudaFree(m_coords_gpu);
    if (m_psi_gpu) cudaFree(m_psi_gpu);
    if (m_psi_temp_gpu) cudaFree(m_psi_temp_gpu);
    if (m_k1_gpu) cudaFree(m_k1_gpu);
    if (m_k2_gpu) cudaFree(m_k2_gpu);
    if (m_k3_gpu) cudaFree(m_k3_gpu);
    if (m_k4_gpu) cudaFree(m_k4_gpu);
    if (m_laplacian_gpu) cudaFree(m_laplacian_gpu);
    if (m_rotation_term_gpu) cudaFree(m_rotation_term_gpu);
    if (m_neighbor_indices_gpu) cudaFree(m_neighbor_indices_gpu);
    if (m_neighbor_distances_gpu) cudaFree(m_neighbor_distances_gpu);
#endif
}

void HypersphereBEC::run(uint32_t n_steps, uint32_t save_every) {
    std::cout << "\n=== Starting Simulation ===" << std::endl;
    std::cout << "Total steps: " << n_steps << std::endl;
    std::cout << "Save every: " << save_every << " steps" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (uint32_t i = 0; i < n_steps; ++i) {
        m_current_step = i;

        // Time step
        step();

        // Save snapshot
        if (i % save_every == 0) {
            save_snapshot();
            log_progress(i, n_steps);
        }
    }

    // Final snapshot
    save_snapshot();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\n=== Simulation Complete ===" << std::endl;
    std::cout << "Total time: " << duration.count() / 1000.0 << " seconds" << std::endl;
    std::cout << "Average speed: " << (n_steps * 1000.0 / duration.count()) << " steps/sec" << std::endl;

    // Finalize file
    if (m_file_writer) {
        m_file_writer->finalize();
    }
}

void HypersphereBEC::step() {
#ifdef USE_CUDA
    evolve_gpe_rk4();
#else
    // CPU fallback (simplified)
    std::cout << "Warning: Running without CUDA. Physics not implemented on CPU." << std::endl;
#endif

    m_time += m_params.dt;
}

void HypersphereBEC::evolve_gpe_rk4() {
#ifdef USE_CUDA
    // RK4 integration
    int n = m_n_active;
    int nn = m_params.n_neighbors;
    float dt = m_params.dt;
    float g = m_params.g;
    float omega = m_params.omega;

    // k1 = F(psi)
    // Compute laplacian and rotation for current psi
    launch_laplacian_kernel(
        (cuDoubleComplex*)m_psi_gpu,
        (cuDoubleComplex*)m_laplacian_gpu,
        (int*)m_neighbor_indices_gpu,
        (float*)m_neighbor_distances_gpu,
        m_n_active,
        m_params.n_neighbors
    );
    launch_rotation_kernel(
        (cuDoubleComplex*)m_psi_gpu,
        (float4_custom*)m_coords_gpu,
        (int*)m_neighbor_indices_gpu,
        (float*)m_neighbor_distances_gpu,
        (cuDoubleComplex*)m_rotation_term_gpu,
        m_params.omega,
        m_n_active,
        m_params.n_neighbors
    );
    launch_gpe_rhs_kernel((cuDoubleComplex*)m_psi_gpu, (cuDoubleComplex*)m_laplacian_gpu,
                          (cuDoubleComplex*)m_rotation_term_gpu, (cuDoubleComplex*)m_k1_gpu, g, n);

    // k2 = F(psi + dt*k1/2)
    launch_axpy_kernel((cuDoubleComplex*)m_psi_gpu, (cuDoubleComplex*)m_k1_gpu,
                      (cuDoubleComplex*)m_psi_temp_gpu, dt/2, n);
    // Recompute laplacian and rotation for psi_temp
    launch_laplacian_kernel(
        (cuDoubleComplex*)m_psi_temp_gpu,
        (cuDoubleComplex*)m_laplacian_gpu,
        (int*)m_neighbor_indices_gpu,
        (float*)m_neighbor_distances_gpu,
        m_n_active,
        m_params.n_neighbors
    );
    launch_rotation_kernel(
        (cuDoubleComplex*)m_psi_temp_gpu,
        (float4_custom*)m_coords_gpu,
        (int*)m_neighbor_indices_gpu,
        (float*)m_neighbor_distances_gpu,
        (cuDoubleComplex*)m_rotation_term_gpu,
        m_params.omega,
        m_n_active,
        m_params.n_neighbors
    );
    launch_gpe_rhs_kernel((cuDoubleComplex*)m_psi_temp_gpu, (cuDoubleComplex*)m_laplacian_gpu,
                          (cuDoubleComplex*)m_rotation_term_gpu, (cuDoubleComplex*)m_k2_gpu, g, n);

    // k3 = F(psi + dt*k2/2)
    launch_axpy_kernel((cuDoubleComplex*)m_psi_gpu, (cuDoubleComplex*)m_k2_gpu,
                      (cuDoubleComplex*)m_psi_temp_gpu, dt/2, n);
    // Recompute laplacian and rotation for psi_temp
    launch_laplacian_kernel(
        (cuDoubleComplex*)m_psi_temp_gpu,
        (cuDoubleComplex*)m_laplacian_gpu,
        (int*)m_neighbor_indices_gpu,
        (float*)m_neighbor_distances_gpu,
        m_n_active,
        m_params.n_neighbors
    );
    launch_rotation_kernel(
        (cuDoubleComplex*)m_psi_temp_gpu,
        (float4_custom*)m_coords_gpu,
        (int*)m_neighbor_indices_gpu,
        (float*)m_neighbor_distances_gpu,
        (cuDoubleComplex*)m_rotation_term_gpu,
        m_params.omega,
        m_n_active,
        m_params.n_neighbors
    );
    launch_gpe_rhs_kernel((cuDoubleComplex*)m_psi_temp_gpu, (cuDoubleComplex*)m_laplacian_gpu,
                          (cuDoubleComplex*)m_rotation_term_gpu, (cuDoubleComplex*)m_k3_gpu, g, n);

    // k4 = F(psi + dt*k3)
    launch_axpy_kernel((cuDoubleComplex*)m_psi_gpu, (cuDoubleComplex*)m_k3_gpu,
                      (cuDoubleComplex*)m_psi_temp_gpu, dt, n);
    // Recompute laplacian and rotation for psi_temp
    launch_laplacian_kernel(
        (cuDoubleComplex*)m_psi_temp_gpu,
        (cuDoubleComplex*)m_laplacian_gpu,
        (int*)m_neighbor_indices_gpu,
        (float*)m_neighbor_distances_gpu,
        m_n_active,
        m_params.n_neighbors
    );
    launch_rotation_kernel(
        (cuDoubleComplex*)m_psi_temp_gpu,
        (float4_custom*)m_coords_gpu,
        (int*)m_neighbor_indices_gpu,
        (float*)m_neighbor_distances_gpu,
        (cuDoubleComplex*)m_rotation_term_gpu,
        m_params.omega,
        m_n_active,
        m_params.n_neighbors
    );
    launch_gpe_rhs_kernel((cuDoubleComplex*)m_psi_temp_gpu, (cuDoubleComplex*)m_laplacian_gpu,
                          (cuDoubleComplex*)m_rotation_term_gpu, (cuDoubleComplex*)m_k4_gpu, g, n);

    // psi_new = psi + (k1 + 2*k2 + 2*k3 + k4) * dt/6
    launch_rk4_combine_kernel((cuDoubleComplex*)m_psi_gpu,
                              (cuDoubleComplex*)m_k1_gpu, (cuDoubleComplex*)m_k2_gpu,
                              (cuDoubleComplex*)m_k3_gpu, (cuDoubleComplex*)m_k4_gpu,
                              (cuDoubleComplex*)m_psi_gpu, dt, n);

    CUDA_CHECK(cudaDeviceSynchronize());
#endif
}

void HypersphereBEC::compute_laplacian() {
    // No longer used - RK4 computes laplacian inline for each stage
}

void HypersphereBEC::compute_rotation_term() {
    // No longer used - RK4 computes rotation inline for each stage
}

void HypersphereBEC::save_snapshot() {
    // Download from GPU
    download_from_gpu();

    // Compute statistics
    SnapshotStatistics stats;
    compute_observables(stats);

    // Detect vortices
    std::vector<VortexInfo> vortices;
    if (m_params.detect_vortices) {
        detect_vortices(vortices);
    }

    stats.n_vortices = vortices.size();
    stats.time = m_time;

    // Quantize point data
    std::vector<PointData> point_data(m_n_active);

    // Calculate bounds for quantization
    float R = m_params.R;
    float delta = m_params.delta;
    float coord_min = -(R + delta);
    float coord_max = R + delta;

    for (size_t i = 0; i < m_n_active; ++i) {
        // Quantize coordinates
        point_data[i].coord[0] = Quantization::quantize_coord(m_coords_cpu[i].w, coord_min, coord_max);
        point_data[i].coord[1] = Quantization::quantize_coord(m_coords_cpu[i].x, coord_min, coord_max);
        point_data[i].coord[2] = Quantization::quantize_coord(m_coords_cpu[i].y, coord_min, coord_max);
        point_data[i].coord[3] = Quantization::quantize_coord(m_coords_cpu[i].z, coord_min, coord_max);

        // Density and phase
        double density = ComplexOps::abs_squared(m_psi_cpu[i]);
        double phase = ComplexOps::phase(m_psi_cpu[i]);

        point_data[i].density = Quantization::float_to_float16(density);
        point_data[i].phase = Quantization::quantize_phase(phase);

        // Velocity (simplified: zero for now)
        for (int j = 0; j < 4; ++j) {
            point_data[i].velocity[j] = 0;
        }
    }

    // Write to file
    if (m_file_writer) {
        m_file_writer->add_snapshot(point_data, stats, vortices);
    }
}

void HypersphereBEC::compute_observables(SnapshotStatistics& stats) {
    std::vector<double> densities;
    densities.reserve(m_n_active - 2);  // Exclude poles

    for (size_t i = 2; i < m_n_active; ++i) {
        double density = ComplexOps::abs_squared(m_psi_cpu[i]);
        densities.push_back(density);
    }

    auto density_stats = Statistics::calculate_all(densities);

    stats.density_min = density_stats.min_val;
    stats.density_max = density_stats.max_val;
    stats.density_mean = density_stats.mean_val;
    stats.density_std = density_stats.std_val;
    stats.density_p5 = density_stats.p5;
    stats.density_p95 = density_stats.p95;
}

void HypersphereBEC::detect_vortices(std::vector<VortexInfo>& vortices) {
    vortices = VortexDetector::detect_vortices(
        m_coords_cpu,
        m_psi_cpu,
        m_neighbor_indices,
        m_params.n_neighbors
    );
}

void HypersphereBEC::log_progress(uint32_t step, uint32_t total_steps) {
    float percent = 100.0f * step / total_steps;

    std::cout << "Step " << step << " / " << total_steps
              << " (" << percent << "%)"
              << " | t = " << m_time
              << " | Snapshots saved: " << m_file_writer->get_snapshot_count()
              << std::endl;
}

} // namespace BEC
