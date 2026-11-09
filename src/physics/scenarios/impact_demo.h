/**
 * @file
 * Build and populate the selectable deterministic glass-physics scenarios.
 *
 * Provides controlled collision scenarios for native impact previews and captures.
 * The built-in impact demo is a purpose-built setup: three real
 * sphere/box collisions at 1, 4, and 9 m/s, with no gravity, no damping, and
 * no sleeping, so the impact palette ramp is visible end-to-end in one frame.
 */

#pragma once

#include "physics/scenarios/scenario_io.h"


namespace app {

/**
 * Return the scenario for a viewer session.
 *
 * If the identifier matches the built-in demo, returns the specialized setup;
 * otherwise forwards to the matrix scenario loader.
 *
 * @param id   Scenario identifier.
 * @param seed Visual and physics seed.
 * @return Fully configured scenario.
 */
sim::Scenario viewerScenario(const std::string& id, std::uint64_t seed);


/**
 * Populate a state from a scenario.
 *
 * For the built-in impact demo, builds the three-projectile setup; otherwise
 * delegates to the standard population routine.
 *
 * @param state State to overwrite.
 * @param scene Scenario to read.
 */
void populateViewer(sim::State& state, const sim::Scenario& scene);

} // namespace app
