#include "VortexDetector.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <algorithm>

// Define M_PI if not defined (for MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace BEC {

std::vector<VortexInfo> VortexDetector::detect_vortices(
    const std::vector<Vector4>& coords,
    const std::vector<Complex>& psi,
    const std::vector<int>& neighbor_indices,
    size_t n_neighbors)
{
    std::vector<VortexInfo> vortices;

    // Find potential vortex cores (low density points)
    std::vector<size_t> cores = find_vortex_cores(psi, 0.5f);

    std::cout << "Found " << cores.size() << " potential vortex cores" << std::endl;

    // For each core, calculate winding number
    std::vector<int> winding_numbers;
    for (size_t core_idx : cores) {
        // Skip poles (indices 0 and 1)
        if (core_idx == 0 || core_idx == 1) continue;

        int winding = calculate_winding_number(
            core_idx,
            neighbor_indices,
            n_neighbors,
            psi
        );

        winding_numbers.push_back(winding);

        if (std::abs(winding) > 0) {
            VortexInfo vortex;
            vortex.position_4d = coords[core_idx];
            vortex.quantum_number = winding;
            vortex.strength = 2.0f * M_PI * winding;
            vortices.push_back(vortex);
        }
    }

    // Debug: show winding number statistics
    if (!winding_numbers.empty()) {
        int non_zero = 0;
        for (int w : winding_numbers) {
            if (w != 0) non_zero++;
        }
        std::cout << "[DEBUG] Winding numbers: " << non_zero << " non-zero out of "
                  << winding_numbers.size() << " cores" << std::endl;

        // Show first few
        size_t show_count = std::min(static_cast<size_t>(5), winding_numbers.size());
        for (size_t i = 0; i < show_count; ++i) {
            std::cout << "  Core " << i << ": winding = " << winding_numbers[i] << std::endl;
        }
    }

    std::cout << "Detected " << vortices.size() << " vortices" << std::endl;

    return vortices;
}

std::vector<size_t> VortexDetector::find_vortex_cores(
    const std::vector<Complex>& psi,
    float density_threshold)
{
    std::vector<size_t> cores;

    // Calculate density statistics for debugging
    std::vector<double> densities;
    for (size_t i = 2; i < psi.size(); ++i) {
        densities.push_back(ComplexOps::abs_squared(psi[i]));
    }

    std::sort(densities.begin(), densities.end());
    double min_density = densities.front();
    double max_density = densities.back();
    double median_density = densities[densities.size() / 2];

    // Use adaptive threshold: fraction of median density
    double adaptive_threshold = 0.3 * median_density;
    double actual_threshold = std::max(static_cast<double>(density_threshold), adaptive_threshold);

    std::cout << "[DEBUG] Vortex core detection:" << std::endl;
    std::cout << "  Density range: [" << min_density << ", " << max_density << "]" << std::endl;
    std::cout << "  Median density: " << median_density << std::endl;
    std::cout << "  Original threshold: " << density_threshold << std::endl;
    std::cout << "  Adaptive threshold (0.3*median): " << adaptive_threshold << std::endl;
    std::cout << "  Using threshold: " << actual_threshold << std::endl;

    for (size_t i = 2; i < psi.size(); ++i) {  // Skip poles
        double density = ComplexOps::abs_squared(psi[i]);

        if (density < actual_threshold) {
            cores.push_back(i);
        }
    }

    std::cout << "  Found " << cores.size() << " cores below threshold" << std::endl;

    // Show first few cores
    size_t show_count = std::min(static_cast<size_t>(5), cores.size());
    for (size_t i = 0; i < show_count; ++i) {
        size_t idx = cores[i];
        double d = ComplexOps::abs_squared(psi[idx]);
        std::cout << "    Core " << i << ": index=" << idx << ", density=" << d << std::endl;
    }

    return cores;
}

int VortexDetector::calculate_winding_number(
    size_t center_index,
    const std::vector<int>& neighbor_indices,
    size_t n_neighbors,
    const std::vector<Complex>& psi)
{
    // Calculate total phase change around loop of neighbors
    double total_phase_change = 0.0;

    for (size_t i = 0; i < n_neighbors; ++i) {
        size_t j = (i + 1) % n_neighbors;

        int idx_i = neighbor_indices[center_index * n_neighbors + i];
        int idx_j = neighbor_indices[center_index * n_neighbors + j];

        if (idx_i < 0 || idx_j < 0) continue;

        double phase_diff = ComplexOps::phase_difference(psi[idx_j], psi[idx_i]);
        total_phase_change += phase_diff;
    }

    // Winding number = total phase change / 2π
    int winding = static_cast<int>(std::round(total_phase_change / (2.0 * M_PI)));

    return winding;
}

float VortexDetector::calculate_circulation(
    const std::vector<size_t>& loop_indices,
    const std::vector<Complex>& psi)
{
    double circulation = 0.0;

    for (size_t i = 0; i < loop_indices.size(); ++i) {
        size_t j = (i + 1) % loop_indices.size();

        size_t idx_i = loop_indices[i];
        size_t idx_j = loop_indices[j];

        double phase_diff = ComplexOps::phase_difference(psi[idx_j], psi[idx_i]);
        circulation += phase_diff;
    }

    return static_cast<float>(circulation);
}

uint32_t PhononDetector::detect_phonons(
    const std::vector<Vector4>& coords,
    const std::vector<Complex>& psi,
    float wavelength_min,
    float wavelength_max)
{
    // Simplified: count density fluctuations
    // For production, implement proper Fourier analysis

    uint32_t count = 0;

    // Calculate density variations
    std::vector<double> densities(psi.size());
    for (size_t i = 0; i < psi.size(); ++i) {
        densities[i] = ComplexOps::abs_squared(psi[i]);
    }

    // Calculate mean density
    double mean_density = 0.0;
    for (double d : densities) {
        mean_density += d;
    }
    mean_density /= densities.size();

    // Count significant fluctuations
    for (double d : densities) {
        if (std::abs(d - mean_density) > 0.1 * mean_density) {
            count++;
        }
    }

    return count / 10;  // Simplified count
}

uint32_t RotonDetector::detect_rotons(
    const std::vector<Vector4>& coords,
    const std::vector<Complex>& psi)
{
    // Simplified: detect localized excitations
    // For production, implement proper dispersion analysis

    uint32_t count = 0;

    // Look for localized density perturbations
    for (size_t i = 2; i < psi.size(); ++i) {
        double density = ComplexOps::abs_squared(psi[i]);

        if (density > 2.0) {  // Localized high density
            count++;
        }
    }

    return count;
}

} // namespace BEC
