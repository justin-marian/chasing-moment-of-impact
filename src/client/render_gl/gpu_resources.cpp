#include "client/render_gl/gpu_resources.h"

#include "client/render_gl/interface_style.h"
#include "client/render_gl/meshes/unit_shapes.h"
#include "client/render_gl/rigid_bodies/state.h"
#include "settings/collisions/constants.h"
#include "settings/world/constants.h"
#include "settings/world/render_constants.h"
#include "settings/world/ui_constants.h"
#include "shared/utils/files.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iterator>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace render {
namespace {

/**
 * Expand the local GLSL includes before passing source to the OpenGL compiler.
 * Includes are evaluated once, remain inside the shader directory, and cannot
 * introduce another version directive. The root version line stays first.
 *
 * @param entry Vertex or fragment shader file.
 * @return Complete shader source with line numbers restored after each include.
 */
std::string loadShaderSource(const std::filesystem::path& entry) {
    const auto root = std::filesystem::weakly_canonical(entry.parent_path());
    std::set<std::filesystem::path> included;
    std::function<std::string(const std::filesystem::path&, int)> expand;

    expand = [&](const std::filesystem::path& requested, int depth) -> std::string {
        if (depth > 32)
            throw std::runtime_error("GLSL include nesting exceeds 32 files");

        const auto path = std::filesystem::weakly_canonical(requested);
        const auto relative = path.lexically_relative(root);
        if (relative.empty() || *relative.begin() == "..")
            throw std::runtime_error("GLSL include leaves shader directory: " + requested.string());
        if (!included.insert(path).second)
            return {};

        std::ifstream input(path);
        if (!input)
            throw std::runtime_error("Cannot read shader: " + path.string());

        std::ostringstream output;
        std::string line;
        int number = 0;

        while (std::getline(input, line)) {
            ++number;
            const auto first = line.find_first_not_of(" \t\r");
            const auto directive = first == std::string::npos ? std::string() : line.substr(first);
            if (directive.starts_with("#pragma once"))
                continue;

            if (depth && directive.starts_with("#version"))
                throw std::runtime_error("Included GLSL file declares a version: " + path.string());

            if (directive.starts_with("#include")) {
                const auto begin = directive.find('"');
                const auto end = directive.find('"', begin == std::string::npos ? begin : begin + 1);
                if (begin == std::string::npos || end == std::string::npos)
                    throw std::runtime_error("Malformed GLSL include in " + path.string());

                output << "\n#line 1\n"
                       << expand(path.parent_path() / directive.substr(begin + 1, end - begin - 1), depth + 1)
                       << "\n#line " << number + 1 << '\n';
            } else {
                output << line << '\n';
            }
        }
        return output.str();
    };

    return expand(entry, 0);
}

// Shared capacities and directions come from their C++ owner, not a second shader definition.
std::string shaderSettings() {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "#define COLLISIONS_GEOMETRY_PI " << float(settings::Collisions::Geometry::PI) << '\n';
    out << "#define COLLISIONS_IMPACTS_PATCHES_PER_SURFACE "
        << settings::Collisions::Impacts::PATCHES_PER_SURFACE << '\n';
    out << "#define WORLD_RENDERING_GPU_IMPACT_BINDING " << settings::World::Rendering::Gpu::IMPACT_BINDING
        << '\n';
    out << "#define WORLD_ASSETS_SKELETON_MAX_BONES " << settings::World::Assets::Skeleton::MAX_BONES << '\n';
    out << "#define WORLD_RENDERING_SCENE_IMPACT_MAX_AVATAR "
        << settings::World::Rendering::Scene::Impact::MAX_AVATAR << '\n';
    out << "#define WORLD_RENDERING_SCENE_IMPACT_MAX_SCENE "
        << settings::World::Rendering::Scene::Impact::MAX_SCENE_IMPACTS << '\n';
    out << "#define WORLD_GAMEPLAY_MAX_PLAYERS " << settings::World::Gameplay::MAX_PLAYERS << '\n';
    const auto direction = glm::make_vec3(settings::World::Rendering::Scene::Shadow::DIRECTION.data());
    out << "#define WORLD_RENDERING_SCENE_SHADOW_DIRECTION vec3(" << direction.x << ',' << direction.y << ','
        << direction.z << ")\n";
    return out.str();
}
} // namespace

std::filesystem::path asset(std::string_view relative) {
    return utils::findAsset(relative, ASSET_ROOT);
}

