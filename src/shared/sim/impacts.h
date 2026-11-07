/**
 * @file
 * Represent bounded contact-impact output shared by both physics backends and renderers.
 *
 * Tracks the most energetic contact events per body so visualization layers
 * can render impact heat maps without unbounded memory growth.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/physics.h"


namespace sim {

/**
 * Pre-solve severity derived from relative velocity and effective mass.
 */
struct ImpactMeasure {
    /* Closing speed along the contact normal in metres per second. */
    Real closing_mps{};
    /* Reduced-mass kinetic energy associated with that closing motion in joules. */
    Real energy_J{};
};


/**
 * Confirmed collision event retained for optional per-body visualization.
 *
 * Local point/normal follow the body after impact; the world wall point is
 * populated only for static world-wall contacts. Maximum events per body are
 * bounded by the impact bank size.
 */
struct ImpactEvent {
    /* Contact point and normal in the impacted body's local frame. */
    Vec3 localPoint_m{}, localNormal{};
    /* World-space point on a contacted static wall; zero for body-body contacts. */
    Vec3 wallPoint_m{};
    /* Impact severity measured before solve and resolved normal impulse after solve. */
    Real energy_J{}, closing_mps{}, impulse_Ns{};
    /* Static wall face index, or -1 for body-body contact. */
    int wallFace_i{-1};
};


/**
 * Measure normal closing speed and reduced-mass impact energy before constraint solving.
 *
 * Separating contacts report zero speed and therefore zero impact energy.
 *
 * @param s  Shared body-state view.
 * @param ct Contact whose pre-solve relative motion is measured.
 * @return Closing speed and corresponding reduced-mass kinetic energy.
 */
SIM_HD inline ImpactMeasure measureImpact(StateView s, const Contact& ct) {
    constexpr Real HALF = Real(.5);

    Real speed = maximum(
        0, 
        -dot(contactVelocity(s, ct.b_i, ct.point_m) - contactVelocity(s, ct.a_i, ct.point_m), ct.normal)
    );
    Real inverse = effectiveMassInverse(s, ct, ct.normal);
    
    return {
        speed, 
        inverse > settings::Collisions::Geometry::EPSILON ? HALF * speed * speed / inverse : 0
    };
}


/**
 * Retain an event only when it is stronger than the weakest existing slot.
 *
 * The fixed-size bank therefore keeps the most energetic recent contacts
 * without requiring allocation or sorting on CPU/CUDA hot paths.
 *
 * @param slots First slot in one body's impact event bank.
 * @param event Candidate event to retain.
 */
SIM_HD inline void retainImpact(ImpactEvent* slots, const ImpactEvent& event) {
    int weakest = 0;

    for (int k = 1; k < settings::Collisions::Impacts::EVENTS_PER_BODY; ++k) {
        if (slots[k].energy_J < slots[weakest].energy_J) {
            weakest = k;
        }
    }

    if (event.energy_J > slots[weakest].energy_J) {
        slots[weakest] = event;
    }
}


/**
 * Record a confirmed energetic contact in each participating body's local frame.
 *
 * Low-speed contacts, negligible-energy contacts, and speculative contacts
 * without resolved normal impulse are ignored so resting support does not
 * create heat marks. Static-wall contacts also retain the wall face and
 * projected world-space wall point.
 *
 * @param s       Shared body-state view.
 * @param ct      Solved contact to inspect.
 * @param measure Pre-solve severity captured before impulses changed velocity.
 * @param c       World configuration used to reconstruct static wall positions.
 * @param out     Per-body impact banks sized for all retained events.
 */
SIM_HD inline void recordImpact(
    StateView s, 
    const Contact& ct, 
    ImpactMeasure measure, 
    const WorldConfig& c, 
    ImpactEvent* out
) {
    constexpr Real MIN_CLOSING_MPS = Real(.5);
    constexpr Real MIN_ENERGY_J = Real(.005);
    constexpr Real MIN_IMPULSE_NS = Real(.0001);
    constexpr Real AXIS_DOMINANCE_THRESHOLD = Real(.5);

    if (measure.closing_mps < MIN_CLOSING_MPS || 
        measure.energy_J < MIN_ENERGY_J || 
        ct.normalImpulse_Ns <= MIN_IMPULSE_NS) {
        return;
    }

    ImpactEvent event{};
    event.energy_J = measure.energy_J;
    event.closing_mps = measure.closing_mps;
    event.impulse_Ns = ct.normalImpulse_Ns;

    if (ct.b_i < 0) {
        int axis = 
            absolute(ct.normal.x) > AXIS_DOMINANCE_THRESHOLD ? 0 : 
            absolute(ct.normal.y) > AXIS_DOMINANCE_THRESHOLD ? 1 : 2;

        event.wallFace_i = axis * 2 + (ct.normal[axis] > 0 ? 1 : 0);
        event.wallPoint_m = ct.point_m;
        event.wallPoint_m[axis] = ct.normal[axis] * c.boxHalfExtents_m[axis];
    }

    event.localPoint_m = rotate(conjugate(s.orientation[ct.a_i]), ct.point_m - s.position_m[ct.a_i]);
    event.localNormal = rotate(conjugate(s.orientation[ct.a_i]), ct.normal);
    retainImpact(out + ct.a_i * settings::Collisions::Impacts::EVENTS_PER_BODY, event);

    if (ct.b_i >= 0) {
        event.localPoint_m = rotate(conjugate(s.orientation[ct.b_i]), ct.point_m - s.position_m[ct.b_i]);
        event.localNormal = rotate(conjugate(s.orientation[ct.b_i]), -ct.normal);
        retainImpact(out + ct.b_i * settings::Collisions::Impacts::EVENTS_PER_BODY, event);
    }
}

} // namespace sim
