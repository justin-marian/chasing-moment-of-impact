/**
 * @file
 * Build and draw instanced rigid bodies, dynamic contact flashes, and glass walls.
 *
 * Handles per-instance transformation matrix construction, three-point lighting
 * configuration, and back-to-front sorting for translucent wall geometry.
 */

#include "client/render_gl/gpu_resources.h"
#include "client/render_gl/rigid_bodies/draw.h"
#include "client/render_gl/rigid_bodies/state.h"
#include "settings/collisions/constants.h"
#include "settings/world/constants.h"
#include "settings/world/render_constants.h"

#include <algorithm>
#include <cstddef>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace render::passes {

namespace {

/**
 * Generate a stable color for a body based on its integer ID.
 *
 * @param id The unique body identifier.
 * @return The generated color vector with full opacity.
 */
glm::vec4 bodyColorForId(int id) {
    constexpr float HUE_DEGREES = 360.0f;

    const float hue = float((unsigned(id) * settings::World::Rendering::RigidBodies::Appearance::BODY_HUE_STEP_DEG) % unsigned(HUE_DEGREES)) / HUE_DEGREES;

    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(
        hue, 
        settings::World::Rendering::RigidBodies::Appearance::BODY_SATURATION,
        settings::World::Rendering::RigidBodies::Appearance::BODY_VALUE, 
        r, g, b
    );
    
    return {r, g, b, 1.0f};
}


/**
 * Configure the three-point scene lighting uniforms for the given shader program.
 *
 * Direction arguments are specified as azimuth and elevation in degrees.
 * Colors are provided in sRGB and converted to linear space once during setup.
 *
 * @param program The OpenGL shader program to update.
 */
void applySceneLighting(GLuint program) {
    constexpr int KEY_AZIMUTH = 35;
    constexpr int KEY_ELEVATION = 45;
    constexpr int FILL_AZIMUTH = -120;
    constexpr int FILL_ELEVATION = 15;
    constexpr int RIM_AZIMUTH = 200;
    constexpr int RIM_ELEVATION = 5;

    uniform(program, "uKeyDir", lightDirection(KEY_AZIMUTH, KEY_ELEVATION));
    uniform(program, "uFillDir", lightDirection(FILL_AZIMUTH, FILL_ELEVATION));
    uniform(program, "uRimDir", lightDirection(RIM_AZIMUTH, RIM_ELEVATION));

    uniform(program, "uKeyColor", linear(settings::World::Rendering::RigidBodies::Lighting::KEY_COLOR_SRGB));
    uniform(program, "uFillColor", linear(settings::World::Rendering::RigidBodies::Lighting::FILL_COLOR_SRGB));
    uniform(program, "uRimColor", linear(settings::World::Rendering::RigidBodies::Lighting::RIM_COLOR_SRGB));
    uniform(program, "uAmbientSky", linear(settings::World::Rendering::RigidBodies::Lighting::AMBIENT_SKY_SRGB));
    uniform(program, "uAmbientGround", linear(settings::World::Rendering::RigidBodies::Lighting::AMBIENT_GROUND_SRGB));

    uniform(program, "uKeyIntensity", settings::World::Rendering::RigidBodies::Lighting::KEY_INTENSITY);
    uniform(program, "uFillIntensity", settings::World::Rendering::RigidBodies::Lighting::FILL_INTENSITY);
    uniform(program, "uRimIntensity", settings::World::Rendering::RigidBodies::Lighting::RIM_INTENSITY);
    uniform(program, "uAmbientIntensity", settings::World::Rendering::RigidBodies::Lighting::AMBIENT_INTENSITY);
}

} // namespace (anonymous)


/**
 * Build per-instance data for all rigid bodies in the current frame.
 *
 * Rebuilds every shape bucket from scratch to prevent stale instances.
 * Calculates the world transformation matrix and derives a temporary
 * contact flash effect based purely on simulation tick time.
 *
 * @param impl Renderer state containing instance buckets and tracking arrays.
 * @param s    Current simulation state providing body positions and flags.
 * @param c    World configuration providing the fixed time step.
 */
