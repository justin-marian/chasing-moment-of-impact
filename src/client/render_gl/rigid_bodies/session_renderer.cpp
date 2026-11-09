/**
 * @file
 * Glass geometry, surface shading, debug overlays, and framebuffer capture.
 *
 * OpenGL draws snapshots; physics remains in the chosen CPU/CUDA backend.
 * This file owns the implementation lifetime, the constructor/destructor, and
 * the per-frame draw sequence. Object and enclosure passes live beside this
 * file; GPU creation is owned by the resources module.
 */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO

#include "settings/collisions/constants.h"
#include "settings/world/constants.h"
#include "settings/world/render_constants.h"

#include "client/render_gl/rigid_bodies/session_renderer.h"
#include "client/render_gl/rigid_bodies/state.h"
#include "client/render_gl/meshes/unit_shapes.h"
#include "client/render_gl/gpu_resources.h"
#include "client/render_gl/rigid_bodies/draw.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stb_image_write.h>


namespace render {

namespace {

/**
 * Upload retained visual marks independently of the draw-pass order.
 *
 * Packs retained impact patches and uploads the body and wall blocks.
 *
 * @param state     Renderer state containing the impact buffer and data.
 * @param bodyCount Number of active bodies to upload.
 */
void uploadImpactPatches(Renderer::Impl& state, int bodyCount) {
    packImpactPatches(state.impactData, state.impacts, bodyCount);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, state.impactBuffer);

    if (bodyCount) {
        glBufferSubData(
            GL_SHADER_STORAGE_BUFFER, 0,
            static_cast<GLsizeiptr>(bodyCount * settings::Collisions::Impacts::PATCHES_PER_SURFACE * sizeof(ImpactGpu)),
            state.impactData.data()
        );
    }

    glBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        static_cast<GLintptr>(settings::World::Rendering::Gpu::WALL_OFFSET * sizeof(ImpactGpu)),
        6 * settings::Collisions::Impacts::PATCHES_PER_SURFACE * sizeof(ImpactGpu),
        state.impactData.data() + settings::World::Rendering::Gpu::WALL_OFFSET
    );

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, state.impactBuffer);
}


/**
 * Read a completed query before starting the next GPU sample.
 *
 * The GPU timer uses alternating queries; the result of the query from two
 * frames ago is known to be ready and can be read without stalling.
 *
 * @param state Renderer state containing the query objects and timing data.
 */
void beginGpuTiming(Renderer::Impl& state) {
    constexpr double NANOSECONDS_TO_MILLISECONDS = 1e-6;
    constexpr int TIMING_DELAY_FRAMES = 2;

    if (state.frame >= TIMING_DELAY_FRAMES) {
        GLint ready = 0;
        glGetQueryObjectiv(state.queries[state.frame % 2], GL_QUERY_RESULT_AVAILABLE, &ready);

        if (ready) {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(state.queries[state.frame % 2], GL_QUERY_RESULT, &ns);
            state.render_ms = double(ns) * NANOSECONDS_TO_MILLISECONDS;
        }
    }

    glBeginQuery(GL_TIME_ELAPSED, state.queries[state.frame % 2]);
}

} // namespace (anonymous)


/**
 * Initialize the renderer context, shaders, geometry, and user interface.
 *
 * @param visible Whether the window should be visible.
 * @param width   Initial window width.
 * @param height  Initial window height.
 */
Renderer::Renderer(bool visible, int width, int height) : impl(std::make_unique<Impl>()) {
    initializeContext(*impl, visible, width, height);
    initializePrograms(*impl);
    initializeGeometry(*impl);
    initializeInterface(*impl);
}


/**
 * Default destructor; resource cleanup is handled by the implementation object.
 */
Renderer::~Renderer() = default;


/**
 * Get the underlying GLFW window handle.
 *
 * @return The GLFW window pointer.
 */
GLFWwindow* Renderer::window() const {
    return impl->window;
}


/**
 * Get the loaded typography resources.
 *
 * @return The font structure.
 */
const Fonts& Renderer::fonts() const {
    return impl->fonts;
}


