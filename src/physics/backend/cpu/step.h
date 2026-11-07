/**
 * @file
 * Declare the staged CPU implementation of the shared fixed-step simulation pipeline.
 */

#pragma once

#include "settings/collisions/constants.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "shared/sim/backend.h"
#include "shared/sim/physics.h"


namespace sim {

/**
 * Single-threaded reference implementation of the fixed rigid-body pipeline.
 *
 * The CPU backend executes the same ten stages and contact-coloring schedule as
 * CUDA so diagnostics, determinism checks and backend comparisons observe the
 * same simulation ordering rather than two independently structured solvers.
 *
 * State storage remains host-owned: upload() and download() are therefore no-ops,
 * while step() operates directly on State through its non-owning StateView.
 */
class CpuBackend final : public Backend {
	/* Stable broad-phase entries and their sorted cell-key/body-ID representation. */
	std::vector<std::pair<std::uint64_t, int>> entries;
	std::vector<std::uint64_t> keys;
	std::vector<int> ids;

	/* Per-body broad-phase candidates and validation counters. */
	std::vector<int> candidates, candidateCounts, overflow;

	/* Fixed per-body contact storage and valid contact counts. */
	std::vector<Contact> contacts;
	std::vector<int> contactCounts;

	/* Optional impact telemetry captured around contact solving. */
	std::vector<ImpactMeasure> impactMeasures;
	std::vector<ImpactEvent> impactEvents;
	bool trackImpacts;

	/* Conflict-free contact coloring storage shared with the CUDA schedule. */
	std::vector<std::uint32_t> colorMasks;
	std::vector<int> colors, coloredIndices;
	std::array<int, settings::Collisions::Solver::MAX_SOLVER_COLORS> colorCounts;
	std::array<int, settings::Collisions::Solver::MAX_SOLVER_COLORS + 1> colorOffsets;

	/**
	 * Apply configured external forces and reset transient contact flags.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 */
	void forces(StateView state, const WorldConfig& config);


	/**
	 * Integrate forces and configured damping into awake body velocities.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 */
	void velocity(StateView state, const WorldConfig& config);


	/**
	 * Refresh cached world-axis bounds before the broad-phase contact search.
	 *
	 * @param state Borrowed active-body arrays.
	 */
	void bounds(StateView state);


	/**
	 * Build bounded candidate lists from stable spatial-cell keys and body ordering.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 */
	void broadPhase(StateView state, const WorldConfig& config);


	/**
	 * Generate supported body/body contacts from the broad-phase candidate lists.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 */
	void bodyContacts(StateView state, const WorldConfig& config);


	/**
	 * Generate contacts between active bodies and the six static world-box faces.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 */
	void worldContacts(StateView state, const WorldConfig& config);


	/**
	 * Resolve the colored contact constraints using the configured iteration count.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 */
	void solver(StateView state, const WorldConfig& config);


	/**
	 * Integrate body placement and apply the final world-boundary position constraint.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 */
	void position(StateView state, const WorldConfig& config);


	/**
	 * Update quiet-body counters and sleeping flags after contact solving.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 */
	void sleep(StateView state, const WorldConfig& config);


	/**
	 * Collect final energy, motion, contact and numerical-validity diagnostics.
	 *
	 * @param state  Borrowed active-body arrays.
	 * @param config Fixed-step world, material and solver settings.
	 * @param result Step result receiving stage timings and validation counters.
	 */
	void diagnostics(StateView state, const WorldConfig& config, StepResult& result);

public:
	/**
	 * Allocate all fixed-capacity scratch buffers used by the CPU solver.
	 *
	 * @param capacity Maximum number of bodies processed by this backend.
	 */
	explicit CpuBackend(int capacity);


	/**
	 * CPU simulation already operates on host-owned State storage.
	 *
	 * @param state State that would be transferred by a device backend.
	 */
	void upload(const State& state) override;


	/**
	 * CPU simulation already leaves results in host-owned State storage.
	 *
	 * @param state State that would receive data from a device backend.
	 */
	void download(State& state) override;


	/**
	 * Return the stable backend identifier used by reports and CLI selection.
	 *
	 * @return Backend name.
	 */
	std::string name() const override;


	/**
	 * Describe the CPU execution mode.
	 *
	 * @return Human-readable single-threaded CPU description.
	 */
	std::string deviceInfo() const override;


	/**
	 * Enable or disable optional per-body impact telemetry.
	 *
	 * @param enabled Whether impact events should be captured.
	 */
	void setImpactTracking(bool enabled) override;


	/**
	 * Return the latest retained impact events while tracking is enabled.
	 *
	 * @return Borrowed impact-event span, or empty when tracking is disabled.
	 */
	std::span<const ImpactEvent> impacts() const override;


	/**
	 * Execute one complete fixed simulation tick and time every solver stage.
	 *
	 * @param state  Host-owned simulation state advanced in place.
	 * @param config Fixed-step solver and world configuration.
	 * @return Per-stage timings and final aggregate diagnostics.
	 * @throws std::runtime_error if non-finite state or candidate overflow is detected.
	 */
	StepResult step(State& state, const WorldConfig& config) override;
};

} // namespace sim
