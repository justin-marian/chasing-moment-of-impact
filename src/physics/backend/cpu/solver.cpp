/**
 * @file
 * CPU backend implementation for the sequential impulse solver.
 */

#include "physics/backend/cpu/step.h"

#include <stdexcept>


namespace sim {

/**
 * Resolve the colored contact constraints using the configured iteration count.
 *
 * Contacts of one graph color never share a dynamic body. CUDA solves that group
 * in parallel; retaining the same color and iteration order here gives both
 * backends the same dependencies.
 *
 * @param state  Borrowed active-body arrays shared by the CPU and CUDA implementation.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 * @throws std::runtime_error If the contact graph exceeds the maximum supported colors.
 */
void CpuBackend::solver(StateView state, const WorldConfig& config) {
	int colorCount = colorContacts(
		state, contacts.data(), contactCounts.data(),
        colorMasks.data(), colors.data(), coloredIndices.data(),
		colorCounts.data(), colorOffsets.data()
	);

	if (colorCount < 0) {
		throw std::runtime_error("Contact graph exceeds 256 colors");
	}

	for (int color = 0; color < colorCount; ++color) {
		for (int k = colorOffsets[color]; k < colorOffsets[color + 1]; ++k) {
			int index = coloredIndices[k];
			initializeContact(state, contacts[index], config.dt_s, config);

			if (trackImpacts) {
				impactMeasures[index] = measureImpact(state, contacts[index]);
			}
		}
	}

	for (int iteration = 0; iteration < config.solverIterations_i; ++iteration) {
		for (int color = 0; color < colorCount; ++color) {
			for (int k = colorOffsets[color]; k < colorOffsets[color + 1]; ++k) {
				solveContactConstraint(state, contacts[coloredIndices[k]], config);
			}
		}
	}

	if (trackImpacts) {
		for (int color = 0; color < colorCount; ++color) {
			for (int k = colorOffsets[color]; k < colorOffsets[color + 1]; ++k) {
				int index = coloredIndices[k];
				recordImpact(state, contacts[index], impactMeasures[index], config, impactEvents.data());
			}
		}
	}

	for (int color = 0; color < colorCount; ++color) {
		for (int k = colorOffsets[color]; k < colorOffsets[color + 1]; ++k) {
			correctContactPosition(state, contacts[coloredIndices[k]], config);
		}
	}

	for (int i = 0; i < state.count_i; ++i) {
		correctWallPosition(state, i, config);
	}
}

} // namespace sim
