#pragma once

#include "SimulationParams.h"
#include "Vector4.h"
#include "Complex.h"
#include "BECFileFormat.h"
#include "BECFileWriter.h"
#include "NeighborFinder.h"
#include <vector>
#include <memory>

namespace BEC {

/**
 * Main simulation class for BEC on 4D hypersphere shell
 */
class HypersphereBEC {
public:
    explicit HypersphereBEC(const SimulationParams& params);
    ~HypersphereBEC();

    // Non-copyable
    HypersphereBEC(const HypersphereBEC&) = delete;
    HypersphereBEC& operator=(const HypersphereBEC&) = delete;

    /**
     * Run simulation
     * @param n_steps - Number of time steps
     * @param save_every - Save snapshot every N steps
     */
    void run(uint32_t n_steps, uint32_t save_every);

    /**
     * Single time step
     */
    void step();

    /**
     * Get current time
     */
    float get_time() const { return m_time; }

    /**
     * Get current step number
     */
    uint32_t get_step() const { return m_current_step; }

private:
    // Parameters
    SimulationParams m_params;

    // CPU data
    std::vector<Vector4> m_coords_cpu;      // 4D coordinates
    std::vector<Complex> m_psi_cpu;         // Wavefunction
    std::vector<int> m_neighbor_indices;    // Neighbor connectivity
    std::vector<float> m_neighbor_distances;
    size_t m_n_active;                      // Number of active points (including poles)

    // GPU data (CUDA pointers)
    void* m_coords_gpu;
    void* m_psi_gpu;
    void* m_psi_temp_gpu;  // Temporary for RK4
    void* m_k1_gpu, *m_k2_gpu, *m_k3_gpu, *m_k4_gpu;  // RK4 stages
    void* m_laplacian_gpu;
    void* m_rotation_term_gpu;
    void* m_neighbor_indices_gpu;
    void* m_neighbor_distances_gpu;

    // File writer
    std::unique_ptr<BECFileWriter> m_file_writer;

    // Simulation state
    float m_time;
    uint32_t m_current_step;

    // Initialization
    void initialize_geometry();
    void initialize_wavefunction();
    void initialize_gpu_memory();
    void setup_file_writer();

    // Physics (will call CUDA kernels or CPU fallback)
    void compute_laplacian();
    void compute_rotation_term();
    void evolve_gpe_rk4();

    // Analysis
    void compute_observables(SnapshotStatistics& stats);
    void detect_vortices(std::vector<VortexInfo>& vortices);

    // Snapshot export
    void save_snapshot();

    // GPU memory management
    void upload_to_gpu();
    void download_from_gpu();
    void free_gpu_memory();

    // Utilities
    void log_progress(uint32_t step, uint32_t total_steps);
};

} // namespace BEC
