#include "ShellGenerator.h"
#include <random>
#include <cmath>
#include <iostream>

namespace BEC {

size_t ShellGenerator::generate_shell_points(const SimulationParams& params, std::vector<Vector4>& points) {
    points.clear();

    // Add poles first (indices 0 and 1)
    add_poles(params.R, points);

    // Setup random number generator
    std::mt19937 rng(params.random_seed);
    std::uniform_real_distribution<float> uniform(-1.0f, 1.0f);
    std::normal_distribution<float> normal(0.0f, 1.0f);

    // Target number of points
    uint32_t target_points = params.estimate_n_points();

    std::cout << "Generating " << target_points << " shell points..." << std::endl;
    std::cout << "  R = " << params.R << ", delta = " << params.delta << std::endl;

    float R_inner = params.R - params.delta / 2.0f;
    float R_outer = params.R + params.delta / 2.0f;

    size_t attempts = 0;
    size_t max_attempts = target_points * 100;  // Try many times

    while (points.size() < target_points + 2 && attempts < max_attempts) {
        // Generate random 4D point using normal distribution
        // This gives uniform distribution on hypersphere
        Vector4 p(
            normal(rng),
            normal(rng),
            normal(rng),
            normal(rng)
        );

        // Normalize to unit hypersphere
        p.normalize();

        // Scale to random radius within shell
        std::uniform_real_distribution<float> radius_dist(R_inner, R_outer);
        float r = radius_dist(rng);
        p *= r;

        // Add point
        points.push_back(p);
        attempts++;

        if (points.size() % 1000 == 0) {
            std::cout << "  Generated " << points.size() - 2 << " points..." << std::endl;
        }
    }

    std::cout << "Generated " << points.size() - 2 << " shell points ";
    std::cout << "(plus 2 poles = " << points.size() << " total)" << std::endl;

    return points.size();
}

void ShellGenerator::add_poles(float R, std::vector<Vector4>& points) {
    // North pole at index 0: [0, 0, 0, R]
    points.push_back(Vector4(0, 0, 0, R));

    // South pole at index 1: [0, 0, 0, -R]
    points.push_back(Vector4(0, 0, 0, -R));

    std::cout << "Added poles:" << std::endl;
    std::cout << "  North (index 0): " << points[0] << std::endl;
    std::cout << "  South (index 1): " << points[1] << std::endl;
}

bool ShellGenerator::is_on_shell(const Vector4& p, float R, float delta) {
    float r = p.magnitude();
    float R_inner = R - delta / 2.0f;
    float R_outer = R + delta / 2.0f;
    return (r >= R_inner && r <= R_outer);
}

Vector4 ShellGenerator::project_to_sphere(const Vector4& p, float R) {
    Vector4 normalized = p.normalized();
    return normalized * R;
}

Vector4 ShellGenerator::random_4d_point(float radius) {
    // Not used in final implementation
    return Vector4(0, 0, 0, 0);
}

Vector4 ShellGenerator::normalize_to_sphere(const Vector4& p, float R) {
    return p.normalized() * R;
}

} // namespace BEC
