/**
 * BEC File Inspector
 *
 * Simple utility to read and display information from .bec files
 * for testing and validation purposes.
 */

#include "BECFileReader.h"
#include "BECFileFormat.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace BEC;

void print_header(const BECHeader& header) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "BEC File Header" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "Magic: 0x" << std::hex << header.magic << std::dec;
    if (header.magic == BEC_MAGIC) {
        std::cout << " ✓ (valid)" << std::endl;
    } else {
        std::cout << " ✗ (INVALID - expected 0x" << std::hex << BEC_MAGIC << std::dec << ")" << std::endl;
    }

    std::cout << "Version: " << header.version << std::endl;
    std::cout << "Snapshots: " << header.n_snapshots << std::endl;
    std::cout << "Points per snapshot: " << header.n_points_per_snapshot << std::endl;

    std::cout << "\nPhysical Parameters:" << std::endl;
    std::cout << "  R = " << header.R << " (healing lengths)" << std::endl;
    std::cout << "  delta = " << header.delta << " (healing lengths)" << std::endl;
    std::cout << "  thickness_ratio = " << header.thickness_ratio << std::endl;
    std::cout << "  g = " << header.g << " (interaction strength)" << std::endl;
    std::cout << "  omega = " << header.omega << " (rotation frequency)" << std::endl;

    std::cout << "\nComputational Parameters:" << std::endl;
    std::cout << "  N = " << header.N << " (grid resolution)" << std::endl;
    std::cout << "  dt = " << header.dt << " (time step)" << std::endl;
    std::cout << "  n_neighbors = " << header.n_neighbors << std::endl;
    std::cout << "  random_seed = " << header.random_seed << std::endl;

    std::cout << "\nPole Positions (4D):" << std::endl;
    std::cout << "  North pole: " << header.north_pole_4d << std::endl;
    std::cout << "  South pole: " << header.south_pole_4d << std::endl;
}

void print_snapshot_stats(size_t index, const SnapshotStatistics& stats) {
    std::cout << "\n--- Snapshot " << index << " ---" << std::endl;
    std::cout << "Time: " << stats.time << std::endl;

    std::cout << "Density:" << std::endl;
    std::cout << "  min = " << stats.density_min << std::endl;
    std::cout << "  max = " << stats.density_max << std::endl;
    std::cout << "  mean = " << stats.density_mean << std::endl;
    std::cout << "  std = " << stats.density_std << std::endl;
    std::cout << "  5th percentile = " << stats.density_p5 << std::endl;
    std::cout << "  95th percentile = " << stats.density_p95 << std::endl;

    std::cout << "Energy:" << std::endl;
    std::cout << "  total = " << stats.total_energy << std::endl;
    std::cout << "  kinetic = " << stats.kinetic_energy << std::endl;
    std::cout << "  interaction = " << stats.interaction_energy << std::endl;
    std::cout << "  rotation = " << stats.rotation_energy << std::endl;

    std::cout << "Excitations:" << std::endl;
    std::cout << "  vortices = " << stats.n_vortices << std::endl;
    std::cout << "  phonons = " << stats.n_phonons << std::endl;
    std::cout << "  rotons = " << stats.n_rotons << std::endl;
}

void print_point_sample(const std::vector<PointData>& points, size_t n_samples = 5) {
    std::cout << "\nSample Points (first " << n_samples << "):" << std::endl;
    std::cout << std::setw(6) << "Index" << " | "
              << std::setw(10) << "Coords" << " | "
              << std::setw(8) << "Density" << " | "
              << std::setw(8) << "Phase" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (size_t i = 0; i < std::min(n_samples, points.size()); ++i) {
        const auto& p = points[i];
        std::cout << std::setw(6) << i << " | "
                  << "(" << p.coord[0] << "," << p.coord[1] << ","
                  << p.coord[2] << "," << p.coord[3] << ") | "
                  << std::setw(8) << p.density << " | "
                  << std::setw(8) << p.phase << std::endl;
    }
}

