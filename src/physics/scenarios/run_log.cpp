/**
 * @file
 * Scenario loading, deterministic state IO, and recorded simulation runs.
 *
 * Provides utilities for writing execution frame logs, trace hashes, and
 * run metadata to disk.
 */

#include "settings/compute/constants.h"
#include "physics/scenarios/scenario_io.h"
#include "shared/sim/physics.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "shared/utils/json.h"


namespace sim {

using utils::jsonVector;
using utils::vectorJson;


namespace {

/**
 * Get the current UTC time formatted as an ISO 8601 string.
 *
 * @return The formatted timestamp string.
 */
std::string utcNow() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm value{};

#ifdef _WIN32
    gmtime_s(&value, &time);
#else
    gmtime_r(&time, &value);
#endif

    std::ostringstream out;
    out << std::put_time(&value, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}


/**
 * Build the run metadata JSON object.
 *
 * Keeps run identity and measurement context together. Solver parameters
 * are stored in the separate configuration file.
 *
 * @param scenario The scenario being executed.
 * @param backend  The backend executing the scenario.
 * @param config   The encoded scenario configuration.
 * @return JSON object containing the metadata.
 */
Json runMetadata(const Scenario& scenario, const Backend& backend, const Json& config) {
    Json data = {
        {"backend", backend.name()},
        {"device", backend.deviceInfo()},
        {"configHash", hashText(config.dump())},
        {"sourceSha256", SIM_SOURCE_SHA256},
        {"startUtc", utcNow()},
        {"seed", scenario.seed},
        {"cpuLogicalCores_i", std::thread::hardware_concurrency()},
        {"realBytes_i", sizeof(Real)}
    };

#ifdef _MSC_VER
    data["compiler"] = "MSVC " + std::to_string(_MSC_VER);
#else
    data["compiler"] = __VERSION__;
#endif

#ifdef _WIN32
    data["os"] = "Windows";

    char cpu[256]{};
    DWORD cpuSize = GetEnvironmentVariableA("PROCESSOR_IDENTIFIER", cpu, sizeof(cpu));
    data["cpuModel"] = cpuSize && cpuSize < sizeof(cpu) ? cpu : "unknown";

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        data["ramBytes_i"] = memory.ullTotalPhys;
    }
#else
    data["os"] = "Linux/Unix";
#endif

    return data;
}

} // namespace (anonymous)


/**
 * Initialize the run log, creating the output directory and header files.
 *
 * @param dir       Output directory path.
 * @param scenario  Scenario being executed.
 * @param backend   Backend executing the scenario.
 * @param initial   Initial simulation state.
 * @throws std::runtime_error If the output files cannot be created.
 */
RunLog::RunLog(
    const std::filesystem::path& dir,
    const Scenario& scenario, const Backend& backend,
    const State& initial
) : directory(dir) {
    std::filesystem::create_directories(directory);

    Json config = encodeConfig(scenario);

    std::ofstream configFile(directory / "config.json");
    if (!configFile) {
        throw std::runtime_error("Cannot write run configuration");
    }
    configFile << config.dump(2);

    saveState(directory / "initial.json", initial);

    frames.open(directory / "frames.csv");
    trace.open(directory / "trace.csv");
    if (!frames || !trace) {
        throw std::runtime_error("Cannot open run CSV");
    }

    trace << "tick_i,stateHash\n";

    /* Write the CSV header matching the exact row layout in append(). */
    frames << "tick_i,t_s,dt_s";
    for (auto name : settings::Compute::Stages::NAMES) {
        frames << ",stage_" << name << "wall_ms";
    }

    frames
        << ",total_wall_ms"
        << ",activeBodies_i"
        << ",sleepingBodies_i"
        << ",contacts_i"
        << ",candidatePairs_i"
        << ",kineticEnergy_J"
        << ",potentialEnergy_J"
        << ",totalEnergy_J"
        << ",linearMomentum_kgmps"
        << ",nanCount_i"
        << ",maxSpeed_mps"
        << ",projectedBodies_i"
        << ",maxWallProjection_m\n";

    metadata = runMetadata(scenario, backend, config);
}


/**
 * Append one simulation step's data to the frame and trace logs.
 *
 * @param state  Current simulation state.
 * @param result Step result containing diagnostics and timings.
 * @param config World configuration for time step access.
 */
void RunLog::append(const State& state, const StepResult& result, const WorldConfig& config) {
    const auto& d = result.diagnostics;

    /* Write the row values strictly matching the header order. */
    frames
        << std::setprecision(12)
        << state.tick_i << ','
        << static_cast<double>(state.tick_i) * config.dt_s << ','
        << config.dt_s;

    for (double ms : result.stage_wall_ms) {
        frames << ',' << ms;
    }

    frames
        << ',' << result.total_wall_ms
        << ',' << d.activeBodies_i
        << ',' << d.sleepingBodies_i
        << ',' << d.contacts_i
        << ',' << d.candidatePairs_i
        << ',' << d.kineticEnergy_J
        << ',' << d.potentialEnergy_J
        << ',' << d.kineticEnergy_J + d.potentialEnergy_J
        << ',' << length(d.linearMomentum_kgmps)
        << ',' << d.nanCount_i
        << ',' << d.maxSpeed_mps
        << ',' << d.projectedBodies_i
        << ',' << d.maxWallProjection_m
        << '\n';

    trace << state.tick_i << ',' << hashState(state) << '\n';
}


/**
 * Finalize the run log by writing the manifest and final state.
 *
 * @param state Final simulation state.
 * @throws std::runtime_error If the metadata file cannot be created.
 */
void RunLog::finish(const State& state) {
    metadata["endUtc"] = utcNow();
    metadata["ticks_i"] = state.tick_i;
    metadata["bodyCount_i"] = state.count_i;
    metadata["replayHash"] = std::to_string(hashState(state));

    std::ofstream metadataFile(directory / "manifest.json");
    if (!metadataFile) {
        throw std::runtime_error("Cannot write run metadata");
    }
    metadataFile << metadata.dump(2);

    saveState(directory / "final.json", state);
    printMotion(directory / "motion.csv", state);
}

} // namespace sim
