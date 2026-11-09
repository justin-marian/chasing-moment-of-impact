/**
 * @file
 * Expose the glass-scene renderer, native UI lifecycle, camera controls and capture boundary.
 *
 * Provides OpenGL rendering of read-only rigid-body snapshots, panels and diagnostics.
 */

#pragma once

#include "client/render_gl/impact_history.h"
#include "shared/sim/state.h"
#include "client/render_gl/interface_style.h"

#include <filesystem>
#include <memory>


/**
 * Borrowed native window handle; the windowing library owns its creation and destruction.
 */
struct GLFWwindow;


namespace render {

/**
 * Read-only SI snapshots; rendering never changes physics.
 */
class Renderer {
public:
    struct Impl;

private:
    std::unique_ptr<Impl> impl;

public:
    /**
     * Create the OpenGL window, render resources and upstream UI interface.
     *
     * @param visible Whether the window is shown interactively or hidden for a capture.
     * @param width   Drawable target width in pixels.
     * @param height  Drawable target height in pixels.
     */
    Renderer(bool visible = true, int width = 1440, int height = 900);


    /**
     * Release this owner's resources once; borrowed state is not destroyed.
     */
    ~Renderer();


    /**
     * Return the owned native window handle.
     *
     * @return The native window pointer.
     */
    GLFWwindow* window() const;


    /**
     * Draw read-only SI state using the active camera, surface shading and overlays.
     *
     * @param state      Host simulation arrays, active body count and authoritative tick in SI units.
     * @param config     Fixed-step world, surface response and solver settings; physical quantities use SI units.
     * @param yaw_rad    Camera orbit yaw around the vertical axis, in radians.
     * @param pitch_rad  Camera orbit elevation in radians.
     * @param distance_m Distance from the camera to its focus point, in metres.
     */
    void draw(
        const sim::State& state, const sim::WorldConfig& config,
        float yaw_rad, float pitch_rad, float distance_m
    );


    /**
     * Select presentation mode and the optional right inspector.
     *
     * @param presentation Whether the viewer should use its presentation-only overlay mode.
     * @param rightPanel   Whether the optional body inspector is visible.
     */
    void setLayout(bool presentation, bool rightPanel);


    /**
     * Select bounds and velocity overlays.
     *
     * @param aabb     Whether world-axis body bounds are shown as a debug overlay.
     * @param obb      Whether oriented body bounds are shown as a debug overlay.
     * @param velocity Whether body velocity vectors are shown as a debug overlay.
     */
    void setDebug(bool aabb, bool obb, bool velocity);


    /**
     * Return the retained visual-only impact patches.
     *
     * @return Reference to the impact history.
     */
    ImpactHistory& impactHistory();


    /**
     * Return loaded typography roles.
     *
     * @return The font structure.
     */
    const Fonts& fonts() const;


    /**
     * Begin a new UI frame.
     */
    void beginUi();


    /**
     * Draw and present the current UI frame.
     */
    void endUi();


    /**
     * Create missing parents and write the RGB framebuffer as a deterministic PNG capture.
     *
     * @param path Destination image path; missing parent directories are created.
     */
    void capture(const std::filesystem::path& path);


    /**
     * Return separately measured OpenGL render time in milliseconds.
     *
     * @return Render time in milliseconds.
     */
    double renderTime_ms() const;


    /**
     * Describe the active OpenGL implementation for capture evidence.
     *
     * @return Driver information string.
     */
    std::string driverInfo() const;
};

} // namespace render