// The info log is read into a fixed buffer; a longer log is truncated rather
// than allocated for, because compile failures are diagnostic and a
// truncated log is still enough to act on.
GLuint loadShader(GLenum stage, const std::string& file) {
    auto source = loadShaderSource(asset("src/client/render_gl/shaders/" + file));
    source.insert(source.find('\n') + 1, shaderSettings());
    const char* code = source.c_str();

    GLuint object = glCreateShader(stage);
    glShaderSource(object, 1, &code, nullptr);
    glCompileShader(object);

    GLint ok = 0;
    glGetShaderiv(object, GL_COMPILE_STATUS, &ok);

    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(object, sizeof(log), nullptr, log);
        glDeleteShader(object);
        throw std::runtime_error(file + ": " + log);
    }

    return object;
}

GLuint linkProgram(const std::string& vertex, const std::string& fragment) {
    GLuint v = loadShader(GL_VERTEX_SHADER, vertex);
    GLuint f = loadShader(GL_FRAGMENT_SHADER, fragment);
    GLuint p = glCreateProgram();

    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);

    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        glDeleteProgram(p);
        throw std::runtime_error(log);
    }

    return p;
}

void uniform(GLuint p, const char* name, float value) {
    glUniform1f(glGetUniformLocation(p, name), value);
}

void uniform(GLuint p, const char* name, glm::vec3 value) {
    glUniform3fv(glGetUniformLocation(p, name), 1, glm::value_ptr(value));
}

// Piecewise sRGB decode: linear below 0.04045, exponential above. Matches
// the IEC 61966-2-1 curve the shaders use on the GPU side.
glm::vec3 linear(unsigned hex) {
    auto c = rgb(hex);
    glm::vec3 v(c.x, c.y, c.z);
    for (int i = 0; i < 3; ++i)
        v[i] = v[i] <= .04045f ? v[i] / 12.92f : std::pow((v[i] + .055f) / 1.055f, 2.4f);
    return v;
}

// Matches the shader's key/fill/rim convention: y-up, azimuth measured from
// +Z and elevation from the horizon.
glm::vec3 lightDirection(float azimuth, float elevation) {
    float a = glm::radians(azimuth), e = glm::radians(elevation);
    return {std::cos(e) * std::sin(a), std::sin(e), std::cos(e) * std::cos(a)};
}

// The 90-degree X rotation turns the XY unit quad into an XZ-plane face.
// The lift below the plane is applied at the call site, not here, because the
// floor and shadow passes want the same transform but different vertical offsets.
glm::mat4 groundPlane(glm::vec3 p, glm::vec3 extent) {
    return glm::translate(glm::mat4(1), p) *
           glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(1, 0, 0)) *
           glm::scale(glm::mat4(1), extent);
}

void initializeContext(Renderer::Impl& state, bool visible, int width, int height) {
    if (!glfwInit())
        throw std::runtime_error("GLFW initialization failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    state.window =
        glfwCreateWindow(width, height, "3D Virtual World Physics - Prototip 1 / 2", nullptr, nullptr);
    if (!state.window)
        throw std::runtime_error("OpenGL 4.6 context unavailable");

    glfwMakeContextCurrent(state.window);

    // Vertical sync; presentation blocks on the monitor's refresh.
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        throw std::runtime_error("GL loader failed");

    // The shader pipeline handles color explicitly; no automatic encode on write is wanted.
    glDisable(GL_FRAMEBUFFER_SRGB);

    const auto* rendererName = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const auto* versionName = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    state.driver = std::string(rendererName ? rendererName : "unknown") + " / " +
                   (versionName ? versionName : "unknown");
}

void initializePrograms(Renderer::Impl& state) {
    // Shared vertex shader with per-pass fragment shaders: the body
    // fragment drives the bodies themselves, the wall/floor/shadow variants
    // are alternative surface treatments over the same vertex stage.
    state.body = linkProgram("body.vert", "body.frag");
    state.wall = linkProgram("body.vert", "wall.frag");
    state.floor = linkProgram("body.vert", "floor.frag");
    state.shadow = linkProgram("body.vert", "shadow.frag");

    // Fully independent passes.
    state.background = linkProgram("background.vert", "background.frag");
    state.line = linkProgram("line.vert", "line.frag");

    // Impact storage consumed by the body and wall fragment shaders. The
    // shader declares it at binding slot 3; the render pass binds it to
    // that slot before drawing. This only allocates the backing storage.
    glGenBuffers(1, &state.impactBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, state.impactBuffer);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(state.impactData.size() * sizeof(ImpactGpu)),
        nullptr, GL_STREAM_DRAW
    );
}