void validate_file(const BECHeader& header, const std::vector<PointData>& points) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Validation" << std::endl;
    std::cout << "========================================" << std::endl;

    bool valid = true;

    // Check header
    if (header.magic != BEC_MAGIC) {
        std::cout << "✗ Invalid magic number" << std::endl;
        valid = false;
    } else {
        std::cout << "✓ Valid magic number" << std::endl;
    }

    if (header.version != BEC_VERSION) {
        std::cout << "✗ Unsupported version: " << header.version << std::endl;
        valid = false;
    } else {
        std::cout << "✓ Valid version" << std::endl;
    }

    // Check point count
    if (points.size() == header.n_points_per_snapshot) {
        std::cout << "✓ Point count matches header: " << points.size() << std::endl;
    } else {
        std::cout << "✗ Point count mismatch: expected " << header.n_points_per_snapshot
                  << ", got " << points.size() << std::endl;
        valid = false;
    }

    // Check poles (indices 0 and 1)
    if (points.size() >= 2) {
        std::cout << "✓ File has pole markers at indices 0 and 1" << std::endl;
    } else {
        std::cout << "✗ File too small to contain poles" << std::endl;
        valid = false;
    }

    // Check physical parameters
    if (header.R > 0 && header.delta > 0) {
        std::cout << "✓ Physical parameters valid (R=" << header.R << ", delta=" << header.delta << ")" << std::endl;
    } else {
        std::cout << "✗ Invalid physical parameters" << std::endl;
        valid = false;
    }

    if (valid) {
        std::cout << "\n✓ File is valid!" << std::endl;
    } else {
        std::cout << "\n✗ File has validation errors!" << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <filename.bec> [options]" << std::endl;
        std::cout << "\nOptions:" << std::endl;
        std::cout << "  --header          Show header only" << std::endl;
        std::cout << "  --snapshot <N>    Show specific snapshot (default: 0)" << std::endl;
        std::cout << "  --all-snapshots   Show all snapshots" << std::endl;
        std::cout << "  --validate        Validate file integrity" << std::endl;
        std::cout << "  --points <N>      Show first N points (default: 5)" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    // Parse options
    bool show_header = true;
    bool show_snapshots = true;
    bool show_all = false;
    bool validate_only = false;
    size_t snapshot_index = 0;
    size_t n_points_to_show = 5;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--header") {
            show_snapshots = false;
        } else if (arg == "--snapshot" && i + 1 < argc) {
            snapshot_index = std::atoi(argv[++i]);
        } else if (arg == "--all-snapshots") {
            show_all = true;
        } else if (arg == "--validate") {
            validate_only = true;
        } else if (arg == "--points" && i + 1 < argc) {
            n_points_to_show = std::atoi(argv[++i]);
        }
    }

    // Open file
    BECFileReader reader;
    if (!reader.open(filename)) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 1;
    }

    const BECHeader& header = reader.get_header();

    // Always show header
    if (show_header) {
        print_header(header);
    }

    // Read first snapshot for validation
    std::vector<PointData> points;
    if (reader.read_snapshot(0, points)) {
        if (validate_only || show_header) {
            validate_file(header, points);
        }

        if (!validate_only && show_snapshots) {
            if (show_all) {
                for (size_t i = 0; i < reader.get_snapshot_count(); ++i) {
                    print_snapshot_stats(i, reader.get_snapshot_stats(i));
                }
            } else {
                if (snapshot_index < reader.get_snapshot_count()) {
                    print_snapshot_stats(snapshot_index, reader.get_snapshot_stats(snapshot_index));

                    std::vector<PointData> snap_points;
                    if (reader.read_snapshot(snapshot_index, snap_points)) {
                        print_point_sample(snap_points, n_points_to_show);
                    }
                } else {
                    std::cerr << "Snapshot index " << snapshot_index
                              << " out of range (0-" << reader.get_snapshot_count() - 1 << ")" << std::endl;
                }
            }
        }
    } else {
        std::cerr << "Failed to read snapshot data" << std::endl;
        return 1;
    }

    reader.close();

    return 0;
}
