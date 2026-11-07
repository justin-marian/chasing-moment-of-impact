/**
 * @file
 * Define validated host-owned simulation arrays and their matching non-owning CPU/CUDA views.
 *
 * This header defines the core memory backbone for the physics simulation.
 * To maximize cache coherency and enable efficient GPU kernel execution, data
 * is stored in a Structure-of-Arrays (SoA) layout rather than an Array-of-Structs.
 *
 * The architecture splits ownership:
 * - The owning State allocates all buffers once on the host.
 * - The non-owning StateView provides raw pointers to these buffers so that
 *   both CPU and CUDA backends can process identical memory layouts without
 *   expensive repacking or device copies.
 */

#pragma once

#include "settings/compute/constants.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "shared/sim/math.h"


namespace sim {

/* Collision shape. Spheres use analytic contacts; other shapes use OBB approximations. */
enum class Shape : int { Sphere, Cube, Cylinder, Cone };
/* High-level material behavior used by scenarios and reporting. */
enum class MaterialClass : int { Rigid, Elastic, Plastic };


/**
 * Fixed storage limits shared by the state and the solver.
 *
 * Contact capacity includes four manifold points per candidate plus contacts
 * generated against the six world-box faces.
 */
struct WorldConfig {
    /* Domain bounds. */
    Vec3 boxHalfExtents_m{20, 15, 20};

    /* Gravity and fixed simulation step. */
    Real gravity_mps2{Real(9.81)}, dt_s{Real(1.0 / 120.0)};

    /* Coulomb friction coefficients. */
    Real frictionStatic_unitless{Real(.5)}, frictionDynamic_unitless{Real(.4)};
    /* Restitution coefficient and minimum closing speed that produces bounce. */
    Real restitution_unitless{Real(.45)}, restitutionSlop_mps{Real(.01)};

    /* Velocity damping applied per second. */
    Real linearDamping_per_s{Real(.01)}, angularDamping_per_s{Real(.05)};
    /* Linear/angular thresholds below which supported bodies may sleep. */
    Real sleepLinVel_mps{Real(.05)}, sleepAngVel_rad_s{Real(.05)};
    int sleepTicks_i{60};

    /* Solver iterations and broad-phase cell size. */
    int solverIterations_i{10};
    Real cellSize_m{Real(1.6)};

    /* Penetration slop and position correction fraction. */
    Real contactSlop_m{Real(.001)}, positionCorrection_unitless{Real(.6)};
    /* Feature toggles. */
    bool enableSleep{true}, enableCcd{true};

    /* Rendering-only wall visibility controls retained with the simulation configuration. */
    bool nearWallVisible{false};
    Real wallAlpha_unitless{Real(.13)};
};


/**
 * Description used to create one rigid body before conversion to SoA storage.
 *
 * Position, velocity and extents use SI units. Orientation is normalized when
 * the body is inserted into the state.
 */
struct Body {
    /* Kinematics. */
    Vec3 position_m{}, velocity_mps{}, angularVelocity_rad_s{};

    /* Geometry and mass. */
    Vec3 halfExtents_m{Real(.35), Real(.35), Real(.35)};
    Quat orientation{};
    Kg mass_kg{1};

    /* Material properties and shape classification. */
    Real gravityScale_unitless{1};
    Shape shape{Shape::Sphere};
};


/**
 * Non-owning host/device view of the simulation structure-of-arrays storage.
 *
 * Every pointer addresses the same body index range. The view owns no memory
 * and remains valid only while the corresponding state buffers remain
 * allocated at the same addresses.
 */
struct StateView {
    int count_i{};

    /* Vector state and cached collision bounds. */
    Vec3 *position_m{}, *velocity_mps{}, *angularVelocity_rad_s{};
    Vec3 *halfExtents_m{}, *inverseInertia_kg_m2{}, *force_N{}, *aabbExtent_m{};

    /* Rotation state. */
    Quat* orientation{};

