#pragma once

#include "Vector4.h"
#include "Complex.h"
#include <vector>
#include <random>

namespace BEC {

/**
 * Spatial resampling utilities for geometry changes
 */
class SpatialResampler {
public:
    /**
     * Resample wavefunction from old geometry to new geometry
     * Uses spatial interpolation + deterministic noise for new regions
     */
    static void resample_wavefunction(
        const std::vector<Vector4>& old_coords,
        const std::vector<Complex>& old_psi,
        const std::vector<Vector4>& new_coords,
        std::vector<Complex>& new_psi,
        uint32_t random_seed,
        float noise_amplitude
    );

private:
    /**
     * Find K nearest neighbors in old geometry for a new point
     */
    static void find_nearest_neighbors(
        const Vector4& point,
        const std::vector<Vector4>& old_coords,
        std::vector<size_t>& indices,
        std::vector<float>& distances,
        size_t K = 4
    );

    /**
     * Interpolate wavefunction value using inverse distance weighting
     */
    static Complex interpolate_wavefunction(
        const std::vector<size_t>& neighbor_indices,
        const std::vector<float>& neighbor_distances,
        const std::vector<Complex>& old_psi
    );

    /**
     * Generate deterministic noise for a point index using RNG
     */
    static Complex generate_noise(
        size_t point_index,
        uint32_t random_seed,
        float noise_amplitude
    );
};

} // namespace BEC
