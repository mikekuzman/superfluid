#include "NeighborFinder.h"
#include <iostream>
#include <algorithm>

// Simplified KD-tree implementation without nanoflann dependency
// For production, use nanoflann for better performance

namespace BEC {

class NeighborFinder::Impl {
public:
    std::vector<Vector4> points;

    void build(const std::vector<Vector4>& pts) {
        points = pts;
    }

    void find_k_nearest(
        const Vector4& query,
        size_t k,
        std::vector<size_t>& indices,
        std::vector<float>& distances) const
    {
        // Simple brute-force approach
        // For production, replace with proper KD-tree

        struct DistIndex {
            float dist;
            size_t index;

            bool operator<(const DistIndex& other) const {
                return dist < other.dist;
            }
        };

        std::vector<DistIndex> dist_indices;
        dist_indices.reserve(points.size());

        for (size_t i = 0; i < points.size(); ++i) {
            float dist = Vector4::distance(query, points[i]);
            dist_indices.push_back({dist, i});
        }

        // Sort and take k nearest
        std::partial_sort(
            dist_indices.begin(),
            dist_indices.begin() + std::min(k, dist_indices.size()),
            dist_indices.end()
        );

        indices.clear();
        distances.clear();

        for (size_t i = 0; i < std::min(k, dist_indices.size()); ++i) {
            indices.push_back(dist_indices[i].index);
            distances.push_back(dist_indices[i].dist);
        }
    }
};

NeighborFinder::NeighborFinder()
    : m_impl(std::make_unique<Impl>())
{
}

NeighborFinder::~NeighborFinder() = default;

void NeighborFinder::build(const std::vector<Vector4>& points) {
    m_impl->build(points);
    std::cout << "Built neighbor tree with " << points.size() << " points" << std::endl;
}

void NeighborFinder::find_k_nearest(
    const Vector4& query_point,
    size_t k,
    std::vector<size_t>& indices,
    std::vector<float>& distances) const
{
    m_impl->find_k_nearest(query_point, k, indices, distances);
}

void NeighborFinder::find_all_neighbors(
    const std::vector<Vector4>& points,
    size_t k,
    std::vector<int>& neighbor_indices,
    std::vector<float>& neighbor_distances)
{
    size_t n_points = points.size();
    neighbor_indices.resize(n_points * k);
    neighbor_distances.resize(n_points * k);

    std::cout << "Finding " << k << " neighbors for each of " << n_points << " points..." << std::endl;

    for (size_t i = 0; i < n_points; ++i) {
        std::vector<size_t> indices;
        std::vector<float> distances;

        find_k_nearest(points[i], k + 1, indices, distances);  // +1 to exclude self

        // Skip first neighbor (self)
        for (size_t j = 0; j < k && j + 1 < indices.size(); ++j) {
            neighbor_indices[i * k + j] = static_cast<int>(indices[j + 1]);
            neighbor_distances[i * k + j] = distances[j + 1];
        }

        if ((i + 1) % 500 == 0) {
            std::cout << "  Processed " << (i + 1) << " / " << n_points << " points" << std::endl;
        }
    }

    std::cout << "Neighbor finding complete." << std::endl;
}

size_t NeighborFinder::size() const {
    return m_impl->points.size();
}

} // namespace BEC
