#pragma once

#include "BECFileFormat.h"
#include <string>
#include <vector>
#include <memory>

namespace BEC {

/**
 * Reads BEC simulation data from compressed binary files
 * Uses memory mapping for efficient large file access
 */
class BECFileReader {
public:
    BECFileReader();
    ~BECFileReader();

    // Non-copyable
    BECFileReader(const BECFileReader&) = delete;
    BECFileReader& operator=(const BECFileReader&) = delete;

    /**
     * Open a BEC file
     */
    bool open(const std::string& filename);

    /**
     * Close the file
     */
    void close();

    /**
     * Check if file is open
     */
    bool is_open() const { return m_is_open; }

    /**
     * Get header
     */
    const BECHeader& get_header() const { return m_header; }

    /**
     * Get number of snapshots
     */
    size_t get_snapshot_count() const { return m_header.n_snapshots; }

    /**
     * Get snapshot statistics
     */
    const SnapshotStatistics& get_snapshot_stats(size_t index) const;

    /**
     * Read a specific snapshot
     */
    bool read_snapshot(size_t index, std::vector<PointData>& points);

    /**
     * Read vortices for a snapshot (if available)
     */
    bool read_vortices(size_t index, std::vector<VortexInfo>& vortices);

    /**
     * Get file size
     */
    size_t get_file_size() const { return m_file_size; }

private:
    std::string m_filename;
    BECHeader m_header;
    std::vector<SnapshotIndex> m_snapshot_indices;
    std::vector<SnapshotStatistics> m_snapshot_stats;
    bool m_is_open;
    size_t m_file_size;

    // Memory mapped file handle (platform-specific)
    void* m_file_mapping;
    void* m_mapped_data;

    // Helper functions
    bool read_header();
    bool read_indices();
    bool read_statistics();
    bool decompress_snapshot(const SnapshotIndex& index, std::vector<PointData>& points);

    // Platform-specific memory mapping
    bool create_memory_mapping();
    void destroy_memory_mapping();
};

} // namespace BEC
