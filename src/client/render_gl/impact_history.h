/**
 * @file
 * Retain, merge and age visual impact patches independently of the collision solver.
 */
#pragma once
#include "settings/collisions/constants.h"

#include "shared/sim/impacts.h"

#include <array>
#include <span>
#include <vector>

namespace render {

/**
 * Surface-local heat region retained after an impact.
 */
struct ImpactPatch {
    sim::Vec3 point_m{}, normal{};
    float energy_J{}, age_s{}, radius_m{};
};

/**
 * Retained body and wall impact patches with age-based visual decay.
 */
class ImpactHistory {
    using Slots = std::array<ImpactPatch, settings::Collisions::Impacts::PATCHES_PER_SURFACE>;

    std::vector<Slots> bodies;
    std::array<Slots, settings::Collisions::Geometry::BOX_FACE_COUNT> walls{};

    /**
     * Merge a nearby hit or replace a free or old retained patch.
     *
     * @param slots Fixed patch bank updated by merging a nearby mark or replacing an old slot.
     * @param patch Retained local-surface mark carrying position, radius, energy and age.
     */
    static void insert(Slots& slots, const ImpactPatch& patch);

public:
    bool enabled{true}, hold{};
    float lifetime_s{8}, redEnergy_J{10};
    float lastEnergy_J{}, lastClosing_mps{}, lastImpulse_Ns{};

    /**
     * Construct with a fixed per-body capacity.
     *
     * @param capacity Maximum number of bodies or retained body slots allocated by this owner.
     */
    explicit ImpactHistory(int capacity = settings::Collisions::Storage::MAX_BODIES);

    /**
     * Clear retained patches and reported impact values.
     */
    void clear();

    /**
     * Return normalized visual strength for one retained patch.
     *
     * @param patch Retained local-surface mark carrying position, radius, energy and age.
     * @return Normalized heat associated with the retained patch's current energy and age.
     */
    float strength(const ImpactPatch& patch) const;

    /**
     * Age retained patches and consume confirmed per-body impacts.
     *
     * @param state Host simulation arrays, active body count and authoritative tick in SI units.
     * @param events Confirmed pre-solve impact measurements retained by the selected backend.
     * @param dt_s Simulation or impact-aging interval in seconds.
     */
    void advance(const sim::State& state, std::span<const sim::ImpactEvent> events, float dt_s);

    /**
     * Return retained patches for one body.
     *
     * @param i Body index in the retained per-body patch storage.
     * @return Borrowed patch slots retained for the requested body.
     */
    const Slots& body(int i) const;

    /**
     * Return retained patches for one wall face.
     *
     * @param i Static wall-face index in the retained wall patch bank.
     * @return Borrowed patch slots retained for the requested world-box face.
     */
    const Slots& wall(int i) const;
};

} // namespace render
