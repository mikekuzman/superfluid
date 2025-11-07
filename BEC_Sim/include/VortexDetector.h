#pragma once

#include "Vector4.h"
#include "Complex.h"
#include "BECFileFormat.h"
#include <vector>

namespace BEC {

/**
 * Detects quantized vortices in the BEC
 */
class VortexDetector {
public:
    /**
     * Detect vortices in wavefunction
     * @param coords - 4D coordinates
     * @param psi - Wavefunction values
     * @param neighbor_indices - Neighbor connectivity
     * @param n_neighbors - Number of neighbors per point
     * @return Vector of detected vortices
     */
    static std::vector<VortexInfo> detect_vortices(
        const std::vector<Vector4>& coords,
        const std::vector<Complex>& psi,
        const std::vector<int>& neighbor_indices,
        size_t n_neighbors
    );

    /**
     * Calculate circulation around a loop
     * @param loop_indices - Indices of points forming closed loop
     * @param psi - Wavefunction values
     * @return Circulation (should be 2π * n for vortex)
     */
    static float calculate_circulation(
        const std::vector<size_t>& loop_indices,
        const std::vector<Complex>& psi
    );

    /**
     * Find vortex core locations
     * Vortex cores have low density and phase winding
     */
    static std::vector<size_t> find_vortex_cores(
        const std::vector<Complex>& psi,
        float density_threshold = 0.1f
    );

    /**
     * Calculate phase winding number around a point
     */
    static int calculate_winding_number(
        size_t center_index,
        const std::vector<int>& neighbor_indices,
        size_t n_neighbors,
        const std::vector<Complex>& psi
    );
};

/**
 * Detects phonons (sound wave excitations)
 */
class PhononDetector {
public:
    /**
     * Detect phonon modes
     */
    static uint32_t detect_phonons(
        const std::vector<Vector4>& coords,
        const std::vector<Complex>& psi,
        float wavelength_min = 10.0f,
        float wavelength_max = 100.0f
    );
};

/**
 * Detects rotons (localized excitations)
 */
class RotonDetector {
public:
    /**
     * Detect roton excitations
     */
    static uint32_t detect_rotons(
        const std::vector<Vector4>& coords,
        const std::vector<Complex>& psi
    );
};

} // namespace BEC
