/**
 * @file
 * CPU backend implementation for updating sleeping bodies.
 */

#include "physics/backend/cpu/step.h"


namespace sim {

/**
 * Update quiet-body counters and sleeping flags after contact solving.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 */
void CpuBackend::sleep(StateView state, const WorldConfig& config) {
	for (int i = 0; i < state.count_i; ++i) {
		updateSleep(state, i, config);
	}
}

} // namespace sim
