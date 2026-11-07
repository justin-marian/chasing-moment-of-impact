/**
 * @file
 * Own OpenGL meshes, programs and render targets used by the glass renderer.
 */
#pragma once

#include "client/render_gl/impact_history.h"
#include "client/render_gl/rigid_bodies/session_renderer.h"
#include "settings/collisions/constants.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

/**
 * Small utilities shared by every pass: asset lookup, shader compilation
 * and linking, per-name uniform setters, sRGB-to-linear colour conversion,
 * spherical light direction, and the ground plane transform.
 *
 * Kept free functions rather than a class because passes only ever read
 * from these; there is no state to hold.
 */

namespace render {

/**
 * Resolve a project-relative asset path.
 * Uses shared working-directory, executable-relative and development fallback discovery.
 *
 * @param relative Project-relative path.
 * @return Absolute path to the first candidate that exists.
 * @throws std::runtime_error If none of the candidates contains the file.
 */
std::filesystem::path asset(std::string_view relative);

/**
 * Compile one GLSL shader file and return its GL object.
 *
 * @param stage GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
 * @param file  Shader filename, resolved under shaders/.
 * @return The compiled shader object.
 * @throws std::runtime_error With the shader info log on failure.
 */
GLuint loadShader(GLenum stage, const std::string& file);

/**
 * Link a vertex and fragment shader into a program.
 * Deletes the intermediate shader objects on both success and failure.
 *
 * @param vertex   Vertex shader filename.
 * @param fragment Fragment shader filename.
 * @return The linked program.
 * @throws std::runtime_error With the program info log on failure.
 */
GLuint linkProgram(const std::string& vertex, const std::string& fragment);

/**
 * Set a float uniform by name.
 * Silently no-ops when the name is not active on the program, which is the
 * common case for uniforms optimised out by the GLSL compiler.
 *
 * @param p Linked OpenGL shader program receiving the uniform.
 * @param name Exact GLSL uniform name; inactive uniforms follow OpenGL's no-op convention.
 * @param value Scalar or vector value uploaded to the named shader uniform.
 */
void uniform(GLuint p, const char* name, float value);

/**
 * Set a vec3 uniform by name; same silent-absence policy as the float overload.
 *
 * @param p Linked OpenGL shader program receiving the uniform.
 * @param name Exact GLSL uniform name; inactive uniforms follow OpenGL's no-op convention.
 * @param value Scalar or vector value uploaded to the named shader uniform.
 */
void uniform(GLuint p, const char* name, glm::vec3 value);

/**
 * sRGB hex (0xRRGGBB) to linear-light vec3.
 *
 * @param hex Packed non-linear RGB color in 0xRRGGBB format.
 * @return RGB components decoded into linear light.
 */
glm::vec3 linear(unsigned hex);

/**
 * Spherical-to-cartesian unit vector from azimuth/elevation in degrees.
 *
 * @param azimuth Light angle around the vertical axis, in degrees.
 * @param elevation Light elevation above the horizon, in degrees.
 * @return Unit world-space light direction.
 */
glm::vec3 lightDirection(float azimuth, float elevation);

/**
 * Ground-plane transform: an XY unit quad laid flat on the XZ plane with the
 * given footprint and offset. Shared by the floor and shadow passes so the
 * +0.003 lift for z-fighting is defined once.
 *
 * @param p World-space position of the ground plane, in metres.
 * @param extent Ground-plane scale/footprint expressed in world-space units.
 * @return Model-to-world transform for the flat ground/shadow quad.
 */
glm::mat4 groundPlane(glm::vec3 p, glm::vec3 extent);

/**
 * Create the GLFW/OpenGL context and record the driver used for rendering.
 *
 * @param state Renderer-owned window, GPU resources and cached pass state.
 * @param visible Whether the GLFW window is shown interactively or hidden for a capture.
 * @param width Drawable target width in pixels.
 * @param height Drawable target height in pixels.
 */
void initializeContext(Renderer::Impl& state, bool visible, int width, int height);
/**
 * Compile and link the shader programs needed by the render passes.
 *
 * @param state Renderer-owned window, GPU resources and cached pass state.
 */
void initializePrograms(Renderer::Impl& state);
/**
 * Create shared body meshes, debug geometry and GPU timer objects.
 *
 * @param state Renderer-owned window, GPU resources and cached pass state.
 */
void initializeGeometry(Renderer::Impl& state);
/**
 * Create the ImGui context, load supplied fonts and initialize the upstream backends.
 *
 * @param state Renderer-owned window, GPU resources and cached pass state.
 */
void initializeInterface(Renderer::Impl& state);

struct ImpactGpu;

/**
 * Pack every body and wall history bank into the flat GPU buffer.
 * Layout: slots [0, settings::Collisions::Storage::MAX_BODIES) hold body patches, slots
 * [settings::Collisions::Storage::MAX_BODIES, settings::Collisions::Storage::MAX_BODIES + 6) hold wall patches, each of settings::Collisions::Impacts::PATCHES_PER_SURFACE
 * entries. Called every frame before draw(); the caller uploads the buffer.
 *
 * @param out        Buffer to write into; must already be sized for the
 *                   full (settings::Collisions::Storage::MAX_BODIES + 6) * settings::Collisions::Impacts::PATCHES_PER_SURFACE entries.
 * @param impacts    Retained history to read.
 * @param bodyCount  Active bodies; wall banks are always written in full.
 */
void packImpactPatches(std::vector<ImpactGpu>& out, const ImpactHistory& impacts, int bodyCount);

} // namespace render
