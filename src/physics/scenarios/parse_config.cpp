/**
 * @file
 * Scenario loading, deterministic state IO, and recorded simulation runs.
 *
 * Provides utilities for parsing flat YAML configuration files, validating
 * physical constraints, generating deterministic matrix scenarios, and
 * encoding/decoding state to JSON.
 */

#include "settings/collisions/constants.h"
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
 * Helper to strictly parse and validate a scenario configuration value.
 */
struct ScenarioValue {
    const std::string& key;
    const std::string& text;
    std::size_t used{};


    /**
     * Parse the text as a finite real number.
     *
     * @return Parsed floating-point value.
     * @throws std::runtime_error If the text contains trailing characters or is not finite.
     */
    Real number() {
        Real value = static_cast<Real>(std::stod(text, &used));

        if (used != text.size() || !std::isfinite(value)) {
            throw std::runtime_error("Invalid number: " + key);
        }

        return value;
    }


    /**
     * Parse the text as an integer.
     *
     * @return Parsed integer value.
     * @throws std::runtime_error If the text contains trailing characters.
     */
    int integer() {
        int value = std::stoi(text, &used);

        if (used != text.size()) {
            throw std::runtime_error("Invalid integer: " + key);
        }

        return value;
    }


    /**
     * Parse the text as an unsigned integer.
     *
     * @return Parsed unsigned integer value.
     * @throws std::runtime_error If the text contains trailing characters.
     */
    std::uint64_t unsignedInteger() {
        auto value = std::stoull(text, &used);

        if (used != text.size()) {
            throw std::runtime_error("Invalid integer: " + key);
        }

        return value;
    }


    /**
     * Parse the text as a boolean.
     *
     * @return True for "true", false for "false".
     * @throws std::runtime_error If the text is neither.
     */
    bool boolean() const {
        if (text == "true") {
            return true;
        }
        if (text == "false") {
            return false;
        }
        throw std::runtime_error("Invalid boolean: " + key);
    }
};


/**
 * Read a specific field from the JSON object into the provided value.
 *
 * @tparam T The expected field type.
 * @param json The source JSON object.
 * @param name The field name.
 * @param value The destination variable.
 */
template <class T>
void readField(const Json& json, const char* name, T& value) {
    value = json.at(name).get<T>();
}


/**
 * Remove leading and trailing whitespace characters.
 *
 * @param value The string to trim.
 * @return The trimmed string.
 */
std::string trim(std::string value) {
    auto first = value.find_first_not_of(" \t\r");

    if (first == std::string::npos) {
        return {};
    }

    auto last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}


/**
 * Resolve a string key to its corresponding scenario field enum.
 *
 * @param key The string key to resolve.
 * @return The matched field enum.
 * @throws std::runtime_error If the key is unknown.
 */
settings::Collisions::Scenario::Field scenarioField(std::string_view key) {
    auto it = std::find_if(
        std::begin(settings::Collisions::Scenario::FIELDS),
        std::end(settings::Collisions::Scenario::FIELDS),
        [key](const auto& field) { return field.first == key; }
    );

    if (it == std::end(settings::Collisions::Scenario::FIELDS)) {
        throw std::runtime_error("Unknown YAML key: " + std::string(key));
    }

    return it->second;
}


/**
 * Apply a parsed string value to the appropriate field in the scenario.
 *
 * @param s    The scenario to modify.
 * @param key  The configuration key name.
 * @param text The raw value string.
 */
void setScenarioValue(Scenario& s, const std::string& key, const std::string& text) {
    ScenarioValue value{key, text};
    switch (scenarioField(key)) {
        case settings::Collisions::Scenario::Field::Id: s.id = text; break;
        case settings::Collisions::Scenario::Field::Seed: s.seed = value.unsignedInteger(); break;
        case settings::Collisions::Scenario::Field::Spheres: s.counts[0] = value.integer(); break;
        case settings::Collisions::Scenario::Field::Cubes: s.counts[1] = value.integer(); break;
        case settings::Collisions::Scenario::Field::Cylinders: s.counts[2] = value.integer(); break;
        case settings::Collisions::Scenario::Field::Cones: s.counts[3] = value.integer(); break;
        case settings::Collisions::Scenario::Field::Radius: s.radius_m = value.number(); break;
        case settings::Collisions::Scenario::Field::MaxInitialSpeed: s.maxInitialSpeed_mps = value.number(); break;
        case settings::Collisions::Scenario::Field::BoxX: s.world.boxHalfExtents_m.x = value.number(); break;
        case settings::Collisions::Scenario::Field::BoxY: s.world.boxHalfExtents_m.y = value.number(); break;
        case settings::Collisions::Scenario::Field::BoxZ: s.world.boxHalfExtents_m.z = value.number(); break;
        case settings::Collisions::Scenario::Field::Dt: s.world.dt_s = value.number(); break;
        case settings::Collisions::Scenario::Field::Gravity: s.world.gravity_mps2 = value.number(); break;
        case settings::Collisions::Scenario::Field::FrictionStatic: s.world.frictionStatic_unitless = value.number(); break;
        case settings::Collisions::Scenario::Field::FrictionDynamic: s.world.frictionDynamic_unitless = value.number(); break;
        case settings::Collisions::Scenario::Field::Restitution: s.world.restitution_unitless = value.number(); break;
        case settings::Collisions::Scenario::Field::LinearDamping: s.world.linearDamping_per_s = value.number(); break;
        case settings::Collisions::Scenario::Field::AngularDamping: s.world.angularDamping_per_s = value.number(); break;
        case settings::Collisions::Scenario::Field::SolverIterations: s.world.solverIterations_i = value.integer(); break;
        case settings::Collisions::Scenario::Field::CellSize: s.world.cellSize_m = value.number(); break;
        case settings::Collisions::Scenario::Field::EnableSleep: s.world.enableSleep = value.boolean(); break;
        case settings::Collisions::Scenario::Field::EnableCcd: s.world.enableCcd = value.boolean(); break;
    }
}


