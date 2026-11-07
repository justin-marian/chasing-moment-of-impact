/**
 * @file
 * Viewer entry point and navigation between glass physics and the shared world.
 *
 * Session setup and frame processing are kept separate. This translation unit
 * handles the main loop, window polling, and switching between the standalone
 * physics viewer and the shared world.
 */

#include "client/physics_app/cli.h"
#include "client/physics_app/run_session.h"

#ifdef SIM_BUILD_MMO
#include "client/world_app/session_loop.h"
#endif

#include <chrono>
#include <iostream>
#include <stdexcept>

#include <GLFW/glfw3.h>

#ifdef _WIN32
/**
 * Vendor-defined Optimus export; prefer the NVIDIA adapter for CPU and CUDA
 * rendering alike.
 * https://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
 */
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
}
#endif

namespace app {

/**
 * Run the main viewer loop.
 *
 * Handles session initialization, physics stepping, and rendering until the
 * window is closed or a capture sequence completes. Visual frames are driven
 * by a live clock, while physics advances using its own fixed time step.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code (0 on success).
 * @throws std::runtime_error On initialization or rendering failure.
 */
int run(int argc, char** argv) {
    auto options = parseViewerOptions(argc, argv);
    Session session;
    initializeSession(session, options);

#ifdef SIM_BUILD_MMO
    if (options.scene.mode == "world" || options.network.port) {
        client::run(*session.renderer, makeClientOptions(options));
        if (options.capture.frames >= 0 || glfwWindowShouldClose(session.renderer->window()))
            return 0;
    }
#endif

    if (session.scene.id == "impact")
        session.view.camera = {.28f, .25f, 22};

    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    bool presentation = options.view.presentation;
    bool paused = options.simulation.paused;

    while (!glfwWindowShouldClose(session.renderer->window()) && (options.capture.frames < 0 || session.view.frames < options.capture.frames)) {
        glfwPollEvents();

        const auto now = Clock::now();
        const double elapsed = std::chrono::duration<double>(now - previous).count();
        previous = now;

        advanceSimulation(session, paused, elapsed);

#ifdef SIM_BUILD_MMO
        if (drawSession(session, paused, presentation)) {
            client::run(*session.renderer, {});
            previous = Clock::now();
            session.simulation.accumulator = 0;
        }
#else
        drawSession(session, paused, presentation);
#endif
    }

    if (options.capture.frames >= 0)
        capture::writeCapture(session, options.capture.frames, options.capture.screenshot);

    return 0;
}

} // namespace app


/**
 * Entry point.
 *
 * Delegates to the app runner and translates unhandled exceptions into a
 * diagnostic message and non-zero exit code.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char** argv) {
    try {
        return app::run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "VIEWER ERROR: " << e.what() << '\n';
        return 1;
    }
}
