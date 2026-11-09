/**
 * @file
 * Scenario loading, deterministic state IO, and recorded simulation runs.
 *
 * Provides utilities for spawning non-overlapping bodies, computing
 * deterministic state hashes, and serializing simulation state to disk.
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

constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;

/**
 * Configuration constants for staggered body spawning.
 */
struct StaggeredSpawn {
    static constexpr int HEIGHT_BANDS = 3;
    static constexpr int PREFERRED_ATTEMPTS = 100;
    static constexpr Real HORIZONTAL_FRACTION = Real(.8);
};


/**
 * Generate a uniformly distributed random real number in [0, 1).
 *
 * @param generator The random engine to pull from.
 * @return The generated float.
 */
Real randomUnit(std::mt19937_64& generator) {
    return static_cast<Real>(std::generate_canonical<double, 53>(generator));
}


/**
 * Generate a uniformly distributed random direction vector on the unit sphere.
 *
 * @param generator The random engine to pull from.
 * @return The unit direction vector.
 */
Vec3 randomDirection(std::mt19937_64& generator) {
    Real z = randomUnit(generator) * 2 - 1;
    Real phi = randomUnit(generator) * 2 * settings::Collisions::Geometry::PI;
    Real radius = Real(sqrt(maximum(0, 1 - z * z)));

    return {radius * Real(cos(phi)), z, radius * Real(sin(phi))};
}

} // namespace (anonymous)


/**
 * Attempt to spawn a body of the given shape at a random non-overlapping position.
 *
 * @param state     The simulation state to spawn into.
 * @param shape     The collision shape to spawn.
 * @param scenario  The scenario defining the world and spawn policies.
 * @param generator The random engine for position and velocity.
 * @return True if the body was successfully spawned, false if capacity is full or volume is exhausted.
 */
bool spawnRandom(State& state, Shape shape, const Scenario& scenario, std::mt19937_64& generator) {
    if (state.count_i == state.capacity_i) {
        return false;
    }

    Body body;
    body.shape = shape;

    Real radius = scenario.radius_m;
    body.halfExtents_m = {radius, radius, radius};
    body.velocity_mps = randomDirection(generator) * (randomUnit(generator) * scenario.maxInitialSpeed_mps);
    body.angularVelocity_rad_s = randomDirection(generator) * (randomUnit(generator) * Real(.8));

    Vec3 axis = randomDirection(generator);
    Real half = randomUnit(generator) * settings::Collisions::Geometry::PI;

    body.orientation = normalize({
        axis.x * Real(sin(half)),
        axis.y * Real(sin(half)),
        axis.z * Real(sin(half)),
        Real(cos(half))
    });

    Real bound = shape == Shape::Sphere ? radius : length(body.halfExtents_m);
    const Vec3 range = scenario.world.boxHalfExtents_m - Vec3{bound, bound, bound};

    /* Attempt random positions until a non-overlapping slot is found. */
    for (int attempt = 0; attempt < 10000; ++attempt) {
        body.position_m = {
            (randomUnit(generator) * 2 - 1) * range.x,
            (randomUnit(generator) * 2 - 1) * range.y,
            (randomUnit(generator) * 2 - 1) * range.z
        };

        if (scenario.staggeredSpawn && attempt < StaggeredSpawn::PREFERRED_ATTEMPTS) {
            /*
             * Alternate height bands so bodies reach the floor at different times.
             * Start closer together horizontally, then fall back to the full box
             * if the preferred region is crowded.
             */
            const int band = state.count_i % StaggeredSpawn::HEIGHT_BANDS;
            body.position_m.x *= StaggeredSpawn::HORIZONTAL_FRACTION;
            body.position_m.z *= StaggeredSpawn::HORIZONTAL_FRACTION;
            body.position_m.y = -range.y +
                (body.position_m.y + range.y + Real(2 * band) * range.y) / Real(StaggeredSpawn::HEIGHT_BANDS);
        }

        bool clear = true;
        for (int i = 0; i < state.count_i; ++i) {
            Real reach = bound + state.boundingRadius_m[i] + Real(.002);
            Vec3 delta = body.position_m - state.position_m[i];

            if (dot(delta, delta) < reach * reach) {
                clear = false;
                break;
            }
        }

        if (clear) {
            return state.spawn(body);
        }
    }

    return false;
}


/**
 * Clear the state and populate it according to the scenario's spawn counts.
 *
 * @param state    The simulation state to populate.
 * @param scenario The scenario defining the required body counts.
 * @throws std::runtime_error If capacity is exhausted or volume cannot accommodate the bodies.
 */
void populate(State& state, const Scenario& scenario) {
    state.clear();
    std::mt19937_64 generator(scenario.seed);

    for (int shape = 0; shape < 4; ++shape) {
        for (int i = 0; i < scenario.counts[shape]; ++i) {
            if (!spawnRandom(state, static_cast<Shape>(shape), scenario, generator)) {
                throw std::runtime_error("Spawn exhausted body capacity or available nonoverlapping volume");
            }
        }
    }
}


/**
 * Compute a deterministic FNV-1a hash representing the current simulation state.
 *
 * @param state The state to hash.
 * @return The 64-bit hash value.
 */
