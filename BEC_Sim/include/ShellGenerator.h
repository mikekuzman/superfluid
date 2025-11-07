#pragma once

#include "Vector4.h"
#include "SimulationParams.h"
#include <vector>

namespace BEC {

/**
 * Generates points on 4D hypersphere shell
 */
class ShellGenerator {
public:
    /**
     * Generate points on shell surface
     * @param params - Simulation parameters
     * @param points - Output vector of 4D points
     * @return Number of points generated
     */
    static size_t generate_shell_points(const SimulationParams& params, std::vector<Vector4>& points);

    /**
     * Generate poles (always at indices 0 and 1)
     * North pole: [0, 0, 0, R]
     * South pole: [0, 0, 0, -R]
     */
    static void add_poles(float R, std::vector<Vector4>& points);

    /**
     * Check if point is on shell
     */
    static bool is_on_shell(const Vector4& p, float R, float delta);

    /**
     * Project point onto hypersphere surface
     */
    static Vector4 project_to_sphere(const Vector4& p, float R);

private:
    /**
     * Generate uniform random points in 4D unit ball
     */
    static Vector4 random_4d_point(float radius);

    /**
     * Normalize to hypersphere surface
     */
    static Vector4 normalize_to_sphere(const Vector4& p, float R);
};

} // namespace BEC
