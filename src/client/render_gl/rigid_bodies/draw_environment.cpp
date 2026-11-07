/**
 * @file
 * Draw the background, floor, shadows, and debug lines for the physics scene.
 *
 * Provides OpenGL rendering passes for the visual environment and diagnostic
 * overlays. State configuration is kept minimal and explicit per pass.
 */

#include "client/render_gl/gpu_resources.h"
#include "client/render_gl/rigid_bodies/draw.h"
#include "client/render_gl/rigid_bodies/state.h"

#include <array>
#include <cmath>

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>


namespace render::passes {

/**
 * Draw the fullscreen background triangle.
 *
 * Disables depth testing and blending, as the background owns every pixel it
 * writes. No vertex attributes are read; positions are derived in the shader
 * from the vertex index. An empty vertex array object is bound for the draw
 * call to be well-formed.
 *
 * @param impl     Renderer state containing the background shader program.
 * @param heightPx Window height passed to the shader uniform.
 */
void drawBackground(const Renderer::Impl& impl, int heightPx) {
    // A background pass owns every pixel it writes; disable the states that
    // would otherwise discard or blend those writes.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(impl.background);
    uniform(impl.background, "uHeight_px", float(heightPx));

    // No per-vertex attributes are read: positions are derived in the shader
    // from the vertex index. A vertex array object still has to be bound for
    // the draw call to be well-formed, even though it carries nothing.
    glBindVertexArray(impl.emptyVao);
    
    // background.vert derives one fullscreen triangle from gl_VertexID; no vertex buffer is needed.
    glDrawArrays(GL_TRIANGLES, 0, 3);
}


/**
 * Draw the world floor plane.
 *
 * Tests against the depth buffer without writing to it, allowing the shadow
 * pass to draw on the same surface. Culling is disabled so the floor remains
 * visible from either side.
 *
 * @param impl        Renderer state containing the floor program and mesh.
 * @param camera      Camera view-projection matrix.
 * @param halfExtents Arena half extents for sizing the floor plane.
 */
void drawFloor(const Renderer::Impl& impl, const glm::mat4& camera, const glm::vec3& halfExtents) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    // Test against depth without preventing the shadow pass.
    glDepthMask(GL_FALSE);
    // Floor remains visible from either side.
    glDisable(GL_CULL_FACE);

    glUseProgram(impl.floor);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(camera));

    const auto plane = groundPlane(
        {0, -halfExtents.y, 0},
        {halfExtents.x, halfExtents.z, 1}
    );
    
    impl.plane->draw({{plane, {1, 1, 1, 1}}});
}


/**
 * Generate ground shadow instances for bodies close to the floor.
 *
 * Bodies above a certain height threshold do not cast a visible shadow and
 * are skipped to save draw calls.
 *
 * @param impl        Renderer state containing the shadow instance buffer.
 * @param s           Simulation state providing body positions and extents.
 * @param halfExtents Arena half extents for the floor plane.
 */
void buildShadows(Renderer::Impl& impl, const sim::State& s, const glm::vec3& halfExtents) {
    constexpr float SHADOW_FADE_DISTANCE_FACTOR = 3.0f;
    constexpr float SHADOW_Y_OFFSET = .003f;
    constexpr float SHADOW_SCALE_FACTOR = 1.7f;

    impl.shadows.clear();

    for (int i = 0; i < s.count_i; ++i) {
        auto p = s.position_m[i];
        float radius = s.boundingRadius_m[i];
        float above = p.y + halfExtents.y - s.aabbExtent_m[i].y;
        
        // Skip bodies that are too high up to cast a visible ground shadow.
        if (above > radius * SHADOW_FADE_DISTANCE_FACTOR) {
            continue;
        }

        // Fade out the shadow as the body rises off the floor.
        float fade = glm::clamp(1 - above / (radius * SHADOW_FADE_DISTANCE_FACTOR), 0.0f, 1.0f);
        auto world = groundPlane(
            {p.x, -halfExtents.y + SHADOW_Y_OFFSET, p.z}, 
            {radius * SHADOW_SCALE_FACTOR, radius * SHADOW_SCALE_FACTOR, 1}
        );

        impl.shadows.push_back({world, {1, 1, 1, 1}, {1, 0, fade, 0}});
    }
}


