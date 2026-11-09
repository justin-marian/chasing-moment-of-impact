/**
 * @file
 * Physics simulation command-line runner.
 *
 * Parses command-line options to configure a simulation scenario, executes
 * the CPU or CUDA backend for a specified number of ticks, and optionally
 * logs the run. Supports replaying a recorded trace to verify determinism.
 */

#include "physics/scenarios/scenario_io.h"
#include "shared/utils/command.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

/**
 * CLI configuration for a single simulation run.
 */
struct Options {
    sim::Scenario scenario{sim::matrixScenario("demo")};
    std::string backend{"cpu"};

    int ticks{600};

    std::filesystem::path output{"generated/reports/simulations/latest"};
    std::filesystem::path replay;
};

/**
 * Parse CLI  arguments into an Options struct.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Configured options with validated backend and tick count.
 * @throws std::runtime_error When an argument is missing, unknown, or invalid.
 */
Options parseOptions(int argc, char** argv) {
    constexpr const char* USAGE = "physics --backend cpu|cuda --scenario demo|A|B|C --config FILE --seed N --ticks N --output DIR\n";

    Options options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto next = [&]() {
            if (++i >= argc)
                throw std::runtime_error("Missing value for " + arg);
            return std::string(argv[i]);
        };

        switch (utils::commandKey(arg)) {
            case utils::commandKey("--help"):
            case utils::commandKey("-h"): std::cout << USAGE; std::exit(EXIT_SUCCESS);
            case utils::commandKey("--backend"): options.backend = next(); break;
            case utils::commandKey("--scenario"): options.scenario = sim::matrixScenario(next(), options.scenario.seed); break;
            case utils::commandKey("--config"): options.scenario = sim::loadScenario(next()); break;
            case utils::commandKey("--seed"): options.scenario.seed = std::stoull(next()); break;
            case utils::commandKey("--ticks"): options.ticks = std::stoi(next()); break;
            case utils::commandKey("--output"): options.output = next(); break;
            case utils::commandKey("--replay"): options.replay = next(); break;
            default: throw std::runtime_error("Unknown option: " + arg);
        }
    }

    if (options.backend != "cpu" && options.backend != "cuda")
        throw std::runtime_error("Backend must be cpu or cuda");
    if (options.ticks < 0)
        throw std::runtime_error("Negative ticks");

    return options;
}


/**
 * Read and parse a JSON file from disk.
 *
 * @param path Path to the JSON file.
 * @return Parsed JSON object.
 * @throws std::runtime_error When the file cannot be opened.
 */
sim::Json readJson(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open " + path.string());

    sim::Json json;
    file >> json;
    return json;
}


/**
 * Verify an existing deterministic trace by re-running the simulation.
 *
 * @param backend Backend used to step the simulation.
 * @param path Directory containing config.json, initial.json, and trace.csv.
 * @return Zero when the replay matches perfectly.
 * @throws std::runtime_error When files are missing or the trace mismatches.
 */
int replaySimulation(sim::Backend& backend, const std::filesystem::path& path) {
    auto scenario = sim::decodeConfig(readJson(path / "config.json"));
    auto state = sim::loadState(path / "initial.json");

    backend.upload(state);

    std::ifstream trace(path / "trace.csv");
    if (!trace)
        throw std::runtime_error("Cannot open " + (path / "trace.csv").string());

    std::string line;
    std::getline(trace, line);

    int compared = 0;
    while (std::getline(trace, line)) {
        if (line.empty())
            continue;

        const auto comma = line.find(',');
        if (comma == std::string::npos)
            throw std::runtime_error("Invalid replay trace row");

        const auto tick = std::stoull(line.substr(0, comma));
        const auto expected = std::stoull(line.substr(comma + 1));

        backend.step(state, scenario.world);
        backend.download(state);

        if (state.tick_i != tick || sim::hashState(state) != expected)
            throw std::runtime_error("Replay mismatch at tick " + std::to_string(tick));

        ++compared;
    }

    std::cout << "Replay VERIFIED: " << compared << " ticks, hash " << sim::hashState(state) << '\n';
    return 0;
}


/**
 * Execute a fresh simulation run and write the results to the output directory.
 *
 * @param backend Backend used to step the simulation.
 * @param options Validated run configuration.
 * @return Zero on success, 2 if any body escaped the box during the run.
 */
int runSimulation(sim::Backend& backend, const Options& options) {
    sim::State state;
    sim::populate(state, options.scenario);
    backend.upload(state);

    sim::RunLog log(options.output, options.scenario, backend, state);
    int escapeFrames = 0;

    for (int i = 0; i < options.ticks; ++i) {
        auto result = backend.step(state, options.scenario.world);
        backend.download(state);

        if (!sim::staysInside(state, options.scenario.world))
            ++escapeFrames;

        log.append(state, result, options.scenario.world);
    }

    log.finish(state);

    std::cout
        << backend.name() << ": "
        << state.count_i << " bodies, " << state.tick_i << " ticks, "
        << "hash="  << sim::hashState(state) << ", escapeFrames=" << escapeFrames
        << ", output=" << options.output.string() << '\n';

    return escapeFrames ? 2 : 0;
}

} // namespace (anonymous)


/**
 * Entry point. Selects the backend and dispatches to replay or run mode.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code (0 on success, 1 on error, 2 on escape).
 */
int main(int argc, char** argv) {
    try {
        const auto options = parseOptions(argc, argv);
        auto backend = options.backend == "cuda" ? sim::createCudaBackend() : sim::createCpuBackend();

        if (!options.replay.empty())
            return replaySimulation(*backend, options.replay);

        return runSimulation(*backend, options);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