/**
 * Validate the logical and physical constraints of a loaded scenario.
 *
 * @param s The scenario to validate.
 * @throws std::runtime_error If any parameter falls outside its allowed bounds.
 */
void validateScenario(const Scenario& s) {
    int total = 0;

    for (int count : s.counts) {
        if (count < 0) {
            throw std::runtime_error("Negative body count");
        }
        total += count;
    }

    if (total > settings::Collisions::Storage::MAX_BODIES) { throw std::runtime_error("Scenario exceeds body capacity"); }
    if (s.world.dt_s <= 0 || s.world.dt_s > Real(.1)) { throw std::runtime_error("Scenario time step outside valid bounds"); }
    if (s.radius_m <= 0) { throw std::runtime_error("Scenario radius must be positive"); }
    if (s.maxInitialSpeed_mps < 0) { throw std::runtime_error("Scenario initial speed cannot be negative"); }
    if (s.world.cellSize_m <= 0) { throw std::runtime_error("Scenario cell size must be positive"); }
    if (s.world.solverIterations_i <= 0 || s.world.solverIterations_i > 100) { throw std::runtime_error("Scenario solver iterations outside valid bounds"); }
    if (s.world.restitution_unitless < 0 || s.world.restitution_unitless > 1) { throw std::runtime_error("Scenario restitution must be in [0, 1]"); }
    if (s.world.frictionDynamic_unitless < 0 || s.world.frictionStatic_unitless < s.world.frictionDynamic_unitless) { throw std::runtime_error("Scenario friction values are invalid"); }
    if (s.world.linearDamping_per_s < 0 || s.world.angularDamping_per_s < 0) { throw std::runtime_error("Scenario damping cannot be negative"); }

    Real smallestHalfExtent = minimum(s.world.boxHalfExtents_m.x,  minimum(s.world.boxHalfExtents_m.y, s.world.boxHalfExtents_m.z));
    if (smallestHalfExtent <= s.radius_m * 2) { throw std::runtime_error("Scenario box is too small for configured bodies"); }
}

} // namespace (anonymous)


/**
 * Generate a predefined matrix scenario by identifier.
 *
 * The demo scenario brings the side walls closer while preserving the full
 * bounce height.
 *
 * @param id   The scenario identifier ("A", "B", "C", or "demo").
 * @param seed The deterministic seed.
 * @return The generated scenario.
 * @throws std::runtime_error If the identifier is unknown.
 */
Scenario matrixScenario(std::string_view id, std::uint64_t seed) {
    Scenario s;
    s.id = id;
    s.seed = seed;

    if (id == "A") { s.counts = {100, 250, 250, 250}; }
    else if (id == "B") { s.counts = {250, 500, 500, 500}; }
    else if (id == "C") { s.counts = {500, 1000, 1250, 1250}; }
    else if (id == "demo") {
        s.world.boxHalfExtents_m = {12, 15, 12};
        s.radius_m = Real(.60);
        s.maxInitialSpeed_mps = Real(6.5);
        s.world.restitution_unitless = Real(.62);
        s.staggeredSpawn = true;
    } else {
        throw std::runtime_error("Unknown scenario: " + std::string(id));
    }

    return s;
}


/**
 * Load a scenario from a flat YAML configuration file.
 *
 * @param path The filesystem path to the YAML file.
 * @return The validated scenario.
 * @throws std::runtime_error If the file cannot be read or parsing fails.
 */
