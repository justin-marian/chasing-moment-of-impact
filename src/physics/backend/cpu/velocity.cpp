/**
 * @file
 * CPU backend implementation for integrating velocities.
 */

#include "physics/backend/cpu/step.h"


namespace sim {

/**
 * Integrate forces and configured damping into awake body velocities.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 */
void CpuBackend::velocity(StateView state, const WorldConfig& config) {
	for (int i = 0; i < state.count_i; ++i) {
		integrateVelocity(state, i, config.dt_s, config);
	}
}

} // namespace sim
