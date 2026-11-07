/**
 * @file
 * CPU backend implementation for generating contact constraints.
 */

#include "settings/collisions/constants.h"
#include "physics/backend/cpu/step.h"


namespace sim {

/**
 * Generate supported body/body contacts from the broad-phase candidate lists.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 */
void CpuBackend::bodyContacts(StateView state, const WorldConfig& config) {
	for (int i = 0; i < state.count_i; ++i) {
		int count = 0;

		for (int k = 0; k < candidateCounts[i]; ++k) {
			int j = candidates[static_cast<std::size_t>(i) * settings::Collisions::Storage::MAX_CANDIDATES + k];

			count += pairContacts(
				state, i, j, config.dt_s, config,
				contacts.data() + static_cast<std::size_t>(i) * settings::Collisions::Storage::MAX_CONTACTS + count
			);
		}

		contactCounts[i] = count;
	}
}


/**
 * Generate contacts between active bodies and the six static world-box faces.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 */
void CpuBackend::worldContacts(StateView state, const WorldConfig& config) {
	for (int i = 0; i < state.count_i; ++i) {
		for (int face = 0; face < 6; ++face) {
			for (int corner = 0; corner < 8; ++corner) {
				Contact contact{};

				if (wallVertexContact(state, i, face, corner, config.dt_s, config, contact)) {
					contacts[static_cast<std::size_t>(i) * settings::Collisions::Storage::MAX_CONTACTS + contactCounts[i]++] = contact;
				}
			}
		}
	}
}

} // namespace sim
