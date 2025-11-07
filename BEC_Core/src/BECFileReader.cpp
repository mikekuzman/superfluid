#include "BECFileReader.h"
#include <iostream>
#include <fstream>
#include <cstring>

// Platform-specific includes for memory mapping
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// LZ4 decompression
#ifdef USE_LZ4
#include <lz4.h>
#else
// Fallback: no compression (just copy data)
namespace {
    int LZ4_decompress_safe(const char* src, char* dst, int compressedSize, int dstCapacity) {
        if (compressedSize > dstCapacity) return -1;
        std::memcpy(dst, src, compressedSize);
        return compressedSize;
    }
}
#endif

namespace BEC {

BECFileReader::BECFileReader()
    : m_is_open(false)
    , m_file_size(0)
    , m_file_mapping(nullptr)
    , m_mapped_data(nullptr)
{
}

BECFileReader::~BECFileReader() {
    close();
}

bool BECFileReader::open(const std::string& filename) {
    if (m_is_open) {
        close();
    }

    m_filename = filename;

    // Get file size
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    m_file_size = file.tellg();
    file.close();

    if (m_file_size < sizeof(BECHeader)) {
        std::cerr << "File too small to be valid BEC file" << std::endl;
        return false;
    }

    // Create memory mapping
    if (!create_memory_mapping()) {
        std::cerr << "Failed to create memory mapping" << std::endl;
        return false;
    }

    // Read header
    if (!read_header()) {
        std::cerr << "Failed to read header" << std::endl;
        destroy_memory_mapping();
        return false;
    }

    // Validate header
    if (!m_header.is_valid()) {
        std::cerr << "Invalid BEC file header" << std::endl;
        destroy_memory_mapping();
        return false;
    }

    // Read indices and statistics
    if (!read_indices() || !read_statistics()) {
        std::cerr << "Failed to read file metadata" << std::endl;
        destroy_memory_mapping();
        return false;
    }

    m_is_open = true;

    std::cout << "Opened BEC file: " << filename << std::endl;
    std::cout << "  Snapshots: " << m_header.n_snapshots << std::endl;
    std::cout << "  Points per snapshot: " << m_header.n_points_per_snapshot << std::endl;
    std::cout << "  R = " << m_header.R << ", delta = " << m_header.delta << std::endl;

    return true;
}

void BECFileReader::close() {
    if (m_is_open) {
        destroy_memory_mapping();
        m_snapshot_indices.clear();
        m_snapshot_stats.clear();
        m_is_open = false;
    }
}

bool BECFileReader::read_header() {
    if (!m_mapped_data) return false;

    std::memcpy(&m_header, m_mapped_data, sizeof(BECHeader));
    return true;
}

bool BECFileReader::read_indices() {
    // For now, read from file sequentially
    // In a complete implementation, store index offset in header
    std::ifstream file(m_filename, std::ios::binary);
    if (!file.is_open()) return false;

    // Skip to end of all snapshot data
    // This is simplified - in production, store index offset in header
    file.seekg(sizeof(BECHeader), std::ios::beg);

    // Read indices
    uint32_t index_count = 0;

    // For now, we'll reconstruct indices by reading the file
    // This is a simplified version
    m_snapshot_indices.resize(m_header.n_snapshots);

    for (size_t i = 0; i < m_header.n_snapshots; ++i) {
        // Record current position
        m_snapshot_indices[i].file_offset = file.tellg();

        // Read compressed size
        uint32_t compressed_size = 0;
        file.read(reinterpret_cast<char*>(&compressed_size), sizeof(uint32_t));

        m_snapshot_indices[i].compressed_size = compressed_size;
        m_snapshot_indices[i].uncompressed_size = m_header.n_points_per_snapshot * sizeof(PointData);

        // Skip compressed data
        file.seekg(compressed_size, std::ios::cur);
    }

    return true;
}

bool BECFileReader::read_statistics() {
    // Simplified: create default statistics
    m_snapshot_stats.resize(m_header.n_snapshots);

    for (size_t i = 0; i < m_header.n_snapshots; ++i) {
        m_snapshot_stats[i].time = i * m_header.dt;
    }

    return true;
}

const SnapshotStatistics& BECFileReader::get_snapshot_stats(size_t index) const {
    if (index >= m_snapshot_stats.size()) {
        static SnapshotStatistics default_stats;
        return default_stats;
    }
    return m_snapshot_stats[index];
}

bool BECFileReader::read_snapshot(size_t index, std::vector<PointData>& points) {
    if (index >= m_snapshot_indices.size()) {
        std::cerr << "Snapshot index out of range" << std::endl;
        return false;
    }

    return decompress_snapshot(m_snapshot_indices[index], points);
}

bool BECFileReader::decompress_snapshot(const SnapshotIndex& index, std::vector<PointData>& points) {
    // Read compressed data from file
    std::ifstream file(m_filename, std::ios::binary);
    if (!file.is_open()) return false;

    // Seek to snapshot data (skip the size field)
    file.seekg(index.file_offset + sizeof(uint32_t), std::ios::beg);

    // Read compressed data
    std::vector<char> compressed_buffer(index.compressed_size);
    file.read(compressed_buffer.data(), index.compressed_size);

    // Allocate output buffer
    points.resize(m_header.n_points_per_snapshot);

    // Decompress
    int decompressed_size = LZ4_decompress_safe(
        compressed_buffer.data(),
        reinterpret_cast<char*>(points.data()),
        index.compressed_size,
        index.uncompressed_size
    );

    if (decompressed_size != static_cast<int>(index.uncompressed_size)) {
        std::cerr << "Decompression failed or size mismatch" << std::endl;
        return false;
    }

    return true;
}

bool BECFileReader::read_vortices(size_t index, std::vector<VortexInfo>& vortices) {
    // Simplified: return empty for now
    vortices.clear();
    return true;
}

bool BECFileReader::create_memory_mapping() {
#ifdef _WIN32
    // Windows implementation
    HANDLE hFile = CreateFileA(
        m_filename.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    HANDLE hMapping = CreateFileMappingA(
        hFile,
        nullptr,
        PAGE_READONLY,
        0,
        0,
        nullptr
    );

    CloseHandle(hFile);

    if (!hMapping) {
        return false;
    }

    void* pData = MapViewOfFile(
        hMapping,
        FILE_MAP_READ,
        0,
        0,
        0
    );

    if (!pData) {
        CloseHandle(hMapping);
        return false;
    }

    m_file_mapping = hMapping;
    m_mapped_data = pData;

    return true;
#else
    // Linux/POSIX implementation
    int fd = ::open(m_filename.c_str(), O_RDONLY);
    if (fd == -1) {
        return false;
    }

    void* pData = ::mmap(nullptr, m_file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);

    if (pData == MAP_FAILED) {
        return false;
    }

    m_mapped_data = pData;
    m_file_mapping = nullptr;  // Not used on POSIX

    return true;
#endif
}

void BECFileReader::destroy_memory_mapping() {
#ifdef _WIN32
    if (m_mapped_data) {
        UnmapViewOfFile(m_mapped_data);
        m_mapped_data = nullptr;
    }

    if (m_file_mapping) {
        CloseHandle(m_file_mapping);
        m_file_mapping = nullptr;
    }
#else
    if (m_mapped_data) {
        ::munmap(m_mapped_data, m_file_size);
        m_mapped_data = nullptr;
    }
#endif
}

} // namespace BEC