/**
 * Set the layout mode for the current frame.
 *
 * @param p Whether presentation mode is active.
 * @param r Whether the right panel is visible.
 */
void Renderer::setLayout(bool p, bool r) {
    impl->presentation = p;
    impl->rightPanel = r;
}


/**
 * Set the debug visualization flags.
 *
 * @param a Whether to draw AABBs.
 * @param o Whether to draw OBBs.
 * @param v Whether to draw velocity vectors.
 */
void Renderer::setDebug(bool a, bool o, bool v) {
    impl->aabb = a;
    impl->obb = o;
    impl->velocity = v;
}


/**
 * Begin a new ImGui frame.
 */
void Renderer::beginUi() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}


/**
 * End the current ImGui frame and render the draw data.
 *
 * @throws std::runtime_error If an OpenGL error occurs during UI rendering.
 */
void Renderer::endUi() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    auto error = glGetError();
    if (error != GL_NO_ERROR) {
        throw std::runtime_error("OpenGL UI error: " + std::to_string(error));
    }
}


/**
 * Render one complete frame of the physics scene.
 *
 * The order of the passes below is the back-to-front drawing order the original
 * single-file draw used; each pass is one call into its own file under passes/.
 *
 * @param s        Simulation state to render.
 * @param c        World configuration.
 * @param yaw      Camera yaw angle in radians.
 * @param pitch    Camera pitch angle in radians.
 * @param distance Camera distance from the origin.
 * @throws std::runtime_error If an OpenGL error occurs during the frame.
 */
void Renderer::draw(const sim::State& s, const sim::WorldConfig& c, float yaw, float pitch, float distance) {
    constexpr float CAMERA_FOV_DEGREES = 48.0f;
    constexpr float CAMERA_NEAR_PLANE = 0.1f;
    constexpr float CAMERA_FAR_PLANE = 400.0f;

    int width = 0, height = 0;
    int logicalWidth = 0, logicalHeight = 0;

    glfwGetFramebufferSize(impl->window, &width, &height);
    glfwGetWindowSize(impl->window, &logicalWidth, &logicalHeight);

    if (height == 0 || logicalHeight == 0) {
        return;
    }

    /* Logical-to-framebuffer scale keeps the layout proportionally correct on retina displays. */
    int left = 0;
    int right = 0;
    int canvasWidth = std::max(1, width - left - right);

    glViewport(0, 0, width, height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(left, 0, canvasWidth, height);

    uploadImpactPatches(*impl, s.count_i);
    beginGpuTiming(*impl);

    passes::drawBackground(*impl, height);

    glm::vec3 eye(
        distance * std::cos(pitch) * std::sin(yaw),
        distance * std::sin(pitch),
        distance * std::cos(pitch) * std::cos(yaw)
    );

    auto camera = glm::perspective(
        glm::radians(CAMERA_FOV_DEGREES),
        float(canvasWidth) / float(height),
        CAMERA_NEAR_PLANE,
        CAMERA_FAR_PLANE
    ) * glm::lookAt(eye, glm::vec3(0), glm::vec3(0, 1, 0));

    glm::vec3 halfExtents(c.boxHalfExtents_m.x, c.boxHalfExtents_m.y, c.boxHalfExtents_m.z);

    passes::drawFloor(*impl, camera, halfExtents);
    passes::buildShadows(*impl, s, halfExtents);
    passes::drawShadows(*impl, camera);
    passes::buildBodies(*impl, s, c);
    passes::drawBodies(*impl, camera, eye);
    passes::buildWalls(*impl, eye, halfExtents);
    passes::drawWalls(*impl, camera, eye);
    passes::buildLines(*impl, s, halfExtents, eye, left, width, right, canvasWidth, height);
    passes::drawLines(*impl, camera);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEndQuery(GL_TIME_ELAPSED);
    ++impl->frame;

    glViewport(0, 0, width, height);

    auto error = glGetError();
    if (error != GL_NO_ERROR) {
        throw std::runtime_error("OpenGL error: " + std::to_string(error));
    }
}


/**
 * Access the impact history owned by the renderer.
 *
 * @return Reference to the impact history.
 */
ImpactHistory& Renderer::impactHistory() {
    return impl->impacts;
}


/**
 * Get the duration of the last complete GPU render pass.
 *
 * @return Render time in milliseconds.
 */
double Renderer::renderTime_ms() const {
    return impl->render_ms;
}


/**
 * Get the OpenGL driver information string.
 *
 * @return Driver info string.
 */
std::string Renderer::driverInfo() const {
    return impl->driver;
}


/**
 * Read the framebuffer and save it as a PNG image.
 *
 * @param path Destination file path (must end in .png).
 * @throws std::runtime_error If the extension is invalid, directories cannot be
 *         created, or the image cannot be written.
 */
void Renderer::capture(const std::filesystem::path& path) {
    constexpr std::size_t CHANNELS = 3;

    if (path.extension() != ".png") {
        throw std::runtime_error("Capture path must end in .png");
    }

    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    int width = 0, height = 0;
    glfwGetFramebufferSize(impl->window, &width, &height);

    const std::size_t stride = std::size_t(width) * CHANNELS;
    std::vector<unsigned char> pixels(stride * std::size_t(height));

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    /* OpenGL rows start at the bottom; PNG rows start at the top. Only row order changes. */
    for (int row = 0; row < height / 2; ++row) {
        std::swap_ranges(
            pixels.begin() + std::size_t(row) * stride,
            pixels.begin() + std::size_t(row + 1) * stride,
            pixels.begin() + std::size_t(height - row - 1) * stride
        );
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Cannot write screenshot");
    }

    const auto write = [](void* context, void* data, int bytes) {
        static_cast<std::ofstream*>(context)->write(static_cast<const char*>(data), bytes);
    };

    if (!stbi_write_png_to_func(write, &output, width, height, int(CHANNELS), pixels.data(), int(stride)) || !output) {
        throw std::runtime_error("Cannot encode PNG screenshot");
    }
}

} // namespace render