Scenario loadScenario(const std::filesystem::path& path) {
    std::ifstream in(path);

    if (!in) {
        throw std::runtime_error("Cannot read scenario " + path.string());
    }

    Scenario scenario;
    std::string line;
    std::set<std::string> seen;

    while (std::getline(in, line)) {
        /* Strip comments before parsing one flat YAML key/value pair. */
        auto comment = line.find('#');
        if (comment != std::string::npos) {
            line.resize(comment);
        }

        if (line.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }

        auto colon = line.find(':');
        if (colon == std::string::npos) {
            throw std::runtime_error("Expected flat YAML key: value");
        }

        auto key = trim(line.substr(0, colon));
        auto value = trim(line.substr(colon + 1));

        if (!seen.insert(key).second) {
            throw std::runtime_error("Duplicate YAML key: " + key);
        }

        setScenarioValue(scenario, key, value);
    }

    validateScenario(scenario);
    return scenario;
}


/**
 * Encode a scenario into a JSON object.
 *
 * @param s The scenario to encode.
 * @return The JSON representation.
 */
Json encodeConfig(const Scenario& s) {
    const auto& c = s.world;

    Json config{
        /* Identity and counts. */
        {"id", s.id},
        {"counts", s.counts},
        {"seed", s.seed},
        /* Geometry and initial state. */
        {"radius_m", s.radius_m},
        {"maxInitialSpeed_mps", s.maxInitialSpeed_mps},
        {"boxHalfExtents_m", vectorJson(c.boxHalfExtents_m)},
        /* World physics. */
        {"dt_s", c.dt_s},
        {"gravity_mps2", c.gravity_mps2},
        {"frictionStatic_unitless", c.frictionStatic_unitless},
        {"frictionDynamic_unitless", c.frictionDynamic_unitless},
        {"restitution_unitless", c.restitution_unitless},
        {"restitutionSlop_mps", c.restitutionSlop_mps},
        {"linearDamping_per_s", c.linearDamping_per_s},
        {"angularDamping_per_s", c.angularDamping_per_s},
        /* Solver and collision settings. */
        {"cellSize_m", c.cellSize_m},
        {"contactSlop_m", c.contactSlop_m},
        {"positionCorrection_unitless", c.positionCorrection_unitless},
        {"sleepLinVel_mps", c.sleepLinVel_mps},
        {"sleepAngVel_rad_s", c.sleepAngVel_rad_s},
        {"sleepTicks_i", c.sleepTicks_i},
        {"solverIterations_i", c.solverIterations_i},
        {"enableSleep", c.enableSleep},
        {"enableCcd", c.enableCcd}
    };

    if (s.staggeredSpawn) {
        config["staggeredSpawn"] = true;
    }

    return config;
}


/**
 * Decode a scenario from a JSON object.
 *
 * @param json The JSON representation to decode.
 * @return The validated scenario.
 * @throws std::runtime_error If decoding fails or validation fails.
 */
Scenario decodeConfig(const Json& json) {
    Scenario s;
    s.id = json.at("id");
    s.counts = json.at("counts").get<std::array<int, 4>>();
    s.seed = json.at("seed");
    s.radius_m = json.at("radius_m");
    s.maxInitialSpeed_mps = json.at("maxInitialSpeed_mps");
    s.staggeredSpawn = json.value("staggeredSpawn", false);

    auto& c = s.world;
    c.boxHalfExtents_m = jsonVector(json.at("boxHalfExtents_m"));

    /* World physics. */
    readField(json, "dt_s", c.dt_s);
    readField(json, "gravity_mps2", c.gravity_mps2);
    readField(json, "frictionStatic_unitless", c.frictionStatic_unitless);
    readField(json, "frictionDynamic_unitless", c.frictionDynamic_unitless);
    readField(json, "restitution_unitless", c.restitution_unitless);
    readField(json, "restitutionSlop_mps", c.restitutionSlop_mps);
    readField(json, "linearDamping_per_s", c.linearDamping_per_s);
    readField(json, "angularDamping_per_s", c.angularDamping_per_s);

    /* Solver and collision settings. */
    readField(json, "cellSize_m", c.cellSize_m);
    readField(json, "contactSlop_m", c.contactSlop_m);
    readField(json, "positionCorrection_unitless", c.positionCorrection_unitless);
    readField(json, "sleepLinVel_mps", c.sleepLinVel_mps);
    readField(json, "sleepAngVel_rad_s", c.sleepAngVel_rad_s);
    readField(json, "sleepTicks_i", c.sleepTicks_i);
    readField(json, "solverIterations_i", c.solverIterations_i);
    readField(json, "enableSleep", c.enableSleep);
    readField(json, "enableCcd", c.enableCcd);

    validateScenario(s);
    return s;
}

} // namespace sim
