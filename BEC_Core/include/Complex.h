#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <complex>

// Define M_PI if not defined (for MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace BEC {

/**
 * Complex number operations for wavefunction
 * Using std::complex but with additional utility functions
 */
using Complex = std::complex<double>;
using ComplexFloat = std::complex<float>;

/**
 * Complex number utilities
 */
namespace ComplexOps {
    // Phase extraction
    inline double phase(const Complex& c) {
        return std::arg(c);
    }

    // Magnitude squared (more efficient than abs() when we don't need sqrt)
    inline double abs_squared(const Complex& c) {
        return c.real() * c.real() + c.imag() * c.imag();
    }

    // Create complex from magnitude and phase
    inline Complex from_polar(double magnitude, double phase) {
        return std::polar(magnitude, phase);
    }

    // Normalize to unit magnitude
    inline Complex normalize(const Complex& c) {
        double mag = std::abs(c);
        if (mag < 1e-10) return Complex(0, 0);
        return c / mag;
    }

    // Safe division
    inline Complex safe_divide(const Complex& numerator, const Complex& denominator) {
        double denom_abs_sq = abs_squared(denominator);
        if (denom_abs_sq < 1e-20) {
            return Complex(0, 0);
        }
        return numerator / denominator;
    }

    // Phase difference (wrapped to [-π, π])
    inline double phase_difference(const Complex& a, const Complex& b) {
        double diff = phase(a) - phase(b);
        while (diff > M_PI) diff -= 2.0 * M_PI;
        while (diff < -M_PI) diff += 2.0 * M_PI;
        return diff;
    }

    // Gradient of phase (for velocity calculation)
    inline double phase_gradient(const Complex& center, const Complex& neighbor) {
        return phase_difference(neighbor, center);
    }
}

/**
 * Wavefunction representation
 */
struct Wavefunction {
    Complex psi;  // Complex wavefunction value

    Wavefunction() : psi(0, 0) {}
    Wavefunction(const Complex& p) : psi(p) {}
    Wavefunction(double real, double imag) : psi(real, imag) {}

    // Physical quantities
    double density() const {
        return ComplexOps::abs_squared(psi);
    }

    double phase() const {
        return ComplexOps::phase(psi);
    }

    double magnitude() const {
        return std::abs(psi);
    }

    // Normalize
    void normalize() {
        psi = ComplexOps::normalize(psi);
    }

    // From polar form
    static Wavefunction from_polar(double magnitude, double phase) {
        return Wavefunction(ComplexOps::from_polar(magnitude, phase));
    }
};

} // namespace BEC
