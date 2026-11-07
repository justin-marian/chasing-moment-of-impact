/**
 * @file
 * CPU backend implementation for integrating positions.
 */

#include "physics/backend/cpu/step.h"


namespace sim {

/**
 * Integrate body placement and apply the final world-boundary position constraint.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 */
void CpuBackend::position(StateView state, const WorldConfig& config) {
	for (int i = 0; i < state.count_i; ++i) {
		integratePosition(state, i, config.dt_s);
		projectWorldConstraints(state, i, config);
	}
}

} // namespace sim
