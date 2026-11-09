/**
 * @file
 * Share quaternion composition, normalization and integration between CPU and CUDA.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/vector.h"


namespace sim {

/**
 * Quaternion orientation stored as vector part (x,y,z) and scalar part w.
 * Default construction yields the identity rotation.
 */
struct Quat {
    Real x{}, y{}, z{}, w{1};

    /**
     * Compose this quaternion with the operand in the declared multiplication order.
     *
     * @param b Second quaternion operand in the declared composition order.
     * @return Quaternion product in operand order.
     */
    SIM_HD Quat operator*(Quat b) const {
        return {
            w * b.x + x * b.w + y * b.z - z * b.y,
            w * b.y - x * b.z + y * b.w + z * b.x,
            w * b.z + x * b.y - y * b.x + z * b.w,
            w * b.w - x * b.x - y * b.y - z * b.z
        };
    }
};


/**
 * Reverse the vector part for inverse unit-quaternion rotation.
 *
 * @param a First quaternion operand in the declared composition order.
 * @return Quaternion result in the project's X, Y, Z, W component order.
 */
SIM_HD inline Quat conjugate(Quat a) { return {-a.x, -a.y, -a.z, a.w}; }


/**
 * Normalize a quaternion, returning identity if the magnitude is numerically zero.
 *
 * @param a First quaternion operand in the declared composition order.
 * @return Quaternion result in the project's X, Y, Z, W component order.
 */
SIM_HD inline Quat normalize(Quat a) {
    Real n = Real(sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w));
    return n > settings::Collisions::Geometry::EPSILON ? Quat{a.x / n, a.y / n, a.z / n, a.w / n} : Quat{};
}


/**
 * Rotate a vector using the quaternion cross-product form.
 *
 * @param q Normalized quaternion describing the world-space rotation.
 * @param v Vector to rotate using the supplied quaternion.
 * @return Vector result in the same coordinate frame and SI units as the inputs.
 */
SIM_HD inline Vec3 rotate(Quat q, Vec3 v) {
    Vec3 t = cross({q.x, q.y, q.z}, v) * 2;
    return v + t * q.w + cross({q.x, q.y, q.z}, t);
}


/**
 * Integrate world-space angular velocity and renormalize orientation.
 *
 * @param q Current orientation.
 * @param omega_rad_s World-space angular velocity in radians per second.
 * @param dt_s Step duration in seconds.
 * @return Quaternion result in the project's X, Y, Z, W component order.
 */
SIM_HD inline Quat integrateQuaternion(Quat q, Vec3 omega_rad_s, Real dt_s) {
    Quat d = Quat{omega_rad_s.x, omega_rad_s.y, omega_rad_s.z, 0} * q;
    Real h = dt_s * Real(.5);
    return normalize(Quat{q.x + d.x * h, q.y + d.y * h, q.z + d.z * h, q.w + d.w * h});
}

} // namespace sim