/**
 * Draw the ground shadow instances.
 *
 * Uses the same blend and depth state as the floor pass, because both draw
 * onto the same plane. Draw order (floor then shadow) is what darkens the grid.
 *
 * @param impl   Renderer state containing the shadow program and mesh.
 * @param camera Camera view-projection matrix.
 */
void drawShadows(const Renderer::Impl& impl, const glm::mat4& camera) {
    glUseProgram(impl.shadow);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(camera));
    
    // Relies on state left over from drawFloor (blend on, depth test on, mask off).
    impl.plane->draw(impl.shadows);
}


namespace {

/**
 * Push one line segment as two vertices.
 *
 * @param out   Destination vertex buffer.
 * @param a     Start position.
 * @param b     End position.
 * @param color Line color.
 */
void emitLine(std::vector<LineVertex>& out, glm::vec3 a, glm::vec3 b, glm::vec4 color) {
    out.push_back({a, color});
    out.push_back({b, color});
}


/**
 * Emit the 12 edges of a box described by the given world matrix.
 *
 * A bit trick enumerates the edges without a switch: for each corner, flipping
 * one of the three low bits moves it along the corresponding axis.
 *
 * @param out   Destination vertex buffer.
 * @param world Transformation matrix for the box.
 * @param color Line color.
 */
void emitBox(std::vector<LineVertex>& out, glm::mat4 world, glm::vec4 color) {
    std::array<glm::vec3, 8> p;
    
    for (int i = 0; i < 8; ++i) {
        // Convert 3-bit binary index into a (+/-1, +/-1, +/-1) local corner coordinate.
        p[i] = glm::vec3(world * glm::vec4(
            i & 1 ? 1.0f : -1.0f, 
            i & 2 ? 1.0f : -1.0f, 
            i & 4 ? 1.0f : -1.0f, 
            1
        ));
    }

    for (int i = 0; i < 8; ++i) {
        // Connect each corner to its neighbors along the X, Y, and Z axes.
        for (int bit : {1, 2, 4}) {
            if (!(i & bit)) {
                emitLine(out, p[i], p[i | bit], color);
            }
        }
    }
}

} // namespace (anonymous)


/**
 * Build debug line geometry for the arena bounds, body bounds, and velocities.
 *
 * Highlights the arena border when the cursor is near it, indicating that
 * the next click will resize rather than interact. Per-body overlays draw on
 * top of the scene with depth testing disabled.
 *
 * @param impl        Renderer state containing the line buffer and debug flags.
 * @param s           Simulation state providing body positions and extents.
 * @param halfExtents Arena half extents for the border.
 * @param eye         Camera eye position (currently unused).
 * @param left        Left UI margin in pixels.
 * @param width       Total width of the window.
 * @param right       Right UI margin in pixels.
 * @param canvasWidth Width of the central canvas area.
 * @param height      Total height of the window.
 */
