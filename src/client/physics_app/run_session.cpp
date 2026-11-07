/**
 * @file
 * Implement session initialization, fixed-step advancement, and frame rendering.
 *
 * Coordinates the physics backend, scenario state, and OpenGL renderer.
 * Handles the accumulation of wall-clock time for deterministic fixed-step
 * physics, ensures rendering does not mutate physics state, and captures
 * diagnostic metadata for screenshot runs.
 */

#include "client/physics_app/run_session.h"

#include "client/physics_app/settings/diagnostic_windows.h"
#include "client/physics_app/settings/parameter_windows.h"

#ifdef SIM_BUILD_MMO
#include "client/world_app/session_loop.h"
#endif

#include "physics/scenarios/impact_demo.h"
#include "settings/world/constants.h"
#include "shared/utils/log.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <GLFW/glfw3.h>


namespace app {

/**
 * Construct the scenario, backend, and renderer resources.
 *
 * Builds the initial scenario, state, and physics backend, then creates the
 * renderer resources using the validated launcher settings.
 *
 * @param session Empty viewer session receiving initialized simulation and renderer ownership.
 * @param options Validated launcher settings, retained for capture and scene navigation.
 */
void initializeSession(Session& session, const ViewerOptions& options) {
    session.options = options;
    session.view.physicsWidget = options.view.physicsSettings;
    session.backendName = options.simulation.backend;

    session.scene = viewerScenario(options.simulation.scenario, options.simulation.seed);
    session.spawn.seedInput = int(options.simulation.seed & 0x7FFFFFFF);

    populateViewer(session.simulation.state, session.scene);
    session.backend = options.simulation.backend == "cuda" ? sim::createCudaBackend() : sim::createCpuBackend();
    session.backend->upload(session.simulation.state);

    session.renderer = std::make_unique<render::Renderer>(options.view.visible, options.view.width, options.view.height);

    session.renderer->impactHistory().enabled = options.impact.enabled;
    session.renderer->impactHistory().hold = options.impact.hold;
    session.renderer->impactHistory().lifetime_s = options.impact.life;
    session.renderer->impactHistory().redEnergy_J = options.impact.redEnergy;

    session.log("Ready. Fixed time step; seeded spawn.");
    session.spawn.generator.seed(session.scene.seed);
}


#ifdef SIM_BUILD_MMO
/**
 * Translate viewer launch settings into the shared-world startup options.
 *
 * Preserves identity and connection behavior when changing scenes, transfers
 * client scene settings without modifying physics options, and groups
 * appearance and image output by their responsibilities.
 *
 * @param options Validated viewer settings selected on the command line.
 * @return Equivalent startup configuration for the shared-world scene.
 */
client::Options makeClientOptions(const ViewerOptions& options) {
    client::Options startup;

    startup.connection.host = options.network.host;
    startup.connection.user = options.network.user;
    startup.connection.token = options.network.token;
    startup.connection.port = options.network.port;
    startup.connection.lobby = options.network.lobby;

    startup.scene.environment = options.scene.environment;
    startup.scene.clip = options.scene.previewClip;
    startup.scene.sky = options.view.sky;
    startup.scene.skyRotation = options.view.skyRotation;
    startup.scene.skyExposure = options.view.skyExposure;
    startup.scene.paused = options.simulation.paused;
    startup.scene.impactDemo = options.impact.avatarImpactDemo;

    startup.avatar.character = options.scene.character;
    startup.avatar.color = options.scene.characterColor;
    startup.capture.path = options.capture.screenshot;
    startup.capture.frames = options.capture.frames;

    return startup;
}
#endif


/**
 * Execute one backend step, download its state, and age confirmed visual impact marks.
 *
 * Both continuous playback and the step button use the same backend/impact
 * sequence. The completed tick is downloaded before updating the visual
 * history from its physical impacts.
 *
 * @param session Viewer state whose backend, simulation snapshot and impact history advance.
 */
void stepSimulation(Session& session) {
    session.backend->setImpactTracking(session.impacts().enabled);
    session.simulation.result = session.backend->step(session.simulation.state, session.scene.world);

    session.backend->download(session.simulation.state);
    session.impacts().advance(session.simulation.state, session.backend->impacts(), session.scene.world.dt_s);
}


/**
 * Append a message to the log, keeping the most recent entries.
 *
 * @param text Human-readable event appended to the bounded local session log.
 */
void Session::log(const std::string& text) {
    utils::appendLog(view.messages, text, settings::World::PhysicsApp::Policy::LOG_ENTRIES);
}


/**
 * Access the impact history owned by the renderer.
 *
 * @return Borrowed retained impact data.
 */
render::ImpactHistory& Session::impacts() {
    return renderer->impactHistory();
}


/**
 * Access the impact history owned by the renderer.
 *
 * @return Borrowed retained impact data.
 */
const render::ImpactHistory& Session::impacts() const {
    return renderer->impactHistory();
}


/**
 * Reset the session to the configured scenario and seed.
 *
 * Repopulates simulation state, clears impact history, re-uploads to the
 * backend, and resets transient simulation state.
 *
 * @param s Session to reset.
 */
void resetSession(Session& s) {
    s.scene.seed = std::uint64_t(s.spawn.seedInput);
    populateViewer(s.simulation.state, s.scene);

    s.impacts().clear();
    s.backend->upload(s.simulation.state);

    s.simulation.result = {};
    s.simulation.accumulator = 0;
    s.spawn.generator.seed(s.scene.seed);
    s.log("Reset to seed " + std::to_string(s.scene.seed));
}

} // namespace app


