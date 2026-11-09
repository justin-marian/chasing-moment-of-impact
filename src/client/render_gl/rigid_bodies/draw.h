/**
 * @file
 * Rigid-body pass contracts grouped by one frame lifecycle.
 *
 * Declares the rendering passes for the background, floor, bodies, shadows,
 * walls, and debug lines. All passes assume the OpenGL context is active.
 */

#pragma once

#include "client/render_gl/rigid_bodies/state.h"


namespace render::passes {

/**
 * Draw the background triangle.
 *
 * @param impl   Renderer state containing the background program and empty vertex array.
 * @param height Framebuffer height in pixels used to size the top color cap.
 */
void drawBackground(const Renderer::Impl& impl, int height);


/**
 * Rebuild the per-shape instance groups from the current simulation state.
 *
 * Includes calculating the per-body contact-flash timer.
 *
 * @param impl Renderer state.
 * @param s    Simulation state providing positions, orientations, shapes, and contact flags.
 * @param c    World config used to convert the tick index into a render-side time.
 */
void buildBodies(Renderer::Impl& impl, const sim::State& s, const sim::WorldConfig& c);


/**
 * Draw the instance groups built by the previous call.
 *
 * Sets up the three-light rig, ambient hemisphere, surface shading texture, and
 * impact-heat uniforms once, then walks the shape buckets.
 *
 * @param impl   Renderer state.
 * @param camera Combined view-projection matrix.
 * @param eye    Camera world position used for specular and hemisphere terms.
 */
void drawBodies(Renderer::Impl& impl, const glm::mat4& camera, const glm::vec3& eye);


/**
 * Draw the arena floor.
 *
 * @param impl        Renderer resources used by the floor pass.
 * @param camera      Combined view-projection matrix.
 * @param halfExtents Arena half-extents for sizing and positioning the floor.
 */
void drawFloor(const Renderer::Impl& impl, const glm::mat4& camera, const glm::vec3& halfExtents);


/**
 * Rebuild the line list for this frame.
 *
 * @param impl        Renderer state.
 * @param s           Simulation state.
 * @param halfExtents Arena half-extents for the border box.
 * @param eye         Camera world position (currently unused).
 * @param left        Canvas left edge in framebuffer pixels.
 * @param width       Framebuffer width.
 * @param right       Canvas right inset in framebuffer pixels.
 * @param canvasWidth Canvas width in framebuffer pixels.
 * @param height      Framebuffer height.
 */
void buildLines(
    Renderer::Impl& impl, const sim::State& s, const glm::vec3& halfExtents,
    const glm::vec3& eye, int left, int width, int right, int canvasWidth, int height
);


/**
 * Upload and draw the line list as GL_LINES.
 *
 * @param impl   Renderer state containing the line buffer.
 * @param camera View-projection matrix.
 */
void drawLines(Renderer::Impl& impl, const glm::mat4& camera);


/**
 * Rebuild the contact-shadow instance list from the current simulation state.
 *
 * Bodies more than three times their bounding radius above the floor are
 * skipped, matching the physics contact cutoff so a non-contacting body
 * produces no visual shadow.
 *
 * @param impl        Renderer state containing the shadow instance buffer.
 * @param s           Simulation state providing body positions and extents.
 * @param halfExtents Arena half-extents for the floor plane.
 */
void buildShadows(Renderer::Impl& impl, const sim::State& s, const glm::vec3& halfExtents);


/**
 * Draw the shadow instances built by the previous call.
 *
 * @param impl   Renderer state containing the shadow program and mesh.
 * @param camera View-projection matrix.
 */
void drawShadows(const Renderer::Impl& impl, const glm::mat4& camera);


/**
 * Rebuild the wall instance list.
 *
 * Skips the face nearest the camera so the camera never sees the wall it is
 * currently behind.
 *
 * @param impl        Renderer state containing the wall instance array.
 * @param eye         Camera world position.
 * @param halfExtents Arena half-extents for sizing the wall faces.
 */
void buildWalls(Renderer::Impl& impl, const glm::vec3& eye, const glm::vec3& halfExtents);


/**
 * Sort the wall instances back-to-front and draw them blended.
 *
 * @param impl   Renderer state containing the wall program and mesh.
 * @param camera View-projection matrix.
 * @param eye    Camera position in world space used for sorting.
 */
void drawWalls(Renderer::Impl& impl, const glm::mat4& camera, const glm::vec3& eye);

} // namespace render::passes
