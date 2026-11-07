/**
 * @file
 * Implement validated host-owned simulation arrays and their matching views.
 *
 * This translation unit handles the fixed-capacity allocation of the
 * structure-of-arrays (SOA) storage, the generation of raw device-compatible
 * pointers, and the validation logic for spawning new rigid bodies.
 */

#include "settings/collisions/constants.h"
#include "shared/sim/state.h"


namespace sim {

/**
 * Allocate the simulation state using the standard configured body capacity.
 */
State::State() : State(settings::Collisions::Storage::MAX_BODIES) {}


/**
 * Allocate every structure-of-arrays (SOA) buffer to one fixed body capacity.
 *
 * Groups vectors, quaternions, scalars, and solver flags so that memory
 * remains perfectly parallel and contiguous for the backend.
 *
 * @param capacity Maximum number of bodies the state may contain.
 */
State::State(int capacity) : capacity_i(capacity) {
    for (auto* values : {
        &position_m, &velocity_mps, &angularVelocity_rad_s, 
        &halfExtents_m,  &inverseInertia_kg_m2, &force_N, &aabbExtent_m
    }) { values->resize(capacity); }

    orientation.resize(capacity);

    for (auto* values : {
        &inverseMass_per_kg, 
        &gravityScale_unitless, 
        &boundingRadius_m, 
        &wallProjection_m
    }) { values->resize(capacity); }

    for (auto* values : {
        &shape_i, 
        &sleepCounter_i, 
        &isSleeping, 
        &hasContact
    }) { values->resize(capacity); }
}


/**
 * Build the mutable non-owning schema consumed by shared CPU/CUDA physics code.
 *
 * @return View over all active body arrays and the current body count.
 */
StateView State::view() {
    return {
        count_i,
        /* Linear kinematics. */
        position_m.data(),
        velocity_mps.data(),
        angularVelocity_rad_s.data(),
        /* Rotation and geometry. */
        orientation.data(),
        halfExtents_m.data(),
        aabbExtent_m.data(),
        /* Mass and force accumulators. */
        inverseMass_per_kg.data(),
        inverseInertia_kg_m2.data(),
        force_N.data(),
        gravityScale_unitless.data(),
        /* Broad-phase bounds. */
        boundingRadius_m.data(),
        wallProjection_m.data(),
        /* Solver and tracking flags. */
        shape_i.data(),
        sleepCounter_i.data(),
        isSleeping.data(),
        hasContact.data()
    };
}


/**
 * Remove all active bodies and reset the simulation tick without reallocating.
 */
void State::clear() {
    count_i = 0;
    tick_i = 0;
}


/**
 * Validate and append one body to the active state.
 *
 * The request is rejected if capacity is exhausted, mass is non-positive,
 * required vectors are non-finite, or any half extent is non-positive.
 *
 * Accepted bodies receive a normalized orientation together with derived
 * inverse mass, inverse inertia (using sphere analytic or OBB formulas), and
 * broad-phase bounding radius.
 *
 * @param body Spawn description in SI units.
 * @return True when the body was valid and inserted successfully.
 */
bool State::spawn(const Body& body) {
    constexpr Real SPHERE_INERTIA_FACTOR = Real(.4);
    constexpr Real BOX_INERTIA_FACTOR = Real(3);

    if (count_i >= capacity_i || 
        body.mass_kg.value <= 0 || 
        !finite(body.position_m) ||
        !finite(body.velocity_mps) || 
        !finite(body.halfExtents_m) ||
        minimum(body.halfExtents_m.x, minimum(body.halfExtents_m.y, body.halfExtents_m.z)) <= 0) {
        return false;
    }

    int i = count_i++;

    position_m[i] = body.position_m;
    orientation[i] = normalize(body.orientation);

    velocity_mps[i] = body.velocity_mps;
    angularVelocity_rad_s[i] = body.angularVelocity_rad_s;
    halfExtents_m[i] = body.halfExtents_m;

    inverseMass_per_kg[i] = 1 / body.mass_kg.value;
    gravityScale_unitless[i] = body.gravityScale_unitless;
    shape_i[i] = static_cast<int>(body.shape);

    sleepCounter_i[i] = 0;
    isSleeping[i] = 0;
    hasContact[i] = 0;

    auto half = body.halfExtents_m;
    auto mass = body.mass_kg.value;

    if (body.shape == Shape::Sphere) {
        auto inverse = 1 / (SPHERE_INERTIA_FACTOR * mass * half.x * half.x);
        inverseInertia_kg_m2[i] = {inverse, inverse, inverse};
        boundingRadius_m[i] = half.x;
    } else {
        inverseInertia_kg_m2[i] = {
            BOX_INERTIA_FACTOR / (mass * (half.y * half.y + half.z * half.z)),
            BOX_INERTIA_FACTOR / (mass * (half.x * half.x + half.z * half.z)),
            BOX_INERTIA_FACTOR / (mass * (half.x * half.x + half.y * half.y))
        };
        boundingRadius_m[i] = length(half);
    }

    return true;
}

} // namespace sim
