/**
 * @file
 * CPU backend lifecycle, state transfer, and execution loop.
 */

#include "settings/compute/constants.h"
#include "settings/collisions/constants.h"
#include "physics/backend/cpu/step.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>


namespace sim {

/**
 * Allocate all fixed-capacity scratch buffers used by the CPU solver.
 *
 * @param capacity Maximum number of bodies processed by this backend.
 */
CpuBackend::CpuBackend(int capacity) : 
    entries(capacity), keys(capacity), ids(capacity),
    candidates(static_cast<std::size_t>(capacity) * settings::Collisions::Storage::MAX_CANDIDATES),
    candidateCounts(capacity), 

    overflow(capacity),
    contacts(static_cast<std::size_t>(capacity) * settings::Collisions::Storage::MAX_CONTACTS),
    contactCounts(capacity), 

    impactMeasures{}, 
    impactEvents{}, 
    trackImpacts(false),

    colorMasks(static_cast<std::size_t>(capacity) * settings::Collisions::Solver::COLOR_MASK_WORDS),
    colors(static_cast<std::size_t>(capacity) * settings::Collisions::Storage::MAX_CONTACTS),
    coloredIndices(static_cast<std::size_t>(capacity) * settings::Collisions::Storage::MAX_CONTACTS),
    colorCounts{}, 
    colorOffsets{} 
{}


/**
 * CPU simulation already operates on host-owned State storage.
 *
 * @param state State that would be transferred by a device backend.
 */
void CpuBackend::upload(const State& state) { (void)state; }
/**
 * CPU simulation already leaves results in host-owned State storage.
 *
 * @param state State that would receive data from a device backend.
 */
void CpuBackend::download(State& state) { (void)state; }


/**
 * Return the stable backend identifier used by reports and CLI selection.
 *
 * @return Backend name.
 */
std::string CpuBackend::name() const { return "Processor Intel(R) Core(TM) i9-14900HX, 2200 Mhz"; }
/**
 * Describe the CPU execution mode.
 *
 * @return Human-readable single-threaded CPU description.
 */
std::string CpuBackend::deviceInfo() const { return "single-threaded CPU"; }


/**
 * Enable or disable optional per-body impact telemetry.
 *
 * Enabling for the first time allocates one measure per contact slot and one
 * retained event bank per body; later toggles reuse that storage.
 *
 * @param enabled Whether impact events should be captured.
 */
void CpuBackend::setImpactTracking(bool enabled) {
	trackImpacts = enabled;

	if (enabled && impactEvents.empty()) {
		impactMeasures.resize(contacts.size());
		impactEvents.resize(contactCounts.size() * settings::Collisions::Impacts::EVENTS_PER_BODY);
	}
}


/**
 * Return the latest retained impact events while tracking is enabled.
 *
 * The span borrows internal storage and remains valid until the backend is
 * destroyed or the telemetry buffers are reallocated.
 *
 * @return Borrowed impact-event span, or empty when tracking is disabled.
 */
std::span<const ImpactEvent> CpuBackend::impacts() const {
	return trackImpacts ? std::span<const ImpactEvent>(impactEvents) : std::span<const ImpactEvent>{};
}


/**
 * Execute one complete fixed simulation tick and time every solver stage.
 *
 * The stage sequence mirrors CUDA: forces, velocity integration, bounds,
 * broad phase, body/world contacts, solving, position integration, sleeping
 * and diagnostics. The authoritative state tick increments exactly once after
 * all stages complete. Numerical failure or broad-phase overflow is reported
 * as an exception rather than returning a partially trusted frame.
 *
 * @param state  Host-owned simulation state advanced in place.
 * @param config Fixed-step solver and world configuration.
 * @return Per-stage timings and final aggregate diagnostics.
 * @throws std::runtime_error If non-finite state or candidate overflow is detected.
 */
StepResult CpuBackend::step(State& state, const WorldConfig& config) {
	auto view = state.view();

	if (trackImpacts) {
		std::fill(impactEvents.begin(), impactEvents.end(), ImpactEvent{});
	}

	using Clock = std::chrono::steady_clock;
	StepResult result{};
	auto start = Clock::now();

	auto timed = [&](int stage, auto function) {
		auto begin = Clock::now();
		function();
		result.stage_wall_ms[stage] = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
	};

	/* Keep stage order identical to the configured stage names and CUDA event timeline. */
	timed(0, [&] { forces(view, config); });
	timed(1, [&] { velocity(view, config); });
	timed(2, [&] { bounds(view); });
	timed(3, [&] { broadPhase(view, config); });
	timed(4, [&] { bodyContacts(view, config); });
	timed(5, [&] { worldContacts(view, config); });
	timed(6, [&] { solver(view, config); });
	timed(7, [&] { position(view, config); });
	timed(8, [&] { sleep(view, config); });
	timed(9, [&] { diagnostics(view, config, result); });

	++state.tick_i;
	result.total_wall_ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

	if (result.diagnostics.nanCount_i || result.diagnostics.overflowCount_i) {
		throw std::runtime_error("CPU frame failed: nonfinite state or candidate capacity exceeded");
	}

	return result;
}


/**
 * Factory function to create the CPU backend.
 *
 * @param capacity_i Maximum number of bodies processed by this backend.
 * @return Unique pointer to the backend.
 */
std::unique_ptr<Backend> createCpuBackend(int capacity_i) {
	return std::make_unique<CpuBackend>(capacity_i);
}

} // namespace sim
