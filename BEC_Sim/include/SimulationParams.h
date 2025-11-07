#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <string>

// Define M_PI if not defined (for MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace BEC {

/**
 * Simulation parameters
 */
struct SimulationParams {
    // Physical parameters
    float R = 1000.0f;              // Hypersphere radius (healing lengths)
    float delta = 25.0f;            // Shell thickness (healing lengths)
    float thickness_ratio = 0.0f;   // If > 0, delta = R / thickness_ratio
    float g = 0.05f;                // Interaction strength
    float omega = 0.5f;             // Rotation frequency

    // Computational parameters
    uint32_t N = 64;                // Grid resolution parameter
    float dt = 0.01f;               // Time step
    uint32_t n_neighbors = 6;       // Neighbors for gradient calculation
    uint32_t random_seed = 42;      // RNG seed

    // Simulation control
    uint32_t n_steps = 10000;       // Total steps
    uint32_t save_every = 100;      // Save snapshot every N steps
    std::string output_file = "output.bec";

    // Initial condition
    float noise_amplitude = 0.01f;  // Initial noise amplitude

    // Analysis
    bool detect_vortices = true;
    bool detect_phonons = true;
    bool detect_rotons = true;

    // Apply thickness ratio if specified
    void apply_thickness_ratio() {
        if (thickness_ratio > 0.0f) {
            delta = R / thickness_ratio;
        }
    }

    // Estimate number of shell points
    uint32_t estimate_n_points() const {
        // Surface area of 3-sphere: 2π²R³
        // With thickness delta, volume ≈ 2π²R²·delta
        // Points spaced by ~(R/N), so total ≈ (2π²R²·delta) / (R/N)³
        float spacing = R / static_cast<float>(N);
        float shell_volume = 2.0f * M_PI * M_PI * R * R * delta;
        float point_volume = spacing * spacing * spacing;
        return static_cast<uint32_t>(shell_volume / point_volume);
    }
};

} // namespace BEC
