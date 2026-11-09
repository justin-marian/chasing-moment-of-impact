/**
 * @file
 * Validate scenario configuration and serialize
 * deterministic state, diagnostics, and replay artifacts.
 */

#pragma once

#include "shared/sim/backend.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>


namespace sim {

using Json = nlohmann::json;


/**
 * Deterministic simulation scenario configuration.
 */
struct Scenario {
    std::string id{"demo"};
    std::uint64_t seed{42};

    /* Sphere, cube, cylinder and cone counts. */
    std::array<int, 4> counts{40, 40, 20, 20};
    /* Shared body radius used by deterministic population. */
    Real radius_m{Real(.35)};
    /* Maximum initial linear speed assigned during population. */
    Real maxInitialSpeed_mps{5};
    /* World bounds, material response and solver configuration. */
    WorldConfig world{};

    /* Prefer staggered heights and a central footprint when placing demo bodies. */
    bool staggeredSpawn{};
};


/**
 * Read and validate a supported flat scenario configuration.
 *
 * @param path Scenario file to load.
 * @return The decoded scenario.
 * @throws std::runtime_error If the file or configuration is invalid.
 */
Scenario loadScenario(const std::filesystem::path& path);


/**
 * Build one predefined matrix scenario with the requested deterministic seed.
 *
 * @param id   Matrix identifier: "demo", "A", "B" or "C".
 * @param seed Deterministic population seed.
 * @return The configured scenario.
 * @throws std::runtime_error For an unknown matrix identifier.
 */
Scenario matrixScenario(std::string_view id, std::uint64_t seed = 42);


/**
 * Populate simulation state from the configured shape counts and seed.
 *
 * @param state    State to replace.
 * @param scenario Population and world configuration.
 */
void populate(State& state, const Scenario& scenario);


/**
 * Try to append one randomly generated body inside the scenario bounds.
 *
 * @param state     State receiving the new body.
 * @param shape     Shape to generate.
 * @param scenario  Spawn constraints and world configuration.
 * @param generator Deterministic random generator.
 * @return True when a body was successfully appended.
 */
bool spawnRandom(State& state, Shape shape, const Scenario& scenario, std::mt19937_64& generator);


/**
 * Hash deterministic simulation fields in recorded order.
 *
 * @param state Host simulation arrays, active body count and authoritative tick in SI units.
 * @return Deterministic hash of the configured simulation fields in recorded order.
 */
std::uint64_t hashState(const State& state);


/**
 * Return the stable textual hash used by recorded configuration data.
 *
 * @param text Configuration text whose stable digest is recorded in run metadata.
 * @return Stable hexadecimal digest of the input configuration text.
 */
std::string hashText(std::string_view text);


/**
 * Serialize the complete scenario and solver configuration.
 *
 * @param scenario Complete deterministic population, world and solver configuration.
 * @return Serialized scenario and solver fields needed for reproducible playback.
 */
Json encodeConfig(const Scenario& scenario);


/**
 * Decode and validate a recorded scenario configuration.
 *
 * @param json Serialized configuration or state document to validate and decode.
 * @return Validated population and world configuration decoded from the document.
 * @throws std::runtime_error If the document is invalid or missing required fields.
 */
Scenario decodeConfig(const Json& json);


/**
 * Write one complete simulation-state checkpoint.
 *
 * @param path  Filesystem path of the state, configuration or image resource being written.
 * @param state Host simulation arrays, active body count and authoritative tick in SI units.
 * @throws std::runtime_error If the file cannot be written.
 */
void saveState(const std::filesystem::path& path, const State& state);


/**
 * Read and validate one simulation-state checkpoint.
 *
 * @param path Filesystem path of the state, configuration or image resource being read.
 * @return Owned host state reconstructed from the checkpoint.
 * @throws std::runtime_error If the file cannot be read or contains invalid data.
 */
State loadState(const std::filesystem::path& path);


/**
 * Export body transform and velocity data as a motion CSV.
 *
 * @param path  Destination CSV path for body transforms and velocities.
 * @param state Host simulation arrays, active body count and authoritative tick in SI units.
 * @throws std::runtime_error If the file cannot be written.
 */
void printMotion(const std::filesystem::path& path, const State& state);


/**
 * Check every body against the configured world bounds.
 *
 * @param state       Simulation state to inspect.
 * @param world       World bounds and solver configuration.
 * @param tolerance_m Allowed distance beyond the nominal bounds.
 * @return True when every body remains inside the accepted region.
 */
bool staysInside(const State& state, const WorldConfig& world, Real tolerance_m = Real(.03));


/**
 * Timing, replay trace, checkpoints and metadata for one deterministic run.
 */
class RunLog {
    /* Directory containing all files produced by this run. */
    std::filesystem::path directory;
    /* Per-step diagnostics state-hash trace. */
    std::ofstream frames, trace;
    /* Environment metadata written when the run finishes. */
    Json metadata;

public:
    /**
     * Begin recording one simulation run.
     *
     * @param directory Output directory for the run.
     * @param scenario  Scenario and solver configuration.
     * @param backend   Backend executing the simulation.
     * @param initial   Initial state written for later replay.
     */
    RunLog(
        const std::filesystem::path& directory,
        const Scenario& scenario,
        const Backend& backend,
        const State& initial
    );


    /**
     * Append one simulation step to the timing and replay traces.
     *
     * @param state  State after the completed step.
     * @param result Step timings and diagnostics.
     * @param config Solver configuration used for the step.
     */
    void append(const State& state, const StepResult& result, const WorldConfig& config);


    /**
     * Finish the recorded run and write its final state, motion and metadata.
     *
     * @param state Final simulation state.
     */
    void finish(const State& state);
};

} // namespace sim
