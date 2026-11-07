/**
 * @file
 * CPU backend implementation for broad-phase collision detection.
 */

#include "settings/collisions/constants.h"
#include "physics/backend/cpu/step.h"

#include <algorithm>


namespace sim {

/**
 * Build bounded candidate lists from stable spatial-cell keys and body ordering.
 *
 * Sorting (cell key, body ID) makes each grid cell contiguous and breaks ties
 * deterministically. Candidate gathering can then search nearby cells without
 * scanning every body pair.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 */
void CpuBackend::broadPhase(StateView state, const WorldConfig& config) {
	Real maxRadius = 0;
	Real maxSpeed = 0;

	for (int i = 0; i < state.count_i; ++i) {
		maxRadius = maximum(maxRadius, state.boundingRadius_m[i]);
		maxSpeed = maximum(maxSpeed, length(state.velocity_mps[i]));
	}

	for (int i = 0; i < state.count_i; ++i) {
		entries[i] = {bodyCellKey(state, i, config), i};
	}

	std::sort(entries.begin(), entries.begin() + state.count_i);

	for (int i = 0; i < state.count_i; ++i) {
		keys[i] = entries[i].first;
		ids[i] = entries[i].second;
	}

	for (int i = 0; i < state.count_i; ++i) {
		overflow[i] = 0;
		candidateCounts[i] = gatherCandidates(
			state, i, config, config.dt_s, maxRadius, maxSpeed, keys.data(), ids.data(),
			candidates.data() + static_cast<std::size_t>(i) * settings::Collisions::Storage::MAX_CANDIDATES,
			overflow[i]
		);
	}
}

} // namespace sim