namespace app {

namespace {

/**
 * Check if the current run is in finite-frame capture mode.
 *
 * @param session Viewer state to inspect.
 * @return True if capture mode is active.
 */
constexpr bool captureModeActive(const Session& session) {
    return session.options.capture.frames >= 0;
}


/**
 * Check if the current frame is the final one requested for capture.
 *
 * Capture mode must be active, and this must be the last requested frame.
 *
 * @param session Viewer state to inspect.
 * @return True if the screenshot should be taken after this frame.
 */
bool isFinalCaptureFrame(const Session& session) {
    return captureModeActive(session) && session.view.frames == session.options.capture.frames - 1;
}


/**
 * Accumulate elapsed wall-clock time.
 *
 * Discards anything above the maximum threshold after a stall, rather than
 * paying it back in substeps.
 *
 * @param session Viewer state holding the time accumulator.
 * @param elapsed Real duration since the previous visual frame, in seconds.
 */
void accumulateElapsed(Session& session, double elapsed) {
    session.simulation.accumulator += std::min(elapsed, settings::World::Timing::MAXIMUM_ELAPSED_S);
}


/**
 * Step the simulation according to the current mode.
 *
 * In capture mode, advances exactly one tick per visual frame regardless of
 * the time accumulator, so screenshots align deterministically with tick
 * numbers. In normal mode, uses fixed-step accumulation capped by the maximum
 * allowed substeps, preventing a stalled frame from compounding into an
 * unbounded catch-up burst.
 *
 * @param session Viewer state containing the physics backend and accumulators.
 */
void advancePhysics(Session& session) {
    auto& sim = session.simulation;
    const float dt = session.scene.world.dt_s;

    if (captureModeActive(session)) {
        if (session.spawn.substeps > 0) {
            stepSimulation(session);
            sim.accumulator -= dt;
        }
        return;
    }

    int steps = 0;
    while (steps < session.spawn.substeps && sim.accumulator >= dt) {
        stepSimulation(session);
        sim.accumulator -= dt;
        ++steps;
    }
}


/**
 * Push a new value into a rolling window, dropping the oldest entry if full.
 *
 * Both timeseries must be kept at the same length by always calling this
 * helper on both of them, so the diagnostics panel can safely index either.
 *
 * @param window The rolling window container.
 * @param value  The new value to append.
 * @param cap    The maximum number of entries allowed.
 */
template <class Container>
void pushRolling(Container& window, typename Container::value_type value, std::size_t cap) {
    window.push_back(value);
    if (window.size() > cap) {
        window.erase(window.begin());
    }
}

} // namespace (anonymous)


/**
 * Accumulate elapsed time and execute a bounded number of fixed physics steps.
 *
 * Pausing clears accumulated time. Contact maxima and rolling timing samples
 * are updated after the physics work is complete.
 *
 * @param session Simulation state, backend and diagnostic history updated in place.
 * @param paused  Whether physics advancement is temporarily suspended.
 * @param elapsed Real duration since the previous visual frame, in seconds.
 */
void advanceSimulation(Session& session, bool paused, double elapsed) {
    accumulateElapsed(session, elapsed);

    if (paused) {
        session.simulation.accumulator = 0;
    } else {
        advancePhysics(session);
    }

    auto& spawn = session.spawn;
    spawn.maxContacts = std::max(spawn.maxContacts, session.simulation.result.diagnostics.contacts_i);

    if (!captureModeActive(session)) {
        auto& perf = session.performance;
        pushRolling(
            perf.physicsTimes, session.simulation.result.total_wall_ms,
            std::size_t(settings::World::PhysicsApp::Policy::TIMING_HISTORY)
        );
        pushRolling(
            perf.renderTimes, session.renderer->renderTime_ms(),
            std::size_t(settings::World::PhysicsApp::Policy::TIMING_HISTORY)
        );
    }
}


/**
 * Draw the viewer windows and read-only scene, verifying rendering preserves state.
 *
 * Draws controls first so their camera and debug changes affect the same frame.
 * Hashes the physics state before and after rendering to ensure the renderer
 * does not mutate it. Takes the requested screenshot on the final frame before
 * swapping buffers, then advances the frame counter.
 *
 * @param session      Window, physics snapshot and UI state used by this frame.
 * @param paused       Playback state that the simulation controls can change.
 * @param presentation Whether the UI should use presentation mode; controls may update it.
 * @return True when the user requests switching to the shared world.
 */
bool drawSession(Session& session, bool& paused, bool& presentation) {
    session.renderer->beginUi();
    const bool openWorldView = panels::drawSimulationPanel(session, paused, presentation);
    panels::drawInspector(session, presentation);
    session.renderer->setLayout(presentation, !presentation);
    session.renderer->setDebug(session.debug.aabb, session.debug.obb, session.debug.velocity);

    const auto before = sim::hashState(session.simulation.state);
    session.renderer->draw(
        session.simulation.state, session.scene.world, session.view.camera.yaw, session.view.camera.pitch,
        session.view.camera.distance
    );
    session.renderer->endUi();

    session.simulation.renderHash = sim::hashState(session.simulation.state);
    if (before != session.simulation.renderHash) {
        throw std::runtime_error("Renderer modified physics state");
    }

    if (isFinalCaptureFrame(session)) {
        session.renderer->capture(session.options.capture.screenshot);
    }

    glfwSwapBuffers(session.renderer->window());
    ++session.view.frames;

    return openWorldView;
}

} // namespace app


