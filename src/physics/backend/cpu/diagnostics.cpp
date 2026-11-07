/**
 * @file
 * CPU backend implementation for aggregating step diagnostics.
 */

#include "physics/backend/cpu/step.h"


namespace sim {

/**
 * Collect final energy, motion, contact and numerical-validity diagnostics for this step.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 * @param result Step result receiving stage timings, contact diagnostics and validation counters.
 */
void CpuBackend::diagnostics(StateView state, const WorldConfig& config, StepResult& result) {
	result.diagnostics = emitDiagnostics(state, config);

	for (int i = 0; i < state.count_i; ++i) {
		result.diagnostics.contacts_i += contactCounts[i];
		result.diagnostics.candidatePairs_i += candidateCounts[i];
		result.diagnostics.overflowCount_i += overflow[i];
	}
}

} // namespace sim