    /* Scalar properties and broad-phase data. */
    Real *inverseMass_per_kg{}, *gravityScale_unitless{}, *boundingRadius_m{};
    Real* wallProjection_m{};

    /* Solver and tracking flags. */
    int *shape_i{}, *sleepCounter_i{}, *isSleeping{}, *hasContact{};
};

static_assert(std::is_standard_layout_v<StateView>);
static_assert(sizeof(Vec3) == 3 * sizeof(Real));
static_assert(sizeof(Quat) == 4 * sizeof(Real));
static_assert(offsetof(StateView, count_i) == 0);


/**
 * Owning fixed-capacity host representation of rigid-body simulation state.
 *
 * Storage uses the same structure-of-arrays organization exposed through the
 * view so CPU and CUDA implementations operate on the same logical schema.
 * Capacity is fixed when constructed and vectors are never grown during
 * normal simulation, keeping body indices and host/device copies stable.
 */
struct State {
    /* Active body count, allocated capacity and authoritative simulation tick. */
    int count_i{}, capacity_i;
    std::uint64_t tick_i{};

    /* Vector state and cached collision bounds. */
    std::vector<Vec3> position_m, velocity_mps, angularVelocity_rad_s;
    std::vector<Vec3> halfExtents_m, inverseInertia_kg_m2, force_N, aabbExtent_m;

    /* Rotation state. */
    std::vector<Quat> orientation;

    /* Scalar properties and broad-phase data. */
    std::vector<Real> inverseMass_per_kg, gravityScale_unitless;
    std::vector<Real> boundingRadius_m, wallProjection_m;

    /* Solver and tracking flags. */
    std::vector<int> shape_i, sleepCounter_i, isSleeping, hasContact;

    /* Allocate state using the standard body capacity. */
    State();
    /* Allocate every SoA buffer to one fixed body capacity. */
    explicit State(int capacity);
    /* Build the mutable non-owning schema consumed by shared CPU/CUDA physics code. */
    StateView view();
    /* Remove all active bodies and reset the simulation tick without reallocating. */
    void clear();

    /* Validate and append one body to the active state. */
    bool spawn(const Body& body);
};


/**
 * Contact constraint produced by narrow-phase collision detection.
 *
 * The normal points from the first body toward the second. A negative index
 * represents a static world wall rather than another body.
 */
struct Contact {
    /* Body indices. */
    int a_i{}, b_i{-1};
    /* World-space contact direction and point. */
    Vec3 normal{}, point_m{};
    /* Penetration and accumulated normal solver state. */
    Real depth_m{}, normalImpulse_Ns{}, targetVelocity_mps{};
    /* Accumulated Coulomb friction impulse in world space. */
    Vec3 tangentImpulse_Ns{};
};

static_assert(std::is_standard_layout_v<Contact>);
static_assert(offsetof(Contact, a_i) == 0);
static_assert(offsetof(Contact, b_i) == sizeof(int));


/* Aggregate evidence collected from one completed simulation step. */
struct Diagnostics {
    /* Energy, momentum and speed metrics. */
    Real kineticEnergy_J{}, potentialEnergy_J{}, maxSpeed_mps{};
    Vec3 linearMomentum_kgmps{};

    /* Body and contact counts. */
    int activeBodies_i{}, sleepingBodies_i{};
    int contacts_i{}, candidatePairs_i{};

    /* Numerical failure counts. */
    int nanCount_i{}, overflowCount_i{};

    /* World-boundary correction stats. */
    Real maxWallProjection_m{};
    int projectedBodies_i{};
};


/* Result returned after one complete backend simulation step. */
struct StepResult {
    /* Wall-clock duration of each stage in the configured stage order. */
    std::array<double, settings::Compute::Stages::COUNT> stage_wall_ms{};
    /* Aggregate step evidence. */
    Diagnostics diagnostics{};
    /* Total backend wall-clock duration in milliseconds. */
    double total_wall_ms{};
};

} // namespace sim
