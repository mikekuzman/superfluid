#include "BECFileWriter.h"
#include <iostream>
#include <cstring>

// LZ4 compression library
#ifdef USE_LZ4
#include <lz4.h>
#else
// Fallback: no compression (just copy data)
namespace {
    int LZ4_compress_default(const char* src, char* dst, int srcSize, int dstCapacity) {
        if (srcSize > dstCapacity) return 0;
        std::memcpy(dst, src, srcSize);
        return srcSize;
    }

    int LZ4_compressBound(int inputSize) {
        return inputSize;
    }
}
#endif

namespace BEC {

BECFileWriter::BECFileWriter(const std::string& filename, const BECHeader& header)
    : m_filename(filename)
    , m_header(header)
    , m_finalized(false)
{
    // Open file in binary mode
    m_file.open(filename, std::ios::binary | std::ios::out);

    if (!m_file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    // Write initial header (will be updated in finalize())
    write_header();
}

BECFileWriter::~BECFileWriter() {
    if (!m_finalized && m_file.is_open()) {
        std::cerr << "Warning: File not finalized before destruction. Finalizing now..." << std::endl;
        finalize();
    }

    if (m_file.is_open()) {
        m_file.close();
    }
}

bool BECFileWriter::write_header() {
    m_file.seekp(0);
    m_file.write(reinterpret_cast<const char*>(&m_header), sizeof(BECHeader));
    return m_file.good();
}

bool BECFileWriter::add_snapshot(
    const std::vector<PointData>& points,
    const SnapshotStatistics& stats,
    const std::vector<VortexInfo>& vortices)
{
    if (!m_file.is_open()) {
        std::cerr << "File not open" << std::endl;
        return false;
    }

    if (m_finalized) {
        std::cerr << "Cannot add snapshot to finalized file" << std::endl;
        return false;
    }

    // Verify point count matches header
    if (points.size() != m_header.n_points_per_snapshot) {
        std::cerr << "Point count mismatch: expected " << m_header.n_points_per_snapshot
                  << ", got " << points.size() << std::endl;
        return false;
    }

    // Compress and write point data
    SnapshotIndex index;
    if (!compress_and_write(points, index)) {
        return false;
    }

    // Store index, statistics, and vortices
    m_snapshot_indices.push_back(index);
    m_snapshot_stats.push_back(stats);
    m_snapshot_vortices.push_back(vortices);

    return true;
}

bool BECFileWriter::compress_and_write(const std::vector<PointData>& points, SnapshotIndex& index) {
    // Get current file position
    index.file_offset = m_file.tellp();
    index.uncompressed_size = points.size() * sizeof(PointData);

    // Allocate compression buffer
    int max_compressed_size = LZ4_compressBound(index.uncompressed_size);
    std::vector<char> compressed_buffer(max_compressed_size);

    // Compress
    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(points.data()),
        compressed_buffer.data(),
        index.uncompressed_size,
        max_compressed_size
    );

    if (compressed_size <= 0) {
        std::cerr << "Compression failed" << std::endl;
        return false;
    }

    index.compressed_size = compressed_size;
    index.compression_ratio = static_cast<float>(index.uncompressed_size) / compressed_size;

    // Write compressed size first
    m_file.write(reinterpret_cast<const char*>(&compressed_size), sizeof(uint32_t));

    // Write compressed data
    m_file.write(compressed_buffer.data(), compressed_size);

    return m_file.good();
}

bool BECFileWriter::finalize() {
    if (!m_file.is_open()) {
        return false;
    }

    if (m_finalized) {
        return true;
    }

    // Update header with actual snapshot count
    m_header.n_snapshots = m_snapshot_indices.size();

    // Write snapshot index table
    uint64_t index_offset = m_file.tellp();

    // Write number of indices
    uint32_t index_count = m_snapshot_indices.size();
    m_file.write(reinterpret_cast<const char*>(&index_count), sizeof(uint32_t));

    // Write indices
    m_file.write(
        reinterpret_cast<const char*>(m_snapshot_indices.data()),
        m_snapshot_indices.size() * sizeof(SnapshotIndex)
    );

    // Write statistics
    uint32_t stats_count = m_snapshot_stats.size();
    m_file.write(reinterpret_cast<const char*>(&stats_count), sizeof(uint32_t));
    m_file.write(
        reinterpret_cast<const char*>(m_snapshot_stats.data()),
        m_snapshot_stats.size() * sizeof(SnapshotStatistics)
    );

    // Write vortex data
    uint32_t vortex_snapshot_count = m_snapshot_vortices.size();
    m_file.write(reinterpret_cast<const char*>(&vortex_snapshot_count), sizeof(uint32_t));

    for (const auto& vortex_list : m_snapshot_vortices) {
        uint32_t vortex_count = vortex_list.size();
        m_file.write(reinterpret_cast<const char*>(&vortex_count), sizeof(uint32_t));
        if (vortex_count > 0) {
            m_file.write(
                reinterpret_cast<const char*>(vortex_list.data()),
                vortex_list.size() * sizeof(VortexInfo)
            );
        }
    }

    // Update header in file
    update_header_in_file();

    m_finalized = true;
    m_file.flush();

    std::cout << "File finalized: " << m_snapshot_indices.size() << " snapshots written" << std::endl;
    std::cout << "Average compression ratio: " << get_average_compression_ratio() << "x" << std::endl;

    return true;
}

void BECFileWriter::update_header_in_file() {
    auto current_pos = m_file.tellp();
    write_header();
    m_file.seekp(current_pos);
}

size_t BECFileWriter::get_file_size() const {
    if (!m_file.is_open()) return 0;

    auto current_pos = const_cast<std::ofstream&>(m_file).tellp();
    const_cast<std::ofstream&>(m_file).seekp(0, std::ios::end);
    size_t size = const_cast<std::ofstream&>(m_file).tellp();
    const_cast<std::ofstream&>(m_file).seekp(current_pos);

    return size;
}

float BECFileWriter::get_average_compression_ratio() const {
    if (m_snapshot_indices.empty()) return 1.0f;

    float total_ratio = 0.0f;
    for (const auto& index : m_snapshot_indices) {
        total_ratio += index.compression_ratio;
    }

    return total_ratio / m_snapshot_indices.size();
}

} // namespace BEC
