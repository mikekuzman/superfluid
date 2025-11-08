#include "SpatialResampler.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace BEC {

void SpatialResampler::resample_wavefunction(
    const std::vector<Vector4>& old_coords,
    const std::vector<Complex>& old_psi,
    const std::vector<Vector4>& new_coords,
    std::vector<Complex>& new_psi,
    uint32_t random_seed,
    float noise_amplitude)
{
    new_psi.resize(new_coords.size());

    // Distance threshold for interpolation (2x average spacing in old geometry)
    float avg_spacing = std::pow(old_coords.size(), -0.25f) * 10.0f; // Rough estimate
    float interp_threshold = 2.0f * avg_spacing;

    for (size_t i = 0; i < new_coords.size(); ++i) {
        // Special handling for poles (indices 0 and 1)
        if (i == 0 || i == 1) {
            new_psi[i] = Complex(1.0, 0.0);
            continue;
        }

        // Find nearest neighbors in old geometry
        std::vector<size_t> neighbor_indices;
        std::vector<float> neighbor_distances;
        find_nearest_neighbors(new_coords[i], old_coords, neighbor_indices, neighbor_distances, 4);

        // If closest neighbor is very close, use interpolation
        if (!neighbor_indices.empty() && neighbor_distances[0] < interp_threshold) {
            new_psi[i] = interpolate_wavefunction(neighbor_indices, neighbor_distances, old_psi);
        }
        else {
            // Point is in new region - use deterministic noise
            new_psi[i] = Complex(1.0, 0.0) + generate_noise(i, random_seed, noise_amplitude);
        }
    }
}

void SpatialResampler::find_nearest_neighbors(
    const Vector4& point,
    const std::vector<Vector4>& old_coords,
    std::vector<size_t>& indices,
    std::vector<float>& distances,
    size_t K)
{
    // Store all distances
    std::vector<std::pair<float, size_t>> dist_index;
    dist_index.reserve(old_coords.size());

    for (size_t i = 2; i < old_coords.size(); ++i) {  // Skip poles
        float dist = (point - old_coords[i]).magnitude();
        dist_index.push_back({dist, i});
    }

    // Partial sort to get K nearest
    size_t actual_K = std::min(K, dist_index.size());
    std::partial_sort(dist_index.begin(),
                      dist_index.begin() + actual_K,
                      dist_index.end(),
                      [](const auto& a, const auto& b) { return a.first < b.first; });

    // Extract results
    indices.resize(actual_K);
    distances.resize(actual_K);
    for (size_t i = 0; i < actual_K; ++i) {
        distances[i] = dist_index[i].first;
        indices[i] = dist_index[i].second;
    }
}

Complex SpatialResampler::interpolate_wavefunction(
    const std::vector<size_t>& neighbor_indices,
    const std::vector<float>& neighbor_distances,
    const std::vector<Complex>& old_psi)
{
    if (neighbor_indices.empty()) {
        return Complex(1.0, 0.0);
    }

    // Inverse distance weighted interpolation
    double total_weight = 0.0;
    Complex weighted_sum(0.0, 0.0);

    for (size_t i = 0; i < neighbor_indices.size(); ++i) {
        float dist = neighbor_distances[i];
        if (dist < 1e-6f) {
            // Point coincides with an old point - return its value
            return old_psi[neighbor_indices[i]];
        }

        double weight = 1.0 / (dist + 1e-6);
        weighted_sum = weighted_sum + old_psi[neighbor_indices[i]] * weight;
        total_weight += weight;
    }

    if (total_weight > 0.0) {
        return weighted_sum * (1.0 / total_weight);
    }

    return Complex(1.0, 0.0);
}

Complex SpatialResampler::generate_noise(
    size_t point_index,
    uint32_t random_seed,
    float noise_amplitude)
{
    // Use point index to generate deterministic noise
    // Seed the RNG with combined seed and index
    std::mt19937 rng(random_seed + static_cast<uint32_t>(point_index));
    std::normal_distribution<double> dist(0.0, 1.0);

    double real = noise_amplitude * dist(rng);
    double imag = noise_amplitude * dist(rng);

    return Complex(real, imag);
}

} // namespace BEC
