/**
 * @file
 * Define the common CPU/CUDA backend lifecycle and fixed-step execution contract.
 *
 * Provides an abstract interface that allows physics to run on different
 * execution hardware without exposing CPU or GPU memory layouts to the caller.
 */

#pragma once

#include "settings/collisions/constants.h"

#include <memory>
#include <span>
#include <string>

#include "shared/sim/impacts.h"
#include "shared/sim/state.h"


namespace sim {

/**
 * Abstract fixed-step physics backend shared by CPU and CUDA implementations.
 *
 * An implementation owns or references the execution storage required by its
 * platform and exposes explicit upload/download boundaries so callers do not
 * depend on host or device memory layouts.
 */
class Backend {
public:
    /* Release owned execution resources through polymorphic lifetime. */
    virtual ~Backend() = default;


    /**
     * Transfer the active host state into backend storage.
     *
     * Device backends copy the active range to execution buffers, while host
     * backends may share storage directly.
     *
     * @param state Host body arrays, active count and tick within capacity.
     */
    virtual void upload(const State& state) = 0;


    /**
     * Advance the backend by one fixed simulation step.
     *
     * Evaluates forces, contacts, constraint solving, integration, sleeping
     * and diagnostics. Device backends may keep body arrays resident until
     * download, but the host tick/count and returned diagnostics are updated.
     *
     * @param state  Host state carrying the authoritative tick and active count.
     * @param config Solver, world and timing parameters in SI units.
     * @return Per-stage wall timings and aggregate diagnostics for this step.
     */
    virtual StepResult step(State& state, const WorldConfig& config) = 0;


    /**
     * Copy the current backend state into host-owned arrays.
     *
     * The destination must already have enough capacity for the active bodies.
     *
     * @param state Host destination receiving the current backend state.
     */
    virtual void download(State& state) = 0;


    /**
     * Return the short backend identifier.
     *
     * @return Stable backend name.
     */
    virtual std::string name() const = 0;


    /**
     * Return a human-readable description of the execution device.
     *
     * @return Device or processor description suitable for diagnostics.
     */
    virtual std::string deviceInfo() const = 0;


    /**
     * Enable or disable collision-impact telemetry.
     *
     * Backends that do not support impact tracking may leave the default
     * no-op behavior.
     *
     * @param enabled Whether confirmed impact events should be retained.
     */
    virtual void setImpactTracking(bool enabled) {
        (void)enabled;
    }


    /**
     * Return the latest retained impact events.
     *
     * The returned span borrows backend-owned storage and becomes invalid when
     * that storage is refreshed or the backend is destroyed. Unsupported or
     * disabled tracking returns an empty span.
     *
     * @return Borrowed view of the latest impact records.
     */
    virtual std::span<const ImpactEvent> impacts() const {
        return {};
    }
};


/**
 * Create the CPU backend with a fixed body capacity.
 *
 * @param capacity_i Maximum number of bodies the backend may store.
 * @return Owned CPU backend instance.
 * @throws std::runtime_error If the requested capacity cannot be created.
 */
std::unique_ptr<Backend> createCpuBackend(int capacity_i = settings::Collisions::Storage::MAX_BODIES);


/**
 * Create the CUDA backend with a fixed device-buffer capacity.
 *
 * @param capacity_i Maximum number of bodies allocated in device storage.
 * @return Owned CUDA backend instance.
 * @throws std::runtime_error If CUDA support, a suitable device, or memory is unavailable.
 */
std::unique_ptr<Backend> createCudaBackend(int capacity_i = settings::Collisions::Storage::MAX_BODIES);


/**
 * Report whether a usable CUDA device is available.
 *
 * @return True when CUDA support is available and a compatible device can be selected.
 */
bool hasCudaDevice();

} // namespace sim