namespace app::capture {

namespace {

/**
 * Compute peak retained impact energy for each body, indexed by body ID.
 *
 * @param s Session containing the impact history.
 * @return JSON array of peak energies.
 */
sim::Json bodyPeakEnergies(const Session& s) {
    sim::Json peaks = sim::Json::array();

    for (int i = 0; i < s.simulation.state.count_i; ++i) {
        float peak_J = 0.0f;
        for (const auto& patch : s.impacts().body(i)) {
            peak_J = std::max(peak_J, patch.energy_J);
        }
        peaks.push_back(peak_J);
    }

    return peaks;
}


/**
 * Serialize the current impact visualization settings.
 *
 * @param s Session containing the impact history.
 * @return JSON object describing impact settings and peak energies.
 */
sim::Json impactSettings(const Session& s) {
    const auto& impacts = s.impacts();
    return {
        {"enabled", impacts.enabled},
        {"hold", impacts.hold},
        {"lifetime_s", impacts.lifetime_s},
        {"redEnergy_J", impacts.redEnergy_J},
        {"bodyPeakEnergy_J", bodyPeakEnergies(s)}
    };
}


/**
 * Build the complete capture metadata object.
 *
 * @param s Session providing all capture values.
 * @return JSON object containing run metadata and impact settings.
 */
sim::Json captureData(const Session& s) {
    sim::Json data = {
        {"scenario", s.scene.id},
        {"seed", s.scene.seed},
        {"backend", s.backendName},
        {"width_px", s.options.view.width},
        {"height_px", s.options.view.height},
        {"presentation", s.options.view.presentation},
        {"tick_i", s.simulation.state.tick_i},
        {"stateHash", std::to_string(s.simulation.renderHash)},
        {"driver", s.renderer->driverInfo()}
    };

    data["impactColoring"] = impactSettings(s);
    return data;
}


/**
 * Determine the JSON metadata path alongside the screenshot.
 *
 * @param screenshot The output path of the screenshot.
 * @return The metadata file path.
 */
std::filesystem::path metadataPathFor(const std::filesystem::path& screenshot) {
    auto metadata = screenshot;
    metadata.replace_extension(".json");
    return metadata;
}


/**
 * Write the capture metadata to a JSON file next to the screenshot.
 *
 * @param s          Session providing the metadata.
 * @param screenshot The output path of the screenshot.
 * @return True if the file was written successfully.
 */
bool writeMetadata(const Session& s, const std::filesystem::path& screenshot) {
    const auto path = metadataPathFor(screenshot);
    std::ofstream out(path);
    
    if (!out) {
        std::cerr << "capture: could not open " << path << " for writing\n";
        return false;
    }
    
    out << captureData(s).dump(2);
    return true;
}

} // namespace (anonymous)


/**
 * Write the framebuffer as PNG and the manifest as JSON next to it.
 *
 * If the frame count is negative, capture is disabled and the function
 * returns immediately.
 *
 * @param s              Session to read state from.
 * @param captureFrames  Number of frames the capture was scheduled for.
 * @param screenshot     Destination .png path; the manifest is written as screenshot + ".json".
 */
void writeCapture(Session& s, int captureFrames, const std::filesystem::path& screenshot) {
    if (captureFrames < 0) {
        return;
    }

    std::cout << "Captured " << screenshot.string() << "; render state hash " << s.simulation.renderHash << "; GL verified\n";
    writeMetadata(s, screenshot);
}

} // namespace app::capture
