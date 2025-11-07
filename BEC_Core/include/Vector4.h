#pragma once

#include <cmath>
#include <ostream>

namespace BEC {

/**
 * 4D vector for hypersphere coordinates (w, x, y, z)
 */
struct Vector4 {
    float w, x, y, z;

    Vector4() : w(0), x(0), y(0), z(0) {}
    Vector4(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}

    // Vector operations
    Vector4 operator+(const Vector4& other) const {
        return Vector4(w + other.w, x + other.x, y + other.y, z + other.z);
    }

    Vector4 operator-(const Vector4& other) const {
        return Vector4(w - other.w, x - other.x, y - other.y, z - other.z);
    }

    Vector4 operator*(float scalar) const {
        return Vector4(w * scalar, x * scalar, y * scalar, z * scalar);
    }

    Vector4 operator/(float scalar) const {
        return Vector4(w / scalar, x / scalar, y / scalar, z / scalar);
    }

    // Compound assignment
    Vector4& operator+=(const Vector4& other) {
        w += other.w; x += other.x; y += other.y; z += other.z;
        return *this;
    }

    Vector4& operator-=(const Vector4& other) {
        w -= other.w; x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }

    Vector4& operator*=(float scalar) {
        w *= scalar; x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    // Dot product
    float dot(const Vector4& other) const {
        return w * other.w + x * other.x + y * other.y + z * other.z;
    }

    // Magnitude
    float magnitude() const {
        return std::sqrt(w * w + x * x + y * y + z * z);
    }

    float magnitude_squared() const {
        return w * w + x * x + y * y + z * z;
    }

    // Normalize
    Vector4 normalized() const {
        float mag = magnitude();
        if (mag < 1e-10f) return Vector4(0, 0, 0, 0);
        return *this / mag;
    }

    void normalize() {
        float mag = magnitude();
        if (mag > 1e-10f) {
            *this /= mag;
        }
    }

    // Distance
    static float distance(const Vector4& a, const Vector4& b) {
        return (a - b).magnitude();
    }

    // Linear interpolation
    static Vector4 lerp(const Vector4& a, const Vector4& b, float t) {
        return a * (1.0f - t) + b * t;
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Vector4& v) {
        os << "(" << v.w << ", " << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

/**
 * 3D vector for visualization
 */
struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator/(float scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    float dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3 cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    float magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    float magnitude_squared() const {
        return x * x + y * y + z * z;
    }

    Vector3 normalized() const {
        float mag = magnitude();
        if (mag < 1e-10f) return Vector3(0, 0, 0);
        return *this / mag;
    }

    void normalize() {
        float mag = magnitude();
        if (mag > 1e-10f) {
            *this /= mag;
        }
    }
};

} // namespace BEC
