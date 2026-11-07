/**
 * @file
 * Track and age visual impact patches for bodies and static walls.
 *
 * Merges nearby impact events into unified hot regions and applies
 * time-based decay to determine visual strength.
 */

#include "settings/collisions/constants.h"
#include "settings/world/constants.h"
#include "settings/world/render_constants.h"
#include "client/render_gl/impact_history.h"

#include <algorithm>
#include <cmath>


namespace render {

/**
 * Merge a nearby hit or replace a free or old retained patch.
 *
 * @param slots The patch slots for a single body or wall face.
 * @param patch The new impact patch to insert.
 */
void ImpactHistory::insert(Slots& slots, const ImpactPatch& patch) {
    auto* target = &slots[0];

    for (auto& p : slots) {
        /* Nearby manifold points represent one physical hot region. */
        if (
            p.energy_J > 0 && 
            sim::dot(p.normal, patch.normal) > settings::World::Rendering::Impacts::History::MERGE_NORMAL_DOT &&
            sim::length(p.point_m - patch.point_m) < patch.radius_m * settings::World::Rendering::Impacts::History::MERGE_RADIUS_FRACTION
        ) {
            if (patch.energy_J >= p.energy_J || p.age_s > settings::World::Rendering::Impacts::History::REPLACE_AGE_S) {
                p = patch;
            }
            return;
        }

        if (p.energy_J == 0) {
            target = &p;
            break;
        }

        if (p.age_s > target->age_s) {
            target = &p;
        }
    }

    *target = patch;
}


/**
 * Construct with a fixed per-body capacity.
 *
 * @param capacity Maximum number of bodies to track.
 */
ImpactHistory::ImpactHistory(int capacity) : bodies(std::size_t(capacity)) {}


/**
 * Clear retained patches and reported impact values.
 */
void ImpactHistory::clear() {
    std::fill(bodies.begin(), bodies.end(), Slots{});
    walls = {};
    lastEnergy_J = lastClosing_mps = lastImpulse_Ns = 0;
}


/**
 * Return normalized visual strength for one retained patch.
 *
 * @param patch The impact patch to evaluate.
 * @return Normalized strength in the range [0, 1].
 */
float ImpactHistory::strength(const ImpactPatch& patch) const {
    constexpr float UNIT_RANGE = 1.f;

    if (patch.energy_J <= 0 || patch.age_s >= lifetime_s) {
        return 0;
    }

    float energy = std::clamp(std::log1p(patch.energy_J) / std::log1p(redEnergy_J), 0.f, UNIT_RANGE);
    
    return energy * std::pow(
        std::max(0.f, 1 - patch.age_s / lifetime_s),
        settings::World::Rendering::Impacts::History::DECAY_EXPONENT
    );
}


/**
 * Age retained patches and consume confirmed per-body impacts.
 *
 * @param state  Current simulation state.
 * @param events Latest confirmed impact events.
 * @param dt_s   Time step in seconds.
 */
void ImpactHistory::advance(const sim::State& state, std::span<const sim::ImpactEvent> events, float dt_s) {
    constexpr float PATCH_AGE_START = 0.0f;
    constexpr float UNIT_RANGE = 1.f;

    auto age = [&](Slots& slots) {
        for (auto& patch : slots) {
            if (!hold) {
                patch.age_s += dt_s;
            }
            if (patch.age_s >= lifetime_s) {
                patch = {};
            }
        }
    };

    for (int i = 0; i < state.count_i; ++i) {
        age(bodies[std::size_t(i)]);
    }
    
    for (auto& wall : walls) {
        age(wall);
    }

    for (int i = 0; i < state.count_i; ++i) {
        for (int k = 0; k < settings::Collisions::Impacts::EVENTS_PER_BODY; ++k) {
            std::size_t index =
                std::size_t(i) * settings::Collisions::Impacts::EVENTS_PER_BODY + std::size_t(k);
                
            if (index >= events.size()) {
                break;
            }

            const auto& event = events[index];
            if (event.energy_J <= 0) {
                continue;
            }

            auto h = state.halfExtents_m[i];
            float size = std::min({h.x, h.y, h.z});
            float peak = std::clamp(std::log1p(event.energy_J) / std::log1p(redEnergy_J), 0.f, UNIT_RANGE);

            ImpactPatch patch{
                event.localPoint_m, 
                event.localNormal, 
                event.energy_J, 
                PATCH_AGE_START,
                std::clamp(
                    size * (
                        settings::World::Rendering::Impacts::History::PATCH_RADIUS_BASE + 
                        settings::World::Rendering::Impacts::History::PATCH_RADIUS_SLOPE * std::sqrt(peak)
                    ),
                    settings::World::Rendering::Impacts::History::PATCH_RADIUS_MIN_M,
                    settings::World::Rendering::Impacts::History::PATCH_RADIUS_MAX_M
                )
            };

            insert(bodies[std::size_t(i)], patch);

            if (event.wallFace_i >= 0 && event.wallFace_i < settings::Collisions::Geometry::BOX_FACE_COUNT) {
                patch.point_m = event.wallPoint_m;
                patch.normal = {};
                patch.radius_m *= settings::World::Rendering::Impacts::History::WALL_RADIUS_GROWTH;
                insert(walls[std::size_t(event.wallFace_i)], patch);
            }

            lastEnergy_J = event.energy_J;
            lastClosing_mps = event.closing_mps;
            lastImpulse_Ns = event.impulse_Ns;
        }
    }
}


/**
 * Return retained patches for one body.
 *
 * @param i Body index.
 * @return The body's retained patch slots.
 */
const ImpactHistory::Slots& ImpactHistory::body(int i) const {
    return bodies[std::size_t(i)];
}


/**
 * Return retained patches for one wall face.
 *
 * @param i Wall face index.
 * @return The wall's retained patch slots.
 */
const ImpactHistory::Slots& ImpactHistory::wall(int i) const {
    return walls[std::size_t(i)];
}

} // namespace render
