#pragma once

#include "Vector4.h"
#include <cmath>

namespace BEC {

/**
 * Projection modes for 4D to 3D visualization
 */
enum class ProjectionMode {
    Perspective = 0,    // Perspective projection from 4D
    Stereographic = 1,  // Stereographic projection
    Orthogonal = 2      // Simple orthogonal drop of w coordinate
};

/**
 * 4D rotation angles (Euler angles for 4D)
 * Rotations in the XW, YW, and ZW planes
 */
struct Rotation4D {
    float xw_angle;  // Rotation in X-W plane
    float yw_angle;  // Rotation in Y-W plane
    float zw_angle;  // Rotation in Z-W plane

    Rotation4D() : xw_angle(0), yw_angle(0), zw_angle(0) {}
    Rotation4D(float xw, float yw, float zw) : xw_angle(xw), yw_angle(yw), zw_angle(zw) {}
};

/**
 * Projection utilities
 */
class Projector {
public:
    /**
     * Rotate a 4D point by Euler angles
     */
    static Vector4 rotate_4d(const Vector4& p, const Rotation4D& rot) {
        Vector4 result = p;

        // XW rotation
        if (std::abs(rot.xw_angle) > 1e-6f) {
            float c = std::cos(rot.xw_angle);
            float s = std::sin(rot.xw_angle);
            float new_x = c * result.x - s * result.w;
            float new_w = s * result.x + c * result.w;
            result.x = new_x;
            result.w = new_w;
        }

        // YW rotation
        if (std::abs(rot.yw_angle) > 1e-6f) {
            float c = std::cos(rot.yw_angle);
            float s = std::sin(rot.yw_angle);
            float new_y = c * result.y - s * result.w;
            float new_w = s * result.y + c * result.w;
            result.y = new_y;
            result.w = new_w;
        }

        // ZW rotation
        if (std::abs(rot.zw_angle) > 1e-6f) {
            float c = std::cos(rot.zw_angle);
            float s = std::sin(rot.zw_angle);
            float new_z = c * result.z - s * result.w;
            float new_w = s * result.z + c * result.w;
            result.z = new_z;
            result.w = new_w;
        }

        return result;
    }

    /**
     * Project 4D point to 3D
     */
    static Vector3 project_to_3d(const Vector4& p4d, ProjectionMode mode, float distance = 2000.0f) {
        switch (mode) {
            case ProjectionMode::Perspective:
                return project_perspective(p4d, distance);

            case ProjectionMode::Stereographic:
                return project_stereographic(p4d, distance);

            case ProjectionMode::Orthogonal:
            default:
                return project_orthogonal(p4d);
        }
    }

    /**
     * Perspective projection (similar to 3D perspective from 4D)
     */
    static Vector3 project_perspective(const Vector4& p4d, float distance) {
        // Project from 4D to 3D using perspective
        // Scale factor depends on distance from w-plane
        float scale = distance / (distance - p4d.w + 1e-6f);
        return Vector3(p4d.x * scale, p4d.y * scale, p4d.z * scale);
    }

    /**
     * Stereographic projection from 4D hypersphere to 3D space
     */
    static Vector3 project_stereographic(const Vector4& p4d, float R) {
        // Stereographic projection from north pole
        // Maps hypersphere to 3D Euclidean space
        float denominator = R - p4d.w + 1e-6f;
        float scale = R / denominator;
        return Vector3(p4d.x * scale, p4d.y * scale, p4d.z * scale);
    }

    /**
     * Orthogonal projection (simply drop w coordinate)
     */
    static Vector3 project_orthogonal(const Vector4& p4d) {
        return Vector3(p4d.x, p4d.y, p4d.z);
    }

    /**
     * Complete projection pipeline: rotate then project
     */
    static Vector3 transform_and_project(const Vector4& p4d,
                                         const Rotation4D& rotation,
                                         ProjectionMode mode,
                                         float distance = 2000.0f) {
        Vector4 rotated = rotate_4d(p4d, rotation);
        return project_to_3d(rotated, mode, distance);
    }
};

} // namespace BEC
