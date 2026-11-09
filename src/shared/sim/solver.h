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
	// Calculate the lever arm from body A's center of mass to the contact point.
	Vec3 ra = ct.point_m - s.position_m[ct.a_i];

	// Linear inverse mass plus the angular contribution (r x axis) dot (I^-1 * (r x axis)).
	Real result = s.inverseMass_per_kg[ct.a_i] + dot(axis, cross(inverseInertia(s, ct.a_i, cross(ra, axis)), ra));

	// If body B exists, add its contribution to the effective mass.
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
	int a = ct.a_i;
	int b = ct.b_i;

	// Update body A (subtract impulse).
	s.velocity_mps[a] -= impulse_Ns * s.inverseMass_per_kg[a];
	s.angularVelocity_rad_s[a] -= inverseInertia(s, a, cross(ct.point_m - s.position_m[a], impulse_Ns));

	// Update body B if it exists (add impulse).
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
	int a = ct.a_i;
	int b = ct.b_i;

	// Mark both bodies as having active contacts this frame.
	s.hasContact[a] = 1;
	if (b >= 0) {
		s.hasContact[b] = 1;
	}

	// Wake up sleeping bodies if an active body collides with them.
	if (b >= 0 && s.isSleeping[a] != s.isSleeping[b]) {
		int moving = s.isSleeping[a] ? b : a;
		if (length(s.velocity_mps[moving]) > c.sleepLinVel_mps ||
			length(s.angularVelocity_rad_s[moving]) > c.sleepAngVel_rad_s) {
			s.isSleeping[a] = 0;
			s.isSleeping[b] = 0;
			s.sleepCounter_i[a] = 0;
			s.sleepCounter_i[b] = 0;
		}
	}

	// Calculate the closing speed along the normal.
	Real closing = dot(contactVelocity(s, b, ct.point_m) - contactVelocity(s, a, ct.point_m), ct.normal);

	// Determine the target bounce velocity (restitution).
	ct.targetVelocity_mps = closing < -c.restitutionSlop_mps ? -c.restitution_unitless * closing : 0;

	// Speculative contact handling for CCD: prevent tunneling by targeting an approach velocity.
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
	// Skip solving if both bodies are asleep.
	if (s.isSleeping[ct.a_i] && (ct.b_i < 0 || s.isSleeping[ct.b_i])) {
		return;
	}

	// Calculate relative velocity at the contact point.
	Vec3 relative = contactVelocity(s, ct.b_i, ct.point_m) - contactVelocity(s, ct.a_i, ct.point_m);

	// Get the effective mass along the normal.
	Real massInv = effectiveMassInverse(s, ct, ct.normal);
	if (massInv < settings::Collisions::Geometry::EPSILON) {
		return;
	}

	// Calculate the normal impulse increment needed to reach the target velocity.
	Real increment = (ct.targetVelocity_mps - dot(relative, ct.normal)) / massInv;
	Real previous = ct.normalImpulse_Ns;

	// Clamp the accumulated impulse to be non-negative (unilateral constraint).
	ct.normalImpulse_Ns = maximum(0, previous + increment);

	// Apply the normal impulse change.
	applyImpulse(s, ct, ct.normal * (ct.normalImpulse_Ns - previous));

	// Recalculate relative velocity after normal impulse application.
	relative = contactVelocity(s, ct.b_i, ct.point_m) - contactVelocity(s, ct.a_i, ct.point_m);

	// Extract the tangential velocity component.
	Vec3 tangent = relative - ct.normal * dot(relative, ct.normal);
	Real speed = length(tangent);
	if (speed < settings::Collisions::Geometry::EPSILON) {
		return;
	}

	// Normalize tangent and get effective mass along it.
	tangent = tangent / speed;
	Real tangentMass = effectiveMassInverse(s, ct, tangent);
	if (tangentMass < settings::Collisions::Geometry::EPSILON) {
		return;
	}

	// Calculate proposed friction impulse.
	Vec3 proposed = ct.tangentImpulse_Ns - tangent * (speed / tangentMass);
	Real magnitude = length(proposed);

	// Apply Coulomb friction limit.
	if (magnitude > c.frictionStatic_unitless * ct.normalImpulse_Ns) {
		// If static friction is exceeded, clamp to dynamic friction.
		proposed = normalize(proposed) * (c.frictionDynamic_unitless * ct.normalImpulse_Ns);
	}

	// Apply the friction impulse change.
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
	// Scale factors to prevent over-correction on 4-point OBB manifolds.
	constexpr Real OBB_MANIFOLD_SCALE = Real(.25);
	constexpr Real SINGLE_POINT_SCALE = Real(1);

	// Calculate how much penetration remains after the allowed slop.
	Real penetration = maximum(0, ct.depth_m - c.contactSlop_m);
	int a = ct.a_i;
	int b = ct.b_i;

	// Skip if not penetrating a wall with an OBB.
	if (b < 0 && s.shape_i[a] != static_cast<int>(Shape::Sphere)) {
		return;
	}

	// Skip if there's no penetration or if the bodies are asleep.
	if (penetration == 0 || (s.isSleeping[a] && (b < 0 || s.isSleeping[b]))) {
		return;
	}

	// Get inverse masses.
	Real invA = s.inverseMass_per_kg[a];
	Real invB = b < 0 ? 0 : s.inverseMass_per_kg[b];
	if (invA + invB < settings::Collisions::Geometry::EPSILON) {
		return;
	}

	// Determine scale based on contact type.
	bool obbPair = s.shape_i[a] != static_cast<int>(Shape::Sphere) &&  (b < 0 || s.shape_i[b] != static_cast<int>(Shape::Sphere));
	Real manifoldScale = obbPair ? OBB_MANIFOLD_SCALE : SINGLE_POINT_SCALE;

	// Calculate total correction vector.
	Vec3 correction = ct.normal * (penetration * c.positionCorrection_unitless * manifoldScale / (invA + invB));

	// Apply correction split by inverse mass.
	s.position_m[a] -= correction * invA;
	if (b >= 0) {
		s.position_m[b] += correction * invB;
	}
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
	// Skip spheres and sleeping bodies.
	if (s.isSleeping[i] || s.shape_i[i] == static_cast<int>(Shape::Sphere)) {
		return;
	}

	// Get the AABB extents for this body.
	Vec3 extent = computeExtent(s, i);

	// Check all 3 axes and both sides (positive and negative).
	for (int axis = 0; axis < 3; ++axis) {
		for (int side = 0; side < 2; ++side) {
			Real sign = side ? Real(1) : Real(-1);

			// Calculate penetration depth past the wall.
			Real depth = sign * s.position_m[i][axis] + extent[axis] - c.boxHalfExtents_m[axis];

			// If past the wall plus slop, push the body back in.
			if (depth > c.contactSlop_m) {
				s.position_m[i][axis] -= sign * (depth - c.contactSlop_m) * c.positionCorrection_unitless;
			}
		}
	}
}

} // namespace sim
