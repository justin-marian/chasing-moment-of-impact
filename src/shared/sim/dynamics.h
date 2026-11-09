/**
 * @file
 * Shared deterministic host/device dynamics operations.
 *
 * Inline definitions are required to allow CUDA device compilation while
 * sharing the exact same implementation logic with the host CPU.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/state.h"


namespace sim {

/**
 * Transform a world-space torque through the body's local inverse inertia tensor.
 *
 * @param s        Active-body state view.
 * @param i        Body index in the active structure-of-arrays range.
 * @param torque_Nm World-space torque in newton-metres.
 * @return Vector result in the same coordinate frame and SI units as the inputs.
 */
SIM_HD inline Vec3 inverseInertia(StateView s, int i, Vec3 torque_Nm) {
    return rotate(
        s.orientation[i],
        multiply(s.inverseInertia_kg_m2[i], rotate(conjugate(s.orientation[i]), torque_Nm))
    );
}


/**
 * Compute world-axis AABB half extents from the sphere radius or rotated OBB.
 *
 * @param s Active-body state view.
 * @param i Body index in the active structure-of-arrays range.
 * @return Vector result in the same coordinate frame and SI units as the inputs.
 */
SIM_HD inline Vec3 computeExtent(StateView s, int i) {
    if (s.shape_i[i] == static_cast<int>(Shape::Sphere)) {
        return {s.halfExtents_m[i].x, s.halfExtents_m[i].x, s.halfExtents_m[i].x};
    }

    Quat q = s.orientation[i];
    Vec3 h = s.halfExtents_m[i];

    return absVec(rotate(q, {1, 0, 0})) * h.x +
           absVec(rotate(q, {0, 1, 0})) * h.y +
           absVec(rotate(q, {0, 0, 1})) * h.z;
}


/**
 * Reset per-step contact flags and apply gravity to awake dynamic bodies.
 *
 * @param s Active-body state view.
 * @param i Body index in the active structure-of-arrays range.
 * @param c Fixed-step world, material and solver settings.
 */
SIM_HD inline void applyForces(StateView s, int i, const WorldConfig& c) {
    s.wallProjection_m[i] = 0;
    s.force_N[i] = {0, 0, 0};
    s.hasContact[i] = 0;

    if (!s.isSleeping[i] && s.inverseMass_per_kg[i] > 0) {
        s.force_N[i].y = -c.gravity_mps2 * s.gravityScale_unitless[i] / s.inverseMass_per_kg[i];
    }
}


/**
 * Integrate force and apply configured linear/angular damping.
 *
 * @param s    Active-body state view.
 * @param i    Body index in the active structure-of-arrays range.
 * @param dt_s Simulation or impact-aging interval in seconds.
 * @param c    Fixed-step world, material and solver settings.
 */
SIM_HD inline void integrateVelocity(StateView s, int i, Real dt_s, const WorldConfig& c) {
    if (s.isSleeping[i]) return;

    s.velocity_mps[i] += s.force_N[i] * (s.inverseMass_per_kg[i] * dt_s);
    s.velocity_mps[i] = s.velocity_mps[i] / (1 + c.linearDamping_per_s * dt_s);
    s.angularVelocity_rad_s[i] = s.angularVelocity_rad_s[i] / (1 + c.angularDamping_per_s * dt_s);
}


/**
 * Refresh a body's cached broad-phase AABB extents.
 *
 * @param s Active-body state view.
 * @param i Body index in the active structure-of-arrays range.
 */
SIM_HD inline void computeAabb(StateView s, int i) {
    s.aabbExtent_m[i] = computeExtent(s, i);
}


/**
 * Project a sphere or oriented bounding box onto the supplied axis.
 *
 * @param s    Active-body state view.
 * @param i    Body index in the active structure-of-arrays range.
 * @param axis World-space projection/separation direction.
 * @return Scalar result in the simulation's active precision.
 */
SIM_HD inline Real projectRadius(StateView s, int i, Vec3 axis) {
    if (s.shape_i[i] == static_cast<int>(Shape::Sphere)) {
        return s.halfExtents_m[i].x;
    }

    Vec3 local = absVec(rotate(conjugate(s.orientation[i]), axis));
    return dot(local, s.halfExtents_m[i]);
}


