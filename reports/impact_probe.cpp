/**
 * @file
 * Run a CPU-only simulation probe to track body-to-body impact events.
 *
 * Reads a generated run directory, steps the physics engine for a fixed number
 * of ticks, and reports how many frames contained active impacts, how many of
 * those occurred in the latter half of the simulation, and the total number of
 * retained records.
 */

#include "physics/scenarios/scenario_io.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>


/* Expected number of command-line arguments (program name + run directory). */
constexpr int EXPECTED_ARGS = 2;
constexpr int EXIT_USAGE_ERROR = 2;

/* Simulation parameters. */
constexpr int TOTAL_TICKS = 1200;
constexpr int LATE_TICK_THRESHOLD = 600;


/**
 * Entry point for the impact probe tool.
 *
 * @param argc Argument count.
 * @param argv Argument vector; expects a single path to the run directory.
 * @return Zero on success, non-zero on error.
 */
int main(int argc, char** argv) {
	if (argc != EXPECTED_ARGS) {
		std::cerr << "Usage: " << argv[0] << " <run_directory>\n";
		return EXIT_USAGE_ERROR;
	}

	try {
		// Load the configuration and initial state from the run directory.
		const std::filesystem::path runDir = argv[1];
		std::ifstream configFile(runDir / "config.json");

		if (!configFile) {
			throw std::runtime_error("Cannot open configuration file");
		}

		const auto scenario = sim::decodeConfig(sim::Json::parse(configFile));
		auto state = sim::loadState(runDir / "initial.json");

		// Initialize the single-threaded CPU backend.
		auto backend = sim::createCpuBackend();
		backend->upload(state);
		backend->setImpactTracking(true);

		int framesWithBodyImpacts = 0;
		int lateFrames = 0;
		int retainedRecords = 0;

		// Step the simulation and inspect the impact telemetry each tick.
		for (int tick = 0; tick < TOTAL_TICKS; ++tick) {
			backend->step(state, scenario.world);

			int bodyImpactRecords = 0;

			// Count only confirmed body-to-body impacts (ignore wall hits).
			for (const auto& impact : backend->impacts()) {
				if (impact.wallFace_i < 0 && impact.energy_J > 0.f) {
					++bodyImpactRecords;
				}
			}

			if (bodyImpactRecords > 0) {
				++framesWithBodyImpacts;

				// Check if the impact occurred in the late phase of the run.
				if (tick >= LATE_TICK_THRESHOLD) {
					++lateFrames;
				}

				retainedRecords += bodyImpactRecords;
			}
		}

		std::cout << "frames=" << framesWithBodyImpacts  << " late_frames=" << lateFrames << " retained_records=" << retainedRecords << '\n';
		return 0;

	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << '\n';
		return 1;
	}
}
