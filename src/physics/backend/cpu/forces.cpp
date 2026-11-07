/**
 * @file
 * CPU backend implementation for applying forces.
 */

#include "physics/backend/cpu/step.h"


namespace sim {

/**
 * Apply configured external forces and reset transient contact flags for the active bodies.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 */
void CpuBackend::forces(StateView state, const WorldConfig& config) {
	for (int i = 0; i < state.count_i; ++i) {
		applyForces(state, i, config);
	}
}

} // namespace sim
