#pragma once

#include "BECFileFormat.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>

namespace BEC {

/**
 * Writes BEC simulation data to compressed binary files
 */
class BECFileWriter {
public:
    BECFileWriter(const std::string& filename, const BECHeader& header);
    ~BECFileWriter();

    // Non-copyable
    BECFileWriter(const BECFileWriter&) = delete;
    BECFileWriter& operator=(const BECFileWriter&) = delete;

    /**
     * Write neighbor graph data (call before adding snapshots)
     * @param neighbor_indices - Neighbor indices (n_points × n_neighbors)
     * @param neighbor_distances - Neighbor distances (n_points × n_neighbors)
     */
    bool write_neighbor_data(
        const std::vector<int>& neighbor_indices,
        const std::vector<float>& neighbor_distances
    );

    /**
     * Add a snapshot to the file
     * @param points - Array of point data
     * @param stats - Snapshot statistics
     * @param vortices - Detected vortices (optional)
     */
    bool add_snapshot(
        const std::vector<PointData>& points,
        const SnapshotStatistics& stats,
        const std::vector<VortexInfo>& vortices = {}
    );

    /**
     * Finalize the file (writes index table, updates header)
     */
    bool finalize();

    /**
     * Get current snapshot count
     */
    size_t get_snapshot_count() const { return m_snapshot_indices.size(); }

    /**
     * Get total file size
     */
    size_t get_file_size() const;

    /**
     * Get average compression ratio
     */
    float get_average_compression_ratio() const;

private:
    std::string m_filename;
    BECHeader m_header;
    std::ofstream m_file;
    std::vector<SnapshotIndex> m_snapshot_indices;
    std::vector<SnapshotStatistics> m_snapshot_stats;
    std::vector<std::vector<VortexInfo>> m_snapshot_vortices;
    bool m_finalized;

    // Helper functions
    bool write_header();
    bool compress_and_write(const std::vector<PointData>& points, SnapshotIndex& index);
    void update_header_in_file();
};

} // namespace BEC