void buildBodies(Renderer::Impl& impl, const sim::State& s, const sim::WorldConfig& c) {
    constexpr float SLEEPING_BRIGHTNESS = 1.1f;
    constexpr float AWAKE_BRIGHTNESS = 1.0f;

    // Every bucket is rebuilt from scratch so a body that disappeared this
    // tick doesn't leave a stale instance behind.
    for (auto& bucket : impl.groups) {
        bucket.clear();
    }

    // Tick time is the only clock this effect uses; using wall-clock time
    // would make the flash non-deterministic under capture or replay.
    const double tickSeconds = double(s.tick_i) * c.dt_s;

    for (int i = 0; i < s.count_i; ++i) {
        const auto position_m = s.position_m[i];
        const auto halfExtents_m = s.halfExtents_m[i];
        const auto orientation = s.orientation[i];

        // Column vectors apply scale first, then rotation, then translation into world space.
        const auto model =
            glm::translate(glm::mat4(1.0f), glm::vec3(position_m.x, position_m.y, position_m.z)) *
            glm::mat4_cast(glm::quat(orientation.w, orientation.x, orientation.y, orientation.z)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(halfExtents_m.x, halfExtents_m.y, halfExtents_m.z));

        // A rising edge of the contact flag starts a fixed-duration flash;
        // the falling edge just lets it decay. The whole effect is a
        // presentation cue: its only input is the per-tick contact flag,
        // which is derived rather than stored, so it never feeds back into
        // physics state and doesn't survive save/load.
        if (s.hasContact[i] && !impl.touching[i]) {
            impl.flashUntil[i] = tickSeconds + settings::World::Rendering::RigidBodies::Appearance::CONTACT_FLASH_S;
        }

        impl.touching[i] = s.hasContact[i] != 0;

        // Squared decay so the falloff reads as an ease-out, not a linear ramp.
        float flash = float(glm::clamp((impl.flashUntil[i] - tickSeconds) / settings::World::Rendering::RigidBodies::Appearance::CONTACT_FLASH_S, 0.0, 1.0));
        flash *= flash;

        // Per-instance payload: brightness scale, contact-flash amount,
        // spare channel, and the stable body id used for shader lookups.
        const float brightness = s.isSleeping[i] ? SLEEPING_BRIGHTNESS : AWAKE_BRIGHTNESS;
        const float flashValue = impl.impacts.enabled ? 0.0f : flash;

        impl.groups[s.shape_i[i]].push_back({
            model,
            bodyColorForId(i),
            {brightness, flashValue, 1.0f, float(i)},
        });
    }
}


/**
 * Draw the opaque rigid body instances to the screen.
 *
 * Enables depth writing and disables blending to ensure bodies render
 * correctly before translucent elements like walls are drawn.
 *
 * @param impl   Renderer state containing the body shader program and meshes.
 * @param camera Camera view-projection matrix.
 * @param eye    Camera position in world space.
 */
void drawBodies(Renderer::Impl& impl, const glm::mat4& camera, const glm::vec3& eye) {
    // Opaque bodies write depth before the translucent enclosure is blended over them.
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUseProgram(impl.body);

    // Location 0 is declared explicitly in body.vert; one matrix transforms every instance.
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(camera));
    uniform(impl.body, "uCameraPos_m", eye);

    applySceneLighting(impl.body);

    // The flash here is the static-scene baseline; per-body flash is carried
    // in the instance payload above.
    uniform(impl.body, "uContactFlash", 0);

    glUniform1i(glGetUniformLocation(impl.body, "uImpactEnabled"), impl.impacts.enabled ? 1 : 0);

    // Each shape has one flat-shaded instance bucket; no per-body texture selection is needed.
    for (std::size_t shape = 0; shape < settings::Collisions::Geometry::SHAPE_COUNT; ++shape) {
        impl.meshes[shape]->draw(impl.groups[shape]);
    }
}


