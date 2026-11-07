/**
 * @file
 * Generate reusable unit-shape triangle streams for both renderer implementations.
 *
 * Provides declarations for procedural geometry generation (quads, cubes,
 * spheres, cylinders, and cones) and the corresponding OpenGL buffer wrappers.
 */

#pragma once

#include "settings/collisions/constants.h"

#include <filesystem>
#include <memory>
#include <vector>

#include <glad/glad.h>

#ifdef SIM_BUILD_MMO
#include "client/animation/model.h"
#endif

#include <glm/glm.hpp>


namespace render {

/**
 * Geometry vertex shared by the rigid-body mesh and world-box adapter.
 */
struct Vertex {
    glm::vec3 p, n;
    glm::vec2 uv, uv1{1, 0};
};


/**
 * Unit XY quad with a +Z normal; used for the floor and the wall faces.
 *
 * @return Triangle vertex stream for an XY unit quad with a +Z normal.
 */
std::vector<Vertex> makeQuad();


/**
 * Rounded unit cube.
 *
 * 1.5% of the edge is beveled without altering the physics extents.
 * Set rounded to false for the flat box used by world collision proxies.
 *
 * @param rounded True for beveled visual body geometry; false for the flat world box.
 * @return Triangle vertex stream for the requested rounded or flat unit cube.
 */
std::vector<Vertex> makeCube(bool rounded = true);
/**
 * Unit sphere, generated as an icosahedron subdivided three times then normalized.
 *
 * @return Subdivided unit-sphere triangle vertex stream with seam-adjusted UVs.
 */
std::vector<Vertex> makeSphere();
/**
 * Unit cylinder or unit cone with rounded edges.
 *
 * @param cone True for a cone profile; false for a cylinder with two flat caps.
 * @return Unit cylinder/cone triangle vertex stream with its cap/profile normals.
 */
std::vector<Vertex> makeRoundMesh(bool cone);


/**
 * Per-instance data uploaded through the instanced vertex buffer.
 */
struct Instance {
    glm::mat4 world;
    glm::vec4 color, visual{1, 0, 1, 0};
};
/**
 * Debug-line vertex with position and display color.
 */
struct LineVertex {
    glm::vec3 p;
    glm::vec4 color;
};
/**
 * One retained impact patch as uploaded to the SSBO.
 */
struct ImpactGpu {
    glm::vec4 pointRadius{}, normalStrength{};
};


/**
 * Instanced drawable mesh: one VAO with a static vertex buffer and a
 * stream-drawn instance buffer.
 *
 * The instance buffer is sized once for the maximum configured body count, so
 * drawing requires only a sub-update without any runtime allocation.
 * Non-copyable because it owns OpenGL objects.
 */
class InstancedMesh {
public:
    GLuint vao{}, vertex{}, instances{};
    GLsizei count{};

    /**
     * Upload validated mesh geometry into exclusively owned OpenGL buffers.
     *
     * @param vertices Triangle vertex stream uploaded once into the mesh's static geometry buffer.
     */
    explicit InstancedMesh(const std::vector<Vertex>& vertices);


    /**
     * Release this owner's resources once; borrowed state is not destroyed.
     * Disallow copying because duplicating the object would duplicate OpenGL resource ownership.
     * Disallow copy assignment because this type has exclusive resource ownership.
     */
    ~InstancedMesh();
    InstancedMesh(const InstancedMesh&) = delete;
    InstancedMesh& operator=(const InstancedMesh&) = delete;


    /**
     * Upload new instance data and draw the batch.
     *
     * A no-op for an empty batch so a pass does not need to check whether it
     * produced any instances this frame.
     *
     * @param data Per-instance world, color and visual data.
     */
    void draw(const std::vector<Instance>& data);
};


#ifdef SIM_BUILD_MMO
namespace detail {

/**
 * Exclusive OpenGL ownership for one uploaded primitive and its material texture references.
 */
struct SkinnedMesh {
    GLuint vao{}, vbo{}, ebo{};
    GLsizei count{};
    glm::vec3 color{};
    std::filesystem::path texture, normalTexture;
    float alphaCutoff{};

    /**
     * Upload validated mesh geometry into exclusively owned OpenGL buffers.
     *
     * @param primitive Validated supplied triangle mesh, including material and optional skin attributes.
     */
    explicit SkinnedMesh(const animation::Primitive& primitive);


    /**
     * Release this owner's resources once; borrowed state is not destroyed.
     * Disallow copying because duplicating the object would duplicate OpenGL resource ownership.
     * Disallow copy assignment because this type has exclusive resource ownership.
     */
    ~SkinnedMesh();
    SkinnedMesh(const SkinnedMesh&) = delete;
    SkinnedMesh& operator=(const SkinnedMesh&) = delete;


    /**
     * Upload one supplied triangle primitive and bind its vertex/index attributes.
     *
     * @param primitive Validated supplied triangle mesh, including material and optional skin attributes.
     */
    void initialize(const animation::Primitive& primitive);


    /**
     * Describe the supplied vertex layout to the currently bound OpenGL vertex array.
     */
    static void initializeAttributes();


    /**
     * Draw this uploaded primitive using the currently active scene program.
     */
    void draw() const;
};


/**
 * Adapt the shared flat cube geometry into a world-renderer triangle primitive.
 *
 * @return Flat unit-box triangle primitive ready for the world mesh uploader.
 */
animation::Primitive cube();

} // namespace detail
#endif

} // namespace render
