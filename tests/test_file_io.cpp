#include "BECFileWriter.h"
#include "BECFileReader.h"
#include "BECFileFormat.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace BEC;

int main() {
    std::cout << "=== Testing BEC File I/O ===" << std::endl;

    // Create test data
    const size_t n_points = 100;
    const size_t n_snapshots = 5;

    BECHeader header;
    header.n_points_per_snapshot = n_points;
    header.R = 1000.0f;
    header.delta = 25.0f;
    header.g = 0.05f;
    header.omega = 0.5f;

    std::cout << "Creating test file..." << std::endl;

    // Write test file
    {
        BECFileWriter writer("test_output.bec", header);

        for (size_t snap = 0; snap < n_snapshots; ++snap) {
            std::vector<PointData> points(n_points);

            // Fill with test data
            for (size_t i = 0; i < n_points; ++i) {
                points[i].coord[0] = i;
                points[i].coord[1] = i * 2;
                points[i].coord[2] = i * 3;
                points[i].coord[3] = i * 4;
                points[i].density = 1000 + i;
                points[i].phase = (i * 1000) % 65536;
            }

            SnapshotStatistics stats;
            stats.time = snap * 0.1f;
            stats.density_mean = 1.0f;

            writer.add_snapshot(points, stats);
        }

        writer.finalize();
    }

    std::cout << "Reading test file..." << std::endl;

    // Read test file
    {
        BECFileReader reader;

        if (!reader.open("test_output.bec")) {
            std::cerr << "Failed to open file" << std::endl;
            return 1;
        }

        std::cout << "File opened successfully" << std::endl;
        std::cout << "  Snapshots: " << reader.get_snapshot_count() << std::endl;

        assert(reader.get_snapshot_count() == n_snapshots);

        // Read first snapshot
        std::vector<PointData> points;
        if (reader.read_snapshot(0, points)) {
            std::cout << "  Read snapshot 0: " << points.size() << " points" << std::endl;
            assert(points.size() == n_points);

            // Verify data
            assert(points[0].coord[0] == 0);
            assert(points[1].coord[0] == 1);
            std::cout << "  Data verification passed" << std::endl;
        } else {
            std::cerr << "Failed to read snapshot" << std::endl;
            return 1;
        }

        reader.close();
    }

    std::cout << "\n✓ All tests passed!" << std::endl;

    return 0;
}
