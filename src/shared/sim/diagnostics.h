/**
 * @file
 * Shared deterministic host/device diagnostics operations.
 *
 * Inline definitions are required to allow CUDA device compilation while
 * sharing the exact same implementation logic with the host CPU.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/dynamics.h"


namespace sim {

/**
 * Aggregate energy, momentum, activity, numerical and boundary-projection diagnostics.
 *
 * Values are computed from the final state of the current step and use SI units.
 *
 * @param s Shared body-state view.
 * @param c Physics configuration.
 * @return Aggregate diagnostics across all active bodies.
 */
SIM_HD inline Diagnostics emitDiagnostics(StateView s, const WorldConfig& c) {
	constexpr Real HALF = Real(.5);

	Diagnostics d{};

	for (int i = 0; i < s.count_i; ++i) {
		// Boundary projections.
		d.maxWallProjection_m = maximum(d.maxWallProjection_m, s.wallProjection_m[i]);
		if (s.wallProjection_m[i] > settings::Collisions::Geometry::EPSILON) {
			++d.projectedBodies_i;
		}

		// Numerical stability.
		if (!finite(s.position_m[i]) || 
			!finite(s.velocity_mps[i]) || 
			!finite(s.angularVelocity_rad_s[i])) {
			++d.nanCount_i;
		}

		// Mass and angular properties.
		Real mass = s.inverseMass_per_kg[i] > 0 ? 1 / s.inverseMass_per_kg[i] : 0;
		Vec3 local = rotate(conjugate(s.orientation[i]), s.angularVelocity_rad_s[i]);
		Vec3 inv = s.inverseInertia_kg_m2[i];

		Real angular = 
			(inv.x > 0 ? local.x * local.x / inv.x : 0) +
			(inv.y > 0 ? local.y * local.y / inv.y : 0) +
			(inv.z > 0 ? local.z * local.z / inv.z : 0);

		// Energy and momentum.
		d.kineticEnergy_J += (mass * dot(s.velocity_mps[i], s.velocity_mps[i]) + angular) * HALF;
		d.potentialEnergy_J += mass * c.gravity_mps2 * s.gravityScale_unitless[i] * (s.position_m[i].y + c.boxHalfExtents_m.y);
		d.linearMomentum_kgmps += s.velocity_mps[i] * mass;

		// Activity.
		d.maxSpeed_mps = maximum(d.maxSpeed_mps, length(s.velocity_mps[i]));
		
		if (s.isSleeping[i]) {
			++d.sleepingBodies_i;
		} else {
			++d.activeBodies_i;
		}
	}
	
	return d;
}

} // namespace sim
