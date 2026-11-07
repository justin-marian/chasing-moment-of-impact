/**
 * @file
 * Group viewer state by responsibility and declare startup, reset and single-step operations.
 *
 * Viewer session state is grouped by responsibility. Panels receive one Session
 * reference while simulation, camera, debug, spawning and timing state remain
 * separated into cohesive sub-structures. Renderer ownership stays last so it
 * is destroyed before referenced state.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "client/physics_app/cli.h"

#ifdef SIM_BUILD_MMO
#include "client/world_app/session_loop.h"
#endif

#include "physics/scenarios/scenario_io.h"
#include "client/render_gl/rigid_bodies/session_renderer.h"


namespace app {

/**
 * Mutable simulation data and reset state.
 */
struct SimulationState {
    sim::State state;
    sim::StepResult result{};
    double accumulator{};
    std::uint64_t renderHash{};
};


/**
 * Camera state used by the interactive viewer.
 */
struct CameraState {
    float yaw{.7853982f};
    float pitch{.328f};
    float distance{83.64f};
};


/**
 * Object creation and scenario editing controls.
 */
struct SpawnState {
    int shape{};
    int count{10};
    int substeps{8};
    int maxContacts{};
    int seedInput{};
    std::mt19937_64 generator;
};


/**
 * Debug visualization and selection state.
 */
struct DebugState {
    int selectedBody{-1};
    bool aabb{};
    bool obb{};
    bool velocity{};
    char filter[80]{};
};


/**
 * Per-frame UI and navigation state.
 */
struct ViewerState {
    CameraState camera;
    std::vector<std::string> messages;

    int frames{};
    bool rightPanel{};

    bool simulationWidget{true}, spawnWidget{true}, logWidget{true};
    bool physicsWidget{}, cameraWidget{};
    bool performanceWidget{true};
    bool overlayVisible{true};
    bool openSession{};
};


/**
 * Recorded frame timing data.
 */
struct PerformanceState {
    std::vector<double> physicsTimes;
    std::vector<double> renderTimes;
};


/**
 * Viewer-owned resources and state grouped by simulation, controls, diagnostics and rendering lifetime.
 */
struct Session {
    /* Immutable after construction. */
    ViewerOptions options;
    sim::Scenario scene;
    std::unique_ptr<sim::Backend> backend;
    std::string backendName;

    /* Mutable state. */
    SimulationState simulation;
    SpawnState spawn;
    DebugState debug;
    ViewerState view;
    PerformanceState performance;

    /* Owned last so it is destroyed before referenced state. */
    std::unique_ptr<render::Renderer> renderer;

    /**
     * Append a message to the log, keeping the most recent entries.
     *
     * @param text Human-readable event appended to the bounded local session log.
     */
    void log(const std::string& text);


    /**
     * Access the impact history owned by the renderer.
     *
     * @return Borrowed retained impact data.
     */
    render::ImpactHistory& impacts();


    /**
     * Return a borrowed view of the most recently retained visual impact data.
     *
     * @return Borrowed retained impact data.
     */
    const render::ImpactHistory& impacts() const;


    /**
     * Return the viewer session's owned simulation state for read-only access or controlled updates.
     *
     * @return Reference to the viewer-owned simulation state.
     */
    sim::State& state() {
        return simulation.state;
    }


    /**
     * Return the viewer session's owned simulation state for read-only access or controlled updates.
     *
     * @return Reference to the viewer-owned simulation state.
     */
    const sim::State& state() const {
        return simulation.state;
    }
};


/**
 * Construct the scenario/backend, upload its initial state and create renderer resources.
 *
 * Rendering uses a seed derived from the scenario so procedural surface shading
 * remains repeatable.
 *
 * @param session Empty viewer session receiving initialized simulation and renderer ownership.
 * @param options Validated launcher settings, retained for capture and scene navigation.
 */
void initializeSession(Session& session, const ViewerOptions& options);


#ifdef SIM_BUILD_MMO
/**
 * Translate viewer launch settings into the corresponding shared-world options.
 *
 * Connection, environment, appearance and capture fields are assigned explicitly by group.
 *
 * @param options Validated viewer settings selected on the command line.
 * @return Equivalent startup configuration for the shared-world scene.
 */
client::Options makeClientOptions(const ViewerOptions& options);
#endif


/**
 * Execute one backend step, download its state and age confirmed visual impact marks.
 *
 * Used by both the fixed-step frame loop and the UI's single-step button.
 *
 * @param session Viewer state whose backend, simulation snapshot and impact history advance.
 */
void stepSimulation(Session& session);


/**
 * Reset the session to the configured scenario and seed.
 *
 * Repopulates simulation state, clears impact history, re-uploads to the
 * backend and resets transient simulation state.
 *
 * @param s Session to reset.
 */
void resetSession(Session& s);

} // namespace app


/**
 * @file
 * Advance and draw one glass-viewer frame while keeping simulation ownership in the session.
 */

#include "client/physics_app/run_session.h"


namespace app {

/**
 * Accumulate elapsed time and execute a bounded number of fixed physics steps.
 *
 * Pausing clears accumulated time; deterministic captures advance one step per
 * frame. Contact maxima and rolling timing samples are updated after the physics work.
 *
 * @param session Simulation state, backend and diagnostic history updated in place.
 * @param paused  Whether physics advancement is temporarily suspended.
 * @param elapsed Real duration since the previous visual frame, in seconds.
 */
void advanceSimulation(Session& session, bool paused, double elapsed);


/**
 * Draw the viewer windows and read-only scene, checking that rendering preserves state.
 *
 * The final requested screenshot is saved before presentation; the frame
 * counter then advances.
 *
 * @param session      Window, physics snapshot and UI state used by this frame.
 * @param paused       Playback state that the simulation controls can change.
 * @param presentation Whether the UI should use presentation mode; controls may update it.
 * @return True when the user requests switching to the shared world.
 */
bool drawSession(Session& session, bool& paused, bool& presentation);

} // namespace app


/**
 * @file
 * Write reproducible glass-viewer capture metadata after the requested final frame.
 *
 * Screenshot capture and the JSON manifest that goes with it. The manifest is a
 * machine-readable record of the run: scenario, seed, backend, surface shading,
 * render state hash, driver string and per-body peak impact energy. Written next
 * to the .png so a capture and its metadata travel together.
 */

#include <filesystem>


namespace app::capture {

/**
 * Write the framebuffer as PNG and the manifest as JSON next to it.
 *
 * @param s             Session to read state from.
 * @param captureFrames Number of frames the capture was scheduled for.
 * @param screenshot    Destination .png path; the manifest is written as screenshot + ".json".
 */
void writeCapture(Session& s, int captureFrames, const std::filesystem::path& screenshot);

} // namespace app::capture
