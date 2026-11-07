/**
 * @file
 * Collision settings: single ownership for named policies, capacity limits,
 * geometry constants, and scenario schema definitions.
 */

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>

#include "shared/sim/scalar.h"


namespace settings {

/**
 * Global collision detection and resolution policies.
 */
struct Collisions {
    /**
     * Geometry constants and dimensions.
     */
    struct Geometry {
        /* Geometry calculations share the scalar precision used by the active simulation build. */
        static constexpr sim::Real EPSILON = sim::Real(1e-7);
        static constexpr sim::Real PI = sim::Real(3.14159265358979323846);

        /* Supported shape count and world-box face count. */
        static constexpr std::size_t SHAPE_COUNT = 4;
        static constexpr int BOX_FACE_COUNT = 6;
    };


    /**
     * Fixed-capacity memory limits for bodies and contacts.
     */
    struct Storage {
        /* Overflow is reported; storage must never silently discard bodies or candidate pairs. */
        static constexpr int MAX_BODIES = 8192;
        static constexpr int MAX_CANDIDATES = 128;

        /* Maximum contacts includes candidates with up to four manifold points plus six faces. */
        static constexpr int MAX_CONTACTS = MAX_CANDIDATES * 4 + 6 * 8;
    };


    /**
     * Sequential impulse solver graph coloring limits.
     */
    struct Solver {
        /* Maximum number of colors used for parallel constraint solving. */
        static constexpr int MAX_SOLVER_COLORS = 256;
        /* Number of 32-bit words needed to represent the color mask. */
        static constexpr int COLOR_MASK_WORDS = MAX_SOLVER_COLORS / 32;
    };


    /**
     * Collision impact telemetry capacities.
     */
    struct Impacts {
        /* Physical telemetry capacity and the visible retained-patch capacity have different purposes. */
        static constexpr int EVENTS_PER_BODY = 4;
        static constexpr int PATCHES_PER_SURFACE = 8;
    };


    /**
     * Spatial grid broad-phase settings.
     */
    struct BroadPhase {
        /* Bias added to cell coordinates to prevent negative indices. */
        static constexpr int CELL_BIAS = 1 << 20;
    };


    /**
     * Scenario configuration schema and field mapping.
     */
    struct Scenario {
        /**
         * Mapped fields for parsing scenario configuration.
         */
        enum class Field {
            /* Identification. */
            Id,
            Seed,
            /* Shape counts. */
            Spheres,
            Cubes,
            Cylinders,
            Cones,
            /* Geometry dimensions. */
            Radius,
            BoxX,
            BoxY,
            BoxZ,
            /* Timing and world parameters. */
            Dt,
            Gravity,
            MaxInitialSpeed,
            /* Material properties. */
            FrictionStatic,
            FrictionDynamic,
            Restitution,
            LinearDamping,
            AngularDamping,
            /* Solver configurations. */
            SolverIterations,
            CellSize,
            EnableSleep,
            EnableCcd
        };


        /* String-to-field mapping for configuration parsing. */
        inline static constexpr std::pair<std::string_view, Field> FIELDS[] = {
            /* Identification. */
            {"id", Field::Id},
            {"seed", Field::Seed},
            /* Shape counts. */
            {"spheres_i", Field::Spheres},
            {"cubes_i", Field::Cubes},
            {"cylinders_i", Field::Cylinders},
            {"cones_i", Field::Cones},
            {"radius_m", Field::Radius},
            /* Geometry dimensions. */
            {"boxHalfExtentX_m", Field::BoxX},
            {"boxHalfExtentY_m", Field::BoxY},
            {"boxHalfExtentZ_m", Field::BoxZ},
            /* Timing and world parameters. */
            {"dt_s", Field::Dt},
            {"gravity_mps2", Field::Gravity},
            {"maxInitialSpeed_mps", Field::MaxInitialSpeed},
            /* Material properties. */
            {"frictionStatic_unitless", Field::FrictionStatic},
            {"frictionDynamic_unitless", Field::FrictionDynamic},
            {"restitution_unitless", Field::Restitution},
            {"linearDamping_per_s", Field::LinearDamping},
            {"angularDamping_per_s", Field::AngularDamping},
            /* Solver configurations. */
            {"solverIterations_i", Field::SolverIterations},
            {"cellSize_m", Field::CellSize},
            {"enableSleep", Field::EnableSleep},
            {"enableCcd", Field::EnableCcd}
        };
    };
};

} // namespace settings