namespace render {

namespace {

/**
 * Release every GL object the renderer owns.
 *
 * The GL context must be current when this runs; callers are responsible for
 * making it so. Every GL object must be deleted before the context (and its
 * window) is destroyed.
 *
 * @param impl The renderer implementation containing the GL resources.
 */
void releaseGlObjects(Renderer::Impl& impl) {
    impl.plane.reset();

    for (auto& mesh : impl.meshes) {
        mesh.reset();
    }

    glDeleteQueries(GLsizei(std::size(impl.queries)), impl.queries);

    /* The six render programs. They're independent; a single initialize list is only for brevity. */
    for (const GLuint program : {impl.body, impl.wall, impl.background, impl.floor, impl.shadow, impl.line}) {
        glDeleteProgram(program);
    }

    /* Single-instance GL objects (one buffer / VAO each). */
    glDeleteBuffers(1, &impl.lineBuffer);
    glDeleteBuffers(1, &impl.impactBuffer);
    glDeleteVertexArrays(1, &impl.lineVao);
    glDeleteVertexArrays(1, &impl.emptyVao);
}

} // namespace (anonymous)


/**
 * Destroy the implementation and release all associated resources.
 */
Renderer::Impl::~Impl() {
    if (!window) {
        return;
    }

    /* The GL context must be current for every subsequent GL call, including ImGui's own shutdown. */
    glfwMakeContextCurrent(window);

    /*
     * Shut down ImGui. The OpenGL3 and GLFW backends free the GL objects ImGui
     * created (fonts, VAOs, shaders); run before releaseGlObjects so each object
     * is freed by the code that created it, and before the window/context are
     * destroyed.
     */
    if (ImGui::GetCurrentContext()) {
        const auto& io = ImGui::GetIO();
        if (io.BackendRendererUserData)
            ImGui_ImplOpenGL3_Shutdown();
        if (io.BackendPlatformUserData)
            ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    /* Release our own GL objects. Safe now: context is current and ImGui is no longer touching any GL state. */
    releaseGlObjects(*this);

    /* Tear down the window and GLFW. No GL call may run after this. */
    glfwDestroyWindow(window);
    glfwTerminate();
}

} // namespace render