/**
 * Build the transformation instances for the five visible static walls.
 *
 * Determines which wall the camera is closest to and skips it, ensuring
 * the enclosure always remains visible from the outside.
 *
 * @param impl        Renderer state containing the wall instance array.
 * @param eye         Camera position in world space.
 * @param halfExtents Arena half extents for sizing the walls.
 */
void buildWalls(Renderer::Impl& impl, const glm::vec3& eye, const glm::vec3& halfExtents) {
    constexpr int WALL_COUNT = 6;
    constexpr float QUARTER_TURN = float(settings::Collisions::Geometry::PI) / 2.0f;

    // Find the axis whose camera-to-extent ratio is largest; that axis
    // carries the face the camera cannot see.
    int nearAxis = 0;
    for (int k = 1; k < 3; ++k) {
        if (std::abs(eye[k] / halfExtents[k]) > std::abs(eye[nearAxis] / halfExtents[nearAxis])) {
            nearAxis = k;
        }
    }

    int nearFace = nearAxis * 2 + (eye[nearAxis] >= 0 ? 1 : 0);

    impl.walls.clear();
    
    for (int face = 0; face < WALL_COUNT; ++face) {
        if (face == nearFace) {
            continue;
        }

        int axis = face / 2;
        glm::vec3 p(0), extent(1);
        p[axis] = (face % 2 ? 1.0f : -1.0f) * halfExtents[axis];
        glm::mat4 rotation(1);

        if (axis == 0) {
            rotation = glm::rotate(rotation, QUARTER_TURN, glm::vec3(0, 1, 0));
            extent = {halfExtents.z, halfExtents.y, 1};
        } else if (axis == 1) {
            rotation = glm::rotate(rotation, QUARTER_TURN, glm::vec3(1, 0, 0));
            extent = {halfExtents.x, halfExtents.z, 1};
        } else {
            extent = {halfExtents.x, halfExtents.y, 1};
        }

        // The wall instance index in visual.w is settings::Collisions::Storage::MAX_BODIES + face so the
        // wall shader can find the right retained patch bank by index.
        impl.walls.push_back({
            glm::translate(glm::mat4(1), p) * rotation * glm::scale(glm::mat4(1), extent),
            {1, 1, 1, 1},
            {1, 0, 1, float(settings::Collisions::Storage::MAX_BODIES + face)}
        });
    }
}


/**
 * Draw the glass wall instances with correct translucency sorting.
 *
 * Sorts instances back-to-front relative to the camera. Sorting front-to-back
 * would drop far walls entirely under straight alpha blending.
 *
 * @param impl   Renderer state containing the wall shader program and mesh.
 * @param camera Camera view-projection matrix.
 * @param eye    Camera position in world space.
 */
void drawWalls(Renderer::Impl& impl, const glm::mat4& camera, const glm::vec3& eye) {
    // Back-to-front sort; front-to-back would drop far walls entirely under
    // straight alpha. The wall colour is alpha 0.06-0.10, so getting the order
    // right is what makes the glass read as layered rather than opaque.
    std::sort(impl.walls.begin(), impl.walls.end(), [&](const auto& a, const auto& b) {
        auto da = glm::vec3(a.world[3]) - eye;
        auto db = glm::vec3(b.world[3]) - eye;
        return glm::dot(da, da) > glm::dot(db, db);
    });

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glUseProgram(impl.wall);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(camera));
    uniform(impl.wall, "uCameraPos_m", eye);
    
    glUniform1i(glGetUniformLocation(impl.wall, "uImpactEnabled"), impl.impacts.enabled ? 1 : 0);
    
    impl.plane->draw(impl.walls);
}

} // namespace render::passes
