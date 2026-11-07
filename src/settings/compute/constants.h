/**
 * @file
 * Compute settings: single ownership for named execution policies,
 * launch dimensions, and stage definitions shared across backends.
 */

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>

#include "shared/sim/math.h"


namespace settings {

/**
 * Global compute policies and dimensions.
 */
struct Compute {

    /**
     * CUDA-specific launch dimensions and memory layouts.
     */
    struct Cuda {
        /* Keep launch bounds, grid rounding and actual thread-block size in agreement. */
        static constexpr int BLOCK_SIZE = 128;
        /* Total device memory footprint per simulated body. */
        static constexpr std::size_t BYTES_PER_BODY =
            7 * sizeof(sim::Vec3) + 
            sizeof(sim::Quat) + 
            4 * sizeof(sim::Real) + 
            4 * sizeof(int);
    };


    /**
     * The serial CPU backend requires no launch dimensions or device buffer strides.
     *
     * CUDA-only settings are intentionally absent here rather than left as unused aliases.
     */
    struct Cpu {};


    /**
     * Named fixed-step execution stages used for timing and diagnostics.
     *
     * The pipeline executes sequentially in the exact order listed below:
     */
    struct Stages {
        inline static constexpr int COUNT = 10;

        inline static constexpr std::array<const char*, COUNT> NAMES{
            "applyForces",       /* Resets step accumulators and applies gravity. */
            "integrateVelocity", /* Updates linear/angular velocities based on forces. */
            "computeAabb",       /* Refreshes world-space bounding volumes. */
            "broadPhase",        /* Identifies overlapping body pairs using the spatial grid. */
            "narrowPhase",       /* Evaluates precise collision geometries for candidates. */
            "generateContacts",  /* Constructs contact manifolds and tracks impacts. */
            "solveContacts",     /* Applies sequential impulses to resolve penetrations. */
            "integratePosition", /* Advances positions and orientations using velocities. */
            "updateSleep",       /* Transitions resting bodies into a sleeping state. */
            "emitDiagnostics"    /* Aggregates energy, momentum, and performance metrics. */
        };
    };
};

} // namespace settings
