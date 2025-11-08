#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <cstring>
#include "Vector4.h"

// Define M_PI if not defined (for MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace BEC {

// File format constants
constexpr uint32_t BEC_MAGIC = 0x42454300;  // "BEC\0"
constexpr uint32_t BEC_VERSION = 1;
constexpr size_t BEC_HEADER_SIZE = 4096;

/**
 * Main file header (4096 bytes)
 * Contains all metadata and physical parameters
 */
struct BECHeader {
    // File identification
    uint32_t magic;                     // Must be BEC_MAGIC (0x42454300)
    uint32_t version;                   // Format version (currently 1)
    uint32_t n_snapshots;               // Number of time snapshots in file
    uint32_t n_points_per_snapshot;     // Points per snapshot (including 2 poles)

    // Physical parameters
    float R;                            // Hypersphere radius (healing lengths)
    float delta;                        // Shell thickness (healing lengths)
    float thickness_ratio;              // R/delta ratio (if used)
    float g;                            // Interaction strength (dimensionless)
    float omega;                        // Rotation frequency (dimensionless)

    // Computational parameters
    uint32_t N;                         // Grid resolution parameter
    float dt;                           // Time step (dimensionless)
    uint32_t n_neighbors;               // Number of neighbors for gradient calc
    uint32_t random_seed;               // RNG seed for reproducibility

    // Pole positions in 4D
    Vector4 north_pole_4d;              // North pole: [0, 0, 0, R]
    Vector4 south_pole_4d;              // South pole: [0, 0, 0, -R]

    // Neighbor graph data location (0 = not present)
    uint64_t neighbor_data_offset;      // File offset to neighbor data (after header)
    uint32_t neighbor_data_size;        // Size of neighbor data in bytes
    uint32_t has_neighbor_data;         // 1 if neighbor data present, 0 otherwise

    // Reserved for future use (pad to 4096 bytes)
    uint8_t reserved[BEC_HEADER_SIZE - 100];

    // Constructor with defaults
    BECHeader() {
        std::memset(this, 0, sizeof(BECHeader));
        magic = BEC_MAGIC;
        version = BEC_VERSION;

        // Default physical parameters
        R = 1000.0f;
        delta = 25.0f;
        thickness_ratio = 40.0f;
        g = 0.05f;
        omega = 0.0f;

        // Default computational parameters
        N = 64;
        dt = 0.01f;
        n_neighbors = 6;
        random_seed = 42;

        // Initialize poles
        north_pole_4d = Vector4(0, 0, 0, R);
        south_pole_4d = Vector4(0, 0, 0, -R);
    }

    // Validate header
    bool is_valid() const {
        return magic == BEC_MAGIC && version == BEC_VERSION && n_snapshots > 0;
    }

    // Calculate delta from thickness_ratio if needed
    void apply_thickness_ratio() {
        if (thickness_ratio > 0.0f) {
            delta = R / thickness_ratio;
        }
    }

    // Update pole positions based on R
    void update_poles() {
        north_pole_4d = Vector4(0, 0, 0, R);
        south_pole_4d = Vector4(0, 0, 0, -R);
    }
};

static_assert(sizeof(BECHeader) == BEC_HEADER_SIZE, "BECHeader must be exactly 4096 bytes");

/**
 * Per-point data stored in file (quantized for compression)
 * 20 bytes per point before compression
 */
struct PointData {
    int16_t coord[4];           // Quantized w,x,y,z coordinates
    uint16_t density;           // Density |ψ|² in float16 format
    uint16_t phase;             // Phase arg(ψ) quantized to [0,65535]→[0,2π]
    int16_t velocity[4];        // 4D velocity field (float16×4)

    PointData() {
        std::memset(this, 0, sizeof(PointData));
    }
};

static_assert(sizeof(PointData) == 20, "PointData must be 20 bytes");

/**
 * Snapshot statistics for visualization scaling
 */
struct SnapshotStatistics {
    float time;                 // Simulation time

    // Density statistics
    float density_min;
    float density_max;
    float density_mean;
    float density_std;
    float density_p5;           // 5th percentile
    float density_p95;          // 95th percentile

    // Phase statistics
    float phase_mean;
    float phase_std;

    // Velocity statistics
    float velocity_mean;
    float velocity_std;
    float velocity_max;

    // Energy and other observables
    float total_energy;
    float kinetic_energy;
    float interaction_energy;
    float rotation_energy;

    // Vortex count
    uint32_t n_vortices;
    uint32_t n_phonons;
    uint32_t n_rotons;

    uint32_t reserved[8];       // For future use

    SnapshotStatistics() {
        std::memset(this, 0, sizeof(SnapshotStatistics));
    }
};

/**
 * Snapshot index entry for fast seeking
 */
struct SnapshotIndex {
    uint64_t file_offset;           // Byte offset in file
    uint32_t compressed_size;       // Size of compressed data
    uint32_t uncompressed_size;     // Size before compression
    float compression_ratio;        // For monitoring

    SnapshotIndex() : file_offset(0), compressed_size(0),
                      uncompressed_size(0), compression_ratio(1.0f) {}
};

/**
 * Vortex detection result
 */
struct VortexInfo {
    Vector4 position_4d;        // Position in 4D
    int32_t quantum_number;     // Circulation quantum number
    float strength;             // Circulation strength

    VortexInfo() : position_4d(), quantum_number(0), strength(0.0f) {}
};

/**
 * Quantization helpers
 */
namespace Quantization {
    // Coordinate quantization (map float to int16)
    inline int16_t quantize_coord(float value, float min_val, float max_val) {
        float normalized = (value - min_val) / (max_val - min_val);
        normalized = std::max(0.0f, std::min(1.0f, normalized));
        return static_cast<int16_t>(normalized * 65535.0f - 32768.0f);
    }

    inline float dequantize_coord(int16_t quantized, float min_val, float max_val) {
        float normalized = (static_cast<float>(quantized) + 32768.0f) / 65535.0f;
        return min_val + normalized * (max_val - min_val);
    }

    // Phase quantization (map [0, 2π] to uint16)
    inline uint16_t quantize_phase(float phase) {
        // Wrap to [0, 2π]
        while (phase < 0.0f) phase += 2.0f * M_PI;
        while (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        return static_cast<uint16_t>((phase / (2.0f * M_PI)) * 65535.0f);
    }

    inline float dequantize_phase(uint16_t quantized) {
        return (static_cast<float>(quantized) / 65535.0f) * 2.0f * M_PI;
    }

    // Simple float16 conversion (for density and velocity)
    // Note: This is a simplified version. For production, use proper IEEE 754 float16
    inline uint16_t float_to_float16(float value) {
        // Simplified: just scale and quantize
        // Range: [0, 10] for density, [-10, 10] for velocity
        float clamped = std::max(-10.0f, std::min(10.0f, value));
        return static_cast<uint16_t>((clamped + 10.0f) / 20.0f * 65535.0f);
    }

    inline float float16_to_float(uint16_t value) {
        return (static_cast<float>(value) / 65535.0f) * 20.0f - 10.0f;
    }
}

} // namespace BEC