/**
 * Find the supported face/edge centroid used to place a contact.
 *
 * @param s    Active-body state view.
 * @param i    Body index in the active structure-of-arrays range.
 * @param axis World-space projection/separation direction.
 * @return Vector result in the same coordinate frame and SI units as the inputs.
 */
SIM_HD inline Vec3 supportCentroid(StateView s, int i, Vec3 axis) {
    if (s.shape_i[i] == static_cast<int>(Shape::Sphere)) {
        return s.position_m[i] + axis * s.halfExtents_m[i].x;
    }

    Vec3 local = rotate(conjugate(s.orientation[i]), axis);
    Vec3 h = s.halfExtents_m[i];
    Vec3 p{};

    for (int k = 0; k < 3; ++k) {
        p[k] =
            local[k] > settings::Collisions::Geometry::EPSILON ?
            h[k] : (local[k] < -settings::Collisions::Geometry::EPSILON ? -h[k] : 0);
    }

    return s.position_m[i] + rotate(s.orientation[i], p);
}


/**
 * Compute linear plus angular velocity at a contact point; static walls return zero.
 *
 * @param s       Active-body state view.
 * @param i       Body index in the active structure-of-arrays range.
 * @param point_m World-space contact point in metres.
 * @return Vector result in the same coordinate frame and SI units as the inputs.
 */
SIM_HD inline Vec3 contactVelocity(StateView s, int i, Vec3 point_m) {
    return i < 0 ? Vec3{} : s.velocity_mps[i] + cross(s.angularVelocity_rad_s[i], point_m - s.position_m[i]);
}


/**
 * Advance position and orientation with the resolved velocities.
 *
 * @param s    Active-body state view.
 * @param i    Body index in the active structure-of-arrays range.
 * @param dt_s Simulation or impact-aging interval in seconds.
 */
SIM_HD inline void integratePosition(StateView s, int i, Real dt_s) {
    if (s.isSleeping[i]) return;

    s.position_m[i] += s.velocity_mps[i] * dt_s;
    s.orientation[i] = integrateQuaternion(s.orientation[i], s.angularVelocity_rad_s[i], dt_s);
    computeAabb(s, i);
}


/**
 * Track quiet-contact ticks and sleep eligible bodies using configured thresholds.
 *
 * @param s Active-body state view.
 * @param i Body index in the active structure-of-arrays range.
 * @param c Fixed-step world, material and solver settings.
 */
SIM_HD inline void updateSleep(StateView s, int i, const WorldConfig& c) {
    if (!c.enableSleep) {
        s.isSleeping[i] = 0;
        s.sleepCounter_i[i] = 0;
        return;
    }

    if (s.hasContact[i] &&
        length(s.velocity_mps[i]) < c.sleepLinVel_mps &&
        length(s.angularVelocity_rad_s[i]) < c.sleepAngVel_rad_s) {

        ++s.sleepCounter_i[i];

        if (s.sleepCounter_i[i] >= c.sleepTicks_i) {
            s.isSleeping[i] = 1;
            s.velocity_mps[i] = {};
            s.angularVelocity_rad_s[i] = {};
        }
    } else if (!s.isSleeping[i]) {
        s.sleepCounter_i[i] = 0;
    }
}


/**
 * Apply the final unilateral world-boundary projection after angular integration.
 *
 * This is a safety correction for bodies whose rotated extent escaped the box;
 * the projected distance is retained for diagnostics while velocities remain
 * governed by contact impulses.
 *
 * @param s Shared body-state view.
 * @param i Body index.
 * @param c Physics configuration.
 */
SIM_HD inline void projectWorldConstraints(StateView s, int i, const WorldConfig& c) {
    if (s.isSleeping[i]) return;

    Vec3 extent = s.aabbExtent_m[i];
    Vec3 before = s.position_m[i];

    for (int axis = 0; axis < 3; ++axis) {
        Real limit = c.boxHalfExtents_m[axis] - extent[axis];

        if (s.position_m[i][axis] > limit) {
            s.position_m[i][axis] = limit;
            s.hasContact[i] = 1;
        } else if (s.position_m[i][axis] < -limit) {
            s.position_m[i][axis] = -limit;
            s.hasContact[i] = 1;
        }
    }

    s.wallProjection_m[i] = length(s.position_m[i] - before);
}

} // namespace sim
