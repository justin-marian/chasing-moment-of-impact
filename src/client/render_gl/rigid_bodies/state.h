/**
 * @file
 * Share private glass-renderer resources between its implementation compilation units.
 *
 * Defines the private implementation structure for the main renderer.
 * It owns all OpenGL objects, shader programs, and instance buffers required
 * by the rendering passes, ensuring a single authoritative owner per process.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "settings/world/constants.h"
#include "settings/world/render_constants.h"
#include "client/render_gl/meshes/unit_shapes.h"

#include "client/render_gl/rigid_bodies/session_renderer.h"
#include "client/render_gl/impact_history.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>


namespace render {

/**
 * Read-only rigid-body renderer, shared UI, and frame capture owner.
 *
 * Passes receive a reference to this structure and read only the fields
 * relevant to their specific responsibility. Copy and move operations are
 * explicitly deleted to prevent duplicating the underlying window and GL
 * handles, which would result in double-free errors.
 */
struct Renderer::Impl {
    /* Window handle and timing state. */
    GLFWwindow* window{};
    GLuint queries[2]{};
    int frame{};
    double render_ms{};

    /* Shader programs. */
    GLuint body{}, wall{}, background{}, floor{}, shadow{}, line{};

    /* Vertex arrays and buffers. */
    GLuint emptyVao{}, lineVao{}, lineBuffer{};
    GLuint impactBuffer{};

    /* Mesh resources. */
    std::unique_ptr<InstancedMesh> plane;
    std::array<std::unique_ptr<InstancedMesh>, settings::Collisions::Geometry::SHAPE_COUNT> meshes;

    /* Metadata and typography. */
    std::string driver;
    Fonts fonts;

    /* Per-frame instance data grouped by shape. */
    std::array<std::vector<Instance>, settings::Collisions::Geometry::SHAPE_COUNT> groups;
    std::vector<Instance> walls;
    std::vector<Instance> shadows;
    std::vector<LineVertex> lines;

    /* Contact flash tracking. */
    std::array<bool, settings::Collisions::Storage::MAX_BODIES> touching{};
    std::array<double, settings::Collisions::Storage::MAX_BODIES> flashUntil{};

    /* Impact telemetry. */
    ImpactHistory impacts;
    std::vector<ImpactGpu> impactData = std::vector<ImpactGpu>(settings::World::Rendering::Gpu::IMPACT_SLOT_COUNT);

    /* UI layout and debug flags. */
    bool presentation{}, rightPanel{};
    bool aabb{}, obb{}, velocity{};

    /* 
     * Members are destroyed after the destructor body returns, by which
     * point the window and GL context are already gone. Their destructors
     * must not touch GL or UI state. Tear down anything that does
     * explicitly in the destructor body, while the context is still alive.
     */
    Impl() = default;
    ~Impl();
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
};

} // namespace render