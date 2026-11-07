/**
 * @file
 * Core mathematical primitives and unit wrappers for the physics simulation.
 *
 * Provides a minimal, standard-layout three-component vector and scalar math
 * helpers tailored for high-performance structure-of-arrays (SoA) memory
 * layouts. Includes explicit dimensional wrappers (metres, seconds, kilograms,
 * metres-per-second) to enforce unit safety at compile time.
 *
 * All operations are dual-targeted to compile and execute on both the host
 * (CPU) and CUDA device without introducing virtual dispatch or runtime overhead.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/scalar.h"

#include <cmath>
#include <cstdint>

#ifdef __CUDACC__
#define SIM_HD __host__ __device__
#else
#define SIM_HD
#endif


namespace sim {

/* Explicit metres wrapper for APIs that need to distinguish distance. */
struct Meters { Real value; };
/* Explicit seconds wrapper for APIs that need to distinguish elapsed time. */
struct Seconds { Real value; };
/* Explicit kilograms wrapper for APIs that need to distinguish mass. */
struct Kg { Real value; };
/* Explicit metres-per-second wrapper for APIs that need to distinguish speed. */
struct Mps { Real value; };

/* Return the smaller scalar. */
SIM_HD inline Real minimum(Real a, Real b) { return a < b ? a : b; }
/* Return the larger scalar. */
SIM_HD inline Real maximum(Real a, Real b) { return a > b ? a : b; }
/* Return the scalar magnitude. */
SIM_HD inline Real absolute(Real a) { return a < 0 ? -a : a; }
/* Limit a scalar to inclusive bounds. */
SIM_HD inline Real clamp(Real a, Real lo, Real hi) { return maximum(lo, minimum(hi, a)); }

/**
 * Three-component vector shared by host and CUDA code.
 *
 * Operators intentionally stay minimal so the type remains standard-layout and
 * inexpensive to pass through structure-of-arrays simulation buffers.
 */
struct Vec3 {
    Real x{}, y{}, z{};
    /* Access a component by axis index (0=X, 1=Y, 2=Z). */
    SIM_HD Real& operator[](int i) { return i == 0 ? x : (i == 1 ? y : z); }
    /* Access a component by axis index (0=X, 1=Y, 2=Z). */
    SIM_HD Real operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
    /* Component-wise addition. */
    SIM_HD Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    /* Component-wise subtraction. */
    SIM_HD Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    /* Component-wise negation. */
    SIM_HD Vec3 operator-() const { return {-x, -y, -z}; }
    /* Scale by a scalar. */
    SIM_HD Vec3 operator*(Real b) const { return {x * b, y * b, z * b}; }
    /* Divide by a non-zero scalar. */
    SIM_HD Vec3 operator/(Real b) const { return *this * (Real(1) / b); }
    /* In-place addition. */
    SIM_HD Vec3& operator+=(Vec3 b) { x += b.x; y += b.y; z += b.z; return *this; }
    /* In-place subtraction. */
    SIM_HD Vec3& operator-=(Vec3 b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
};


/* Component-wise multiplication. */
SIM_HD inline Vec3 multiply(Vec3 a, Vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
/* Scalar projection product (dot product). */
SIM_HD inline Real dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
/* Right-handed cross product. */
SIM_HD inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
/* Euclidean vector length. */
SIM_HD inline Real length(Vec3 a) { return Real(sqrt(dot(a, a))); }
/* Normalize a vector, returning a fallback if the magnitude is numerically zero. */
SIM_HD inline Vec3 normalize(Vec3 a, Vec3 fallback = {1, 0, 0}) { Real n = length(a); return n > settings::Collisions::Geometry::EPSILON ? a / n : fallback; }
/* Component-wise absolute value. */
SIM_HD inline Vec3 absVec(Vec3 a) { return {absolute(a.x), absolute(a.y), absolute(a.z)}; }
/* Check if all components remain within the supported finite numerical range. */
SIM_HD inline bool finite(Vec3 a) { constexpr Real MAX_FINITE_RANGE = Real(1e30); return (absolute(a.x) < MAX_FINITE_RANGE && absolute(a.y) < MAX_FINITE_RANGE && absolute(a.z) < MAX_FINITE_RANGE); }

} // namespace sim
