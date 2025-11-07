#pragma once

#include "Vector4.h"
#include <vector>
#include <memory>

namespace BEC {

/**
 * KD-tree based neighbor finding for 4D points
 * Uses nanoflann library
 */
class NeighborFinder {
public:
    NeighborFinder();
    ~NeighborFinder();

    /**
     * Build KD-tree from points
     */
    void build(const std::vector<Vector4>& points);

    /**
     * Find k nearest neighbors
     * @param query_point - Point to find neighbors for
     * @param k - Number of neighbors
     * @param indices - Output neighbor indices
     * @param distances - Output distances
     */
    void find_k_nearest(
        const Vector4& query_point,
        size_t k,
        std::vector<size_t>& indices,
        std::vector<float>& distances
    ) const;

    /**
     * Find all neighbors for all points
     * @param points - Input points
     * @param k - Number of neighbors per point
     * @param neighbor_indices - Output: flat array [n_points * k]
     * @param neighbor_distances - Output: flat array [n_points * k]
     */
    void find_all_neighbors(
        const std::vector<Vector4>& points,
        size_t k,
        std::vector<int>& neighbor_indices,
        std::vector<float>& neighbor_distances
    );

    /**
     * Get number of points in tree
     */
    size_t size() const;

private:
    class Impl;  // Forward declaration for pimpl
    std::unique_ptr<Impl> m_impl;
};

} // namespace BEC