void buildLines(
    Renderer::Impl& impl, const sim::State& s, 
    const glm::vec3& halfExtents, 
    const glm::vec3& eye, int left, int width, int right, int canvasWidth, int height
) {
    constexpr glm::vec4 ARENA_BORDER_COLOR = {211.0f / 255, 209.0f / 255, 203.0f / 255, .4f};
    constexpr glm::vec4 ARENA_HOVER_COLOR = {35.0f / 255, 131.0f / 255, 226.0f / 255, .3f};
    constexpr glm::vec4 AABB_COLOR = {35.0f / 255, 131.0f / 255, 226.0f / 255, .4f};
    constexpr glm::vec4 OBB_COLOR = {35.0f / 255, 131.0f / 255, 226.0f / 255, .6f};
    constexpr glm::vec4 VELOCITY_COLOR = {15.0f / 255, 123.0f / 255, 108.0f / 255, .8f};
    
    constexpr float HOVER_EDGE_FRACTION = .08f;
    constexpr float HOVER_BOTTOM_FRACTION = .92f;
    constexpr float VELOCITY_SCALE = .2f;

    (void)eye;
    impl.lines.clear();

    // Arena border is always shown; the presentation layout skips the rest.
    emitBox(impl.lines, glm::scale(glm::mat4(1), halfExtents), ARENA_BORDER_COLOR);
    if (impl.presentation) {
        return;
    }

    // Highlight the arena border when the cursor is near it; this is the
    // in-app cue that the next click will resize rather than interact.
    auto mouse = ImGui::GetIO().MousePos;
    bool hover = mouse.x > float(left) && mouse.x < float(width - right) &&
                 (mouse.x < float(left) + float(canvasWidth) * HOVER_EDGE_FRACTION ||
                  mouse.x > float(width - right) - float(canvasWidth) * HOVER_EDGE_FRACTION ||
                  mouse.y < float(height) * HOVER_EDGE_FRACTION || 
                  mouse.y > float(height) * HOVER_BOTTOM_FRACTION);
    if (hover) {
        emitBox(impl.lines, glm::scale(glm::mat4(1), halfExtents), ARENA_HOVER_COLOR);
    }

    if (!impl.aabb && !impl.obb && !impl.velocity) {
        return;
    }

    // Per-body overlays draw on top of the scene; disabling depth lets an
    // occluded body still show its bounds.
    glDisable(GL_DEPTH_TEST);
    
    for (int i = 0; i < s.count_i; ++i) {
        auto p = s.position_m[i];
        auto h = s.halfExtents_m[i];
        auto e = s.aabbExtent_m[i];
        auto q = s.orientation[i];
        auto translation = glm::translate(glm::mat4(1), glm::vec3(p.x, p.y, p.z));
        
        if (impl.aabb) {
            // Axis-aligned bounds (no rotation).
            emitBox(
                impl.lines, 
                translation * glm::scale(glm::mat4(1), glm::vec3(e.x, e.y, e.z)),
                AABB_COLOR
            );
        }
        
        if (impl.obb) {
            // Oriented bounds (includes rotation).
            emitBox(
                impl.lines,
                translation * glm::mat4_cast(glm::quat(q.w, q.x, q.y, q.z)) *
                    glm::scale(glm::mat4(1), glm::vec3(h.x, h.y, h.z)),
                OBB_COLOR
            );
        }
        
        if (impl.velocity) {
            // Draw a line indicating the direction and magnitude of the body's velocity.
            auto v = s.velocity_mps[i];
            emitLine(
                impl.lines, 
                {p.x, p.y, p.z}, 
                {p.x + v.x * VELOCITY_SCALE, p.y + v.y * VELOCITY_SCALE, p.z + v.z * VELOCITY_SCALE},
                VELOCITY_COLOR
            );
        }
    }
}


/**
 * Upload and draw the accumulated debug line geometry.
 *
 * @param impl   Renderer state containing the line buffer.
 * @param camera Camera view-projection matrix.
 */
void drawLines(Renderer::Impl& impl, const glm::mat4& camera) {
    glUseProgram(impl.line);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(camera));
    glBindVertexArray(impl.lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, impl.lineBuffer);
    
    // Stream the lines to the GPU, replacing the previous frame's buffer.
    glBufferData(
        GL_ARRAY_BUFFER, 
        static_cast<GLsizeiptr>(impl.lines.size() * sizeof(LineVertex)), 
        impl.lines.data(),
        GL_STREAM_DRAW
    );
    
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(impl.lines.size()));
}

} // namespace render::passes
