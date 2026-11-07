/**
 * @file
 * Shared deterministic host/device solver operations.
 *
 * Inline definitions are required to allow CUDA device compilation while
 * sharing the exact same implementation logic with the host CPU.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/dynamics.h"


namespace sim {

/**
 * Compute the inverse effective mass along an axis, including angular inertia.
 *
 * @param s    Active-body state view.
 * @param ct   Contact constraint defining body IDs, point and normal.
 * @param axis World-space projection/separation direction.
 * @return Scalar result in the simulation's active Real precision.
 */
SIM_HD inline Real effectiveMassInverse(StateView s, const Contact& ct, Vec3 axis) {
    Vec3 ra = ct.point_m - s.position_m[ct.a_i];
    Real result = s.inverseMass_per_kg[ct.a_i] + dot(axis, cross(inverseInertia(s, ct.a_i, cross(ra, axis)), ra));

    if (ct.b_i >= 0) {
        Vec3 rb = ct.point_m - s.position_m[ct.b_i];
        result += s.inverseMass_per_kg[ct.b_i] + dot(axis, cross(inverseInertia(s, ct.b_i, cross(rb, axis)), rb));
    }

    return result;
}


/**
 * Apply a contact impulse to the linear and angular velocities of participating bodies.
 *
 * @param s          Active-body state view.
 * @param ct         Contact constraint defining body IDs, point and normal.
 * @param impulse_Ns World-space impulse in newton-seconds.
 */
SIM_HD inline void applyImpulse(StateView s, const Contact& ct, Vec3 impulse_Ns) {
    int a = ct.a_i, b = ct.b_i;
    s.velocity_mps[a] -= impulse_Ns * s.inverseMass_per_kg[a];
    s.angularVelocity_rad_s[a] -= inverseInertia(s, a, cross(ct.point_m - s.position_m[a], impulse_Ns));

    if (b >= 0) {
        s.velocity_mps[b] += impulse_Ns * s.inverseMass_per_kg[b];
        s.angularVelocity_rad_s[b] += inverseInertia(s, b, cross(ct.point_m - s.position_m[b], impulse_Ns));
    }
}


/**
 * Prepare one contact constraint before iterative solving.
 *
 * Sets contact flags, wakes sleeping pairs if meaningful motion is present,
 * derives the restitution target velocity from pre-solve closing speed, and
 * uses speculative penetration to contribute an approach target for CCD.
 *
 * @param s     Shared body-state view.
 * @param ct    Contact constraint to initialize in place.
 * @param dt_s  Fixed step duration.
 * @param c     Physics configuration.
 */
SIM_HD inline void initializeContact(StateView s, Contact& ct, Real dt_s, const WorldConfig& c) {
    int a = ct.a_i, b = ct.b_i;
    s.hasContact[a] = 1;
    
    if (b >= 0) 
        s.hasContact[b] = 1;

    if (b >= 0 && s.isSleeping[a] != s.isSleeping[b]) {
        int moving = s.isSleeping[a] ? b : a;
        if (length(s.velocity_mps[moving]) > c.sleepLinVel_mps || 
            length(s.angularVelocity_rad_s[moving]) > c.sleepAngVel_rad_s) {
            s.isSleeping[a] = s.isSleeping[b] = 0;
            s.sleepCounter_i[a] = s.sleepCounter_i[b] = 0;
        }
    }

    Real closing = dot(contactVelocity(s, b, ct.point_m) - contactVelocity(s, a, ct.point_m), ct.normal);
    ct.targetVelocity_mps = closing < -c.restitutionSlop_mps ? -c.restitution_unitless * closing : 0;
    
    if (ct.depth_m < -c.contactSlop_m) {
        ct.targetVelocity_mps = minimum(ct.targetVelocity_mps, ct.depth_m / dt_s);
    }
}


/**
 * Apply one sequential-impulse iteration for normal and Coulomb friction constraints.
 *
 * Normal impulses remain non-negative to preserve unilateral contact. Tangent
 * impulse is accumulated and limited by configured static/dynamic friction bounds.
 *
 * @param s  Shared body-state view.
 * @param ct Contact constraint with accumulated impulses.
 * @param c  Physics configuration.
 */