void initializeGeometry(Renderer::Impl& state) {
    // Shape variants in the order the simulation indexes them: sphere,
    // cube, cylinder, cone. The last two differ only in cap shape.
    auto geometry = std::array<std::vector<Vertex>, settings::Collisions::Geometry::SHAPE_COUNT>{
        makeSphere(), makeCube(), makeRoundMesh(false), makeRoundMesh(true)
    };

    for (std::size_t i = 0; i < geometry.size(); ++i) {
        state.meshes[i] = std::make_unique<InstancedMesh>(geometry[i]);

        // A shape bucket may contain every body; reserve once before the frame loop.
        state.groups[i].reserve(settings::Collisions::Storage::MAX_BODIES);
    }

    state.plane = std::make_unique<InstancedMesh>(makeQuad());

    // No per-vertex attributes for the background: positions are derived
    // from the vertex index in the shader, but a bound VAO is still
    // required for the draw to be well-formed.
    glGenVertexArrays(1, &state.emptyVao);

    // Line pipeline: position in slot 0, color in slot 1, interleaved.
    glGenVertexArrays(1, &state.lineVao);
    glGenBuffers(1, &state.lineBuffer);
    glBindVertexArray(state.lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, state.lineBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), reinterpret_cast<void*>(offsetof(LineVertex, color))
    );

    // Alternating timestamp queries, one pair per in-flight frame, so the
    // CPU never stalls on a result that hasn't been written yet.
    glGenQueries(static_cast<GLsizei>(std::size(state.queries)), state.queries);
}

void initializeInterface(Renderer::Impl& state) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImFontConfig config;
    config.OversampleH = config.OversampleV = settings::World::Ui::Font::Resources::FONT_OVERSAMPLE;
    config.PixelSnapH = false;

    auto load = [&](const char* file, float sizePx) {
        const auto path = asset(std::string(settings::World::Ui::Font::Resources::FONT_DIRECTORY) + file);
        auto* face = io.Fonts->AddFontFromFileTTF(path.string().c_str(), sizePx, &config);
        if (!face)
            throw std::runtime_error("Cannot load font " + path.string());
        return face;
    };

    state.fonts.body = load("Inter-Regular.ttf", settings::World::Ui::Font::Sizes::BODY_PX);
    state.fonts.label = load("Inter-Medium.ttf", settings::World::Ui::Font::Sizes::LABEL_PX);
    state.fonts.section = load("Inter-SemiBold.ttf", settings::World::Ui::Font::Sizes::SECTION_PX);
    state.fonts.title = load("Inter-SemiBold.ttf", settings::World::Ui::Font::Sizes::TITLE_PX);
    state.fonts.mono = load("JetBrainsMono-Regular.ttf", settings::World::Ui::Font::Sizes::LABEL_PX);
    state.fonts.hint = load("Inter-Regular.ttf", settings::World::Ui::Font::Sizes::HINT_PX);
    state.fonts.parameterLabel = load("Inter-Medium.ttf", settings::World::Ui::Font::Sizes::PARAMETER_PX);
    state.fonts.parameterValue =
        load("JetBrainsMono-Regular.ttf", settings::World::Ui::Font::Sizes::PARAMETER_PX);
    state.fonts.username = load("Inter-SemiBold.ttf", settings::World::Ui::Font::Sizes::BODY_PX);

    io.FontDefault = state.fonts.body;

    applyInterfaceStyle();
    ImGui_ImplGlfw_InitForOpenGL(state.window, true);
    ImGui_ImplOpenGL3_Init(settings::World::Rendering::Shaders::VERSION);
}

namespace {

// Copy one bank of settings::Collisions::Impacts::PATCHES_PER_SURFACE into the flat buffer at the given slot.
// Radius lives in pointRadius.w, current strength in normalStrength.w; every
// other component is a straight copy of the source patch.
void packBank(std::vector<ImpactGpu>& out, int slot, const ImpactHistory& impacts, const auto& patches) {
    for (int k = 0; k < settings::Collisions::Impacts::PATCHES_PER_SURFACE; ++k) {
        const auto& p = patches[std::size_t(k)];
        out[std::size_t(slot) * settings::Collisions::Impacts::PATCHES_PER_SURFACE + std::size_t(k)] = {
            {p.point_m.x, p.point_m.y, p.point_m.z, p.radius_m},
            {p.normal.x, p.normal.y, p.normal.z, impacts.strength(p)}
        };
    }
}

} // namespace

void packImpactPatches(std::vector<ImpactGpu>& out, const ImpactHistory& impacts, int bodyCount) {
    for (int i = 0; i < bodyCount; ++i)
        packBank(out, i, impacts, impacts.body(i));
    for (int face = 0; face < 6; ++face)
        packBank(out, settings::Collisions::Storage::MAX_BODIES + face, impacts, impacts.wall(face));
}

} // namespace render