std::uint64_t hashState(const State& state) {
    std::uint64_t hash = FNV_OFFSET_BASIS;

    auto append = [&](const auto& value) {
        auto* bytes = reinterpret_cast<const unsigned char*>(&value);
        for (std::size_t i = 0; i < sizeof(value); ++i) {
            hash ^= bytes[i];
            hash *= FNV_PRIME;
        }
    };

    append(state.count_i);
    append(state.tick_i);

    for (int i = 0; i < state.count_i; ++i) {
        append(state.position_m[i]);
        append(state.velocity_mps[i]);
        append(state.angularVelocity_rad_s[i]);
        append(state.orientation[i]);
        append(state.halfExtents_m[i]);
        append(state.inverseMass_per_kg[i]);
        append(state.gravityScale_unitless[i]);
        append(state.shape_i[i]);
        append(state.sleepCounter_i[i]);
        append(state.isSleeping[i]);
    }

    return hash;
}


/**
 * Compute a deterministic FNV-1a hash of a text string.
 *
 * @param text The text to hash.
 * @return The hash value formatted as a hexadecimal string.
 */
std::string hashText(std::string_view text) {
    std::uint64_t hash = FNV_OFFSET_BASIS;

    for (unsigned char value : text) {
        hash ^= value;
        hash *= FNV_PRIME;
    }

    std::ostringstream out;
    out << std::hex << hash;
    return out.str();
}


/**
 * Serialize the simulation state to a JSON file on disk.
 *
 * @param path  Destination file path.
 * @param state State to serialize.
 * @throws std::runtime_error If the file cannot be written.
 */
void saveState(const std::filesystem::path& path, const State& state) {
    Json json = {
        {"tick_i", state.tick_i},
        {"bodies", Json::array()}
    };

    for (int i = 0; i < state.count_i; ++i) {
        json["bodies"].push_back({
            {"position_m", vectorJson(state.position_m[i])},
            {"velocity_mps", vectorJson(state.velocity_mps[i])},
            {"angularVelocity_rad_s", vectorJson(state.angularVelocity_rad_s[i])},
            {"orientation", Json::array({
                state.orientation[i].x,
                state.orientation[i].y,
                state.orientation[i].z,
                state.orientation[i].w
            })},
            {"halfExtents_m", vectorJson(state.halfExtents_m[i])},
            {"mass_kg", 1 / state.inverseMass_per_kg[i]},
            {"gravityScale_unitless", state.gravityScale_unitless[i]},
            {"shape_i", state.shape_i[i]},
            {"sleepCounter_i", state.sleepCounter_i[i]},
            {"isSleeping", state.isSleeping[i]}
        });
    }

    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Cannot write state file " + path.string());
    }

    out << json.dump(2);
}


/**
 * Deserialize simulation state from a JSON file on disk.
 *
 * @param path Source file path.
 * @return The loaded simulation state.
 * @throws std::runtime_error If the file cannot be read or contains invalid bodies.
 */
State loadState(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot read state file " + path.string());
    }

    Json json;
    in >> json;

    State state;
    state.tick_i = json.at("tick_i");

    for (const auto& value : json.at("bodies")) {
        Body body;
        body.position_m = jsonVector(value.at("position_m"));
        body.velocity_mps = jsonVector(value.at("velocity_mps"));
        body.angularVelocity_rad_s = jsonVector(value.at("angularVelocity_rad_s"));
        body.halfExtents_m = jsonVector(value.at("halfExtents_m"));

        const auto& q = value.at("orientation");
        body.orientation = {
            q.at(0).get<Real>(),
            q.at(1).get<Real>(),
            q.at(2).get<Real>(),
            q.at(3).get<Real>()
        };

        body.mass_kg.value = value.at("mass_kg");
        body.gravityScale_unitless = value.at("gravityScale_unitless");
        body.shape = static_cast<Shape>(value.at("shape_i").get<int>());

        if (!state.spawn(body)) {
            throw std::runtime_error("Invalid state body");
        }

        int i = state.count_i - 1;
        state.orientation[i] = body.orientation;
        state.sleepCounter_i[i] = value.at("sleepCounter_i");
        state.isSleeping[i] = value.at("isSleeping");
    }

    return state;
}


/**
 * Export the motion state of all bodies to a CSV file.
 *
 * @param path  Destination file path.
 * @param state The state to export.
 * @throws std::runtime_error If the file cannot be written.
 */
void printMotion(const std::filesystem::path& path, const State& state) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Cannot write motion CSV " + path.string());
    }

    out << "tick_i,body_i,shape_i,x_m,y_m,z_m,vx_mps,vy_mps,vz_mps,wx_rad_s,wy_rad_s,wz_rad_s\n" << std::setprecision(9);

    for (int i = 0; i < state.count_i; ++i) {
        auto position = state.position_m[i];
        auto velocity = state.velocity_mps[i];
        auto angular = state.angularVelocity_rad_s[i];

        out << state.tick_i
            << ',' << i
            << ',' << state.shape_i[i]
            << ',' << position.x
            << ',' << position.y
            << ',' << position.z
            << ',' << velocity.x
            << ',' << velocity.y
            << ',' << velocity.z
            << ',' << angular.x
            << ',' << angular.y
            << ',' << angular.z
            << '\n';
    }
}


/**
 * Check if all bodies remain within the world bounding box.
 *
 * @param state       The simulation state to check.
 * @param world       The world configuration defining the bounding box.
 * @param tolerance_m Allowed penetration distance.
 * @return True if all bodies are inside, false otherwise.
 */
bool staysInside(const State& state, const WorldConfig& world, Real tolerance_m) {
    for (int i = 0; i < state.count_i; ++i) {
        Vec3 position = absVec(state.position_m[i]);
        Vec3 extent = state.aabbExtent_m[i];

        for (int axis = 0; axis < 3; ++axis) {
            if (position[axis] + extent[axis] > world.boxHalfExtents_m[axis] + tolerance_m) {
                return false;
            }
        }
    }

    return true;
}

} // namespace sim
