/**
 * @file
 * CPU backend implementation for computing bounding volumes.
 */

#include "physics/backend/cpu/step.h"


namespace sim {

/**
 * Refresh cached world-axis bounds before the broad-phase contact search.
 *
 * @param state Borrowed active-body arrays shared by the CPU and CUDA implementation.
 */
void CpuBackend::bounds(StateView state) {
	for (int i = 0; i < state.count_i; ++i) {
		computeAabb(state, i);
	}
}

} // namespace sim