SIM_HD inline void solveContactConstraint(StateView s, Contact& ct, const WorldConfig& c) {
    if (s.isSleeping[ct.a_i] && (ct.b_i < 0 || s.isSleeping[ct.b_i])) 
        return;

    Vec3 relative = contactVelocity(s, ct.b_i, ct.point_m) - contactVelocity(s, ct.a_i, ct.point_m);
    Real massInv = effectiveMassInverse(s, ct, ct.normal);
    
    if (massInv < settings::Collisions::Geometry::EPSILON) 
        return;

    Real increment = (ct.targetVelocity_mps - dot(relative, ct.normal)) / massInv;
    Real previous = ct.normalImpulse_Ns;
    ct.normalImpulse_Ns = maximum(0, previous + increment);

    applyImpulse(s, ct, ct.normal * (ct.normalImpulse_Ns - previous));
    relative = contactVelocity(s, ct.b_i, ct.point_m) - contactVelocity(s, ct.a_i, ct.point_m);

    Vec3 tangent = relative - ct.normal * dot(relative, ct.normal);
    Real speed = length(tangent);
    
    if (speed < settings::Collisions::Geometry::EPSILON) 
        return;

    tangent = tangent / speed;
    Real tangentMass = effectiveMassInverse(s, ct, tangent);
    
    if (tangentMass < settings::Collisions::Geometry::EPSILON) 
        return;

    Vec3 proposed = ct.tangentImpulse_Ns - tangent * (speed / tangentMass);
    Real magnitude = length(proposed);
    
    if (magnitude > c.frictionStatic_unitless * ct.normalImpulse_Ns) {
        proposed = normalize(proposed) * (c.frictionDynamic_unitless * ct.normalImpulse_Ns);
    }

    Vec3 change = proposed - ct.tangentImpulse_Ns;
    ct.tangentImpulse_Ns = proposed;
    applyImpulse(s, ct, change);
}


/**
 * Apply split positional correction for penetrations remaining after velocity solving.
 *
 * The correction changes positions only, avoiding artificial kinetic energy. OBB
 * manifolds scale each point so four-point faces do not over-correct the pair.
 *
 * @param s  Shared body-state view.
 * @param ct Solved contact.
 * @param c  Physics configuration.
 */
SIM_HD inline void correctContactPosition(StateView s, const Contact& ct, const WorldConfig& c) {
    constexpr Real OBB_MANIFOLD_SCALE = Real(.25);
    constexpr Real SINGLE_POINT_SCALE = Real(1);

    Real penetration = maximum(0, ct.depth_m - c.contactSlop_m);
    int a = ct.a_i, b = ct.b_i;

    if (b < 0 && s.shape_i[a] != static_cast<int>(Shape::Sphere)) 
        return;
        
    if (penetration == 0 || (s.isSleeping[a] && (b < 0 || s.isSleeping[b]))) 
        return;

    Real invA = s.inverseMass_per_kg[a], invB = b < 0 ? 0 : s.inverseMass_per_kg[b];
    
    if (invA + invB < settings::Collisions::Geometry::EPSILON) 
        return;

    bool obbPair = s.shape_i[a] != static_cast<int>(Shape::Sphere) &&  (b < 0 || s.shape_i[b] != static_cast<int>(Shape::Sphere));
    Real manifoldScale = obbPair ? OBB_MANIFOLD_SCALE : SINGLE_POINT_SCALE;

    Vec3 correction = ct.normal * (penetration * c.positionCorrection_unitless * manifoldScale / (invA + invB));
    
    s.position_m[a] -= correction * invA;
    if (b >= 0) 
        s.position_m[b] += correction * invB;
}


/**
 * Project a non-sphere OBB back inside static world bounds after integration.
 *
 * Sleeping bodies and spheres are excluded because their wall handling follows
 * the normal contact path.
 *
 * @param s Shared body-state view.
 * @param i Body index.
 * @param c Physics configuration.
 */
SIM_HD inline void correctWallPosition(StateView s, int i, const WorldConfig& c) {
    if (s.isSleeping[i] || s.shape_i[i] == static_cast<int>(Shape::Sphere)) 
        return;

    Vec3 extent = computeExtent(s, i);
    for (int axis = 0; axis < 3; ++axis) {
        for (int side = 0; side < 2; ++side) {
            Real sign = side ? Real(1) : Real(-1);
            Real depth = sign * s.position_m[i][axis] + extent[axis] - c.boxHalfExtents_m[axis];
            
            if (depth > c.contactSlop_m) {
                s.position_m[i][axis] -= sign * (depth - c.contactSlop_m) * c.positionCorrection_unitless;
            }
        }
    }
}

} // namespace sim
