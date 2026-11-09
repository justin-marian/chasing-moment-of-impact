/**
 * @file
 * Generate unit shape meshes and manage OpenGL instance buffers.
 *
 * Provides procedural geometry generation for basic primitives (quads, cubes,
 * spheres, cylinders, and cones) and wraps their VBO/VAO state management. Also
 * handles skinned mesh creation when the MMO build is enabled.
 */

#include "settings/collisions/constants.h"
#include "settings/world/constants.h"
#include "settings/world/render_constants.h"
#include "client/render_gl/meshes/unit_shapes.h"

#include <algorithm>
#include <array>
#include <cmath>


namespace render {

namespace {

/**
 * Push one triangle's three vertices onto the output stream.
 *
 * @param out Destination vertex buffer.
 * @param a   First vertex.
 * @param b   Second vertex.
 * @param c   Third vertex.
 */
void tri(std::vector<Vertex>& out, const Vertex& a, const Vertex& b, const Vertex& c) {
    out.push_back(a);
    out.push_back(b);
    out.push_back(c);
}

} // namespace (anonymous)


/**
 * Generate a standard unit quad centered at the origin facing the +Z axis.
 *
 * A quad is universally constructed as two adjacent triangles sharing a diagonal.
 *
 * @see Unity Manual: Create a quad mesh via script
 *      https://docs.unity3d.com/7000.0/Documentation/Manual/Example-CreatingaBillboardPlane.html
 *
 * @return Vertex array for two triangles.
 */
std::vector<Vertex> makeQuad() {
    // Two triangles making up the quad.
    // Winding order is Counter-Clockwise (CCW) so the face points outward along +Z.
    return {
        {{-1, -1, 0}, {0, 0, 1}, {0, 0}}, // Bottom-left
        {{1, -1, 0}, {0, 0, 1}, {1, 0}},  // Bottom-right
        {{1, 1, 0}, {0, 0, 1}, {1, 1}},   // Top-right

        {{-1, -1, 0}, {0, 0, 1}, {0, 0}}, // Bottom-left
        {{1, 1, 0}, {0, 0, 1}, {1, 1}},   // Top-right
        {{-1, 1, 0}, {0, 0, 1}, {0, 1}}   // Top-left
    };
}


/**
 * Generate a unit cube.
 *
 * If rounded is true, each face is treated as a grid. Every vertex is snapped
 * toward the cube core and pushed back out along the core-to-vertex direction.
 * This produces a rounded silhouette without changing the unit extents required
 * by collision detection. This technique is commonly referred to as vertex
 * snapping or normal-based extrusion on a subdivided block.
 *
 * @see Procedural Rounded Cube Generation
 *      https://www.coursera.org/learn/unity-design-deform-meshes-3d-geometry-control
 *
 * @param rounded Whether to generate a subdivided, rounded cube.
 * @return Vertex array for the cube.
 */
std::vector<Vertex> makeCube(bool rounded) {
    constexpr float CORE_EXTENT = .97f;
    constexpr float ROUNDING_OFFSET = .03f;
    constexpr float UV_SCALE = 1.02f;

    std::vector<Vertex> out;
    const int segments = rounded ? settings::World::Rendering::Meshes::CUBE_SEGMENTS : 1;

    // Iterate over the 3 axes (X, Y, Z) to generate all 6 faces (+/- for each).
    for (int axis = 0; axis < 3; ++axis) {
        for (int sign : {-1, 1}) {
            // Subdivide each face into a grid of segments x segments.
            for (int i = 0; i < segments; ++i) {
                for (int j = 0; j < segments; ++j) {

                    auto vertex = [&](int a, int b) {
                        glm::vec3 p(0);
                        p[axis] = float(sign);
                        p[(axis + 1) % 3] = -1 + 2 * float(a) / segments;
                        p[(axis + 2) % 3] = -1 + 2 * float(b) / segments;

                        if (!rounded) {
                            glm::vec3 normal(0);
                            normal[axis] = float(sign);
                            return Vertex{p, normal, {float(a), float(b)}};
                        }

                        // Snap to the inner core, calculate the outward normal, then extrude.
                        glm::vec3 core = glm::clamp(p, glm::vec3(-CORE_EXTENT), glm::vec3(CORE_EXTENT));
                        glm::vec3 normal = glm::normalize(p - core);

                        return Vertex{
                            core + ROUNDING_OFFSET * normal,
                            normal,
                            {
                                float(a) / settings::World::Rendering::Meshes::CUBE_SEGMENTS * UV_SCALE,
                                float(b) / settings::World::Rendering::Meshes::CUBE_SEGMENTS * UV_SCALE
                            }
                        };
                    };

                    // Fetch the 4 corners of the current grid cell.
                    auto a = vertex(i, j);
                    auto b = vertex(i + 1, j);
                    auto c = vertex(i + 1, j + 1);
                    auto d = vertex(i, j + 1);

                    // Emit 2 triangles per grid cell (CCW winding to face outward).
                    tri(out, a, b, c);
                    tri(out, a, c, d);
                }
            }
        }
    }

    return out;
}


/**
 * Generate a unit sphere using a subdivided icosahedron (Icosphere).
 *
 * Three subdivision levels bring the sphere close enough to smooth for a
 * shading reference; more would only burn vertex budget without changing the
 * silhouette at typical camera distances. Each subdivision level splits every
 * triangle into four smaller triangles and projects the new vertices to the
 * unit sphere.
 *
 * @see Geodesic Icosahedron (Recursive Subdivision)
 *      https://pypi.org/project/icosphere/
 *
 * @see Creating an icosphere mesh in code (Andreas Kahler)
 *      http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
 *
 * @return Vertex array for the sphere.
 */
std::vector<Vertex> makeSphere() {
    constexpr int SUBDIVISION_LEVELS = 3;

    // Golden ratio used to determine the 12 vertices of the base icosahedron.
    float t = (1 + std::sqrt(5.0f)) / 2;
    std::array<glm::vec3, 12> points = {
        glm::vec3(-1, t, 0), {1, t, 0}, {-1, -t, 0}, {1, -t, 0},
        {0, -1, t},          {0, 1, t}, {0, -1, -t}, {0, 1, -t},
        {t, 0, -1},          {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}
    };

    std::vector<std::array<glm::vec3, 3>> triangles;
    for (auto& face : settings::World::Rendering::Meshes::FACES) {
        triangles.push_back({
            glm::normalize(points[face[0]]),
            glm::normalize(points[face[1]]),
            glm::normalize(points[face[2]])
        });
    }

    // Subdivide each triangle into 4 smaller triangles and project to the sphere surface.
    for (int level = 0; level < SUBDIVISION_LEVELS; ++level) {
        std::vector<std::array<glm::vec3, 3>> next;

        for (auto& f : triangles) {
            auto a = glm::normalize(f[0] + f[1]);
            auto b = glm::normalize(f[1] + f[2]);
            auto c = glm::normalize(f[2] + f[0]);

            next.push_back({f[0], a, c});
            next.push_back({f[1], b, a});
            next.push_back({f[2], c, b});
            next.push_back({a, b, c});
        }

        triangles = std::move(next);
    }

    std::vector<Vertex> out;
    for (auto& f : triangles) {
        std::array<Vertex, 3> v;

        // Map Cartesian coordinates to spherical UVs (longitude/latitude).
        for (int i = 0; i < 3; ++i) {
            v[i] = {
                f[i], f[i],
                {
                    std::atan2(f[i].z, f[i].x) / (2 * float(settings::Collisions::Geometry::PI)) + .5f,
                    std::asin(glm::clamp(f[i].y, -1.0f, 1.0f)) / float(settings::Collisions::Geometry::PI) + .5f
                }
            };
        }

        // Sphere UV seam: triangles that straddle the atan2 wrap get their
        // low-x vertices pushed past 1.0 so the texture sample does not
        // streak across the seam.
        float low = std::min({v[0].uv.x, v[1].uv.x, v[2].uv.x});
        float high = std::max({v[0].uv.x, v[1].uv.x, v[2].uv.x});

        if (high - low > .5f) {
            for (auto& p : v) {
                if (p.uv.x < .5f) {
                    p.uv.x += 1;
                }
            }
        }

        tri(out, v[0], v[1], v[2]);
    }

    return out;
}


/**
 * Generate a cylinder or cone.
 *
 * Both share the ring construction: the ring profile differs only in whether
 * the top ring has radius 1 (cylinder) or 0 (cone apex). This ring-based
 * approach is standard across modern engines.
 *
 * @see Godot Engine: CylinderMesh
 *      https://docs.godotengine.org/en/4.0/classes/class_cylindermesh.html
 *
 * @see Babylon.js: Mesh Generators
 *      https://github.com/babylonjs/babylon-lite/blob/91f1be782f9446bebfc2d148edf0c4f691b04446/docs/lite/architecture/10-mesh-generators.md
 *
 * @param cone Whether to generate a cone instead of a cylinder.
 * @return Vertex array for the requested shape.
 */
std::vector<Vertex> makeRoundMesh(bool cone) {
    std::vector<Vertex> out;

    // Define the cross-sectional profile from bottom to top.
    std::vector<glm::vec2> rings = cone
        ? std::vector<glm::vec2>{{.97f, -1}, {1, -.97f}, {0, 1}}
        : std::vector<glm::vec2>{{.97f, -1}, {1, -.97f}, {1, .97f}, {.97f, 1}};

    // Loop around the circumference.
    for (int i = 0; i < settings::World::Rendering::Meshes::ROUND_SEGMENTS; ++i) {
        float a = 2 * float(settings::Collisions::Geometry::PI) * float(i) / settings::World::Rendering::Meshes::ROUND_SEGMENTS;
        float b = 2 * float(settings::Collisions::Geometry::PI) * float(i + 1) /  settings::World::Rendering::Meshes::ROUND_SEGMENTS;

        // Stitch the current ring to the next ring moving up the profile.
        for (std::size_t r = 0; r + 1 < rings.size(); ++r) {

            auto vertex = [&](float angle, std::size_t k, float u) {
                auto ring = rings[k];
                glm::vec3 n(std::cos(angle), cone ? .5f : 0, std::sin(angle));

                // Adjust normals for the bottom and top edges to create a smooth rounded bevel.
                if (k == 0) {
                    n = glm::normalize(n + glm::vec3(0, -1, 0));
                } else if (!cone && k + 1 == rings.size()) {
                    n = glm::normalize(n + glm::vec3(0, 1, 0));
                }

                return Vertex{
                    {ring.x * std::cos(angle), ring.y, ring.x * std::sin(angle)},
                    glm::normalize(n),
                    {u, (ring.y + 1) / 2}
                };
            };

            // Fetch the 4 corners of the current quad face on the wall.
            auto p = vertex(a, r, float(i) / settings::World::Rendering::Meshes::ROUND_SEGMENTS);
            auto q = vertex(b, r, float(i + 1) / settings::World::Rendering::Meshes::ROUND_SEGMENTS);
            auto v = vertex(b, r + 1, float(i + 1) / settings::World::Rendering::Meshes::ROUND_SEGMENTS);
            auto w = vertex(a, r + 1, float(i) / settings::World::Rendering::Meshes::ROUND_SEGMENTS);

            tri(out, p, q, w);
            if (rings[r + 1].x > 0) {
                tri(out, q, v, w);
            }
        }

        // Bottom cap. The cap fan uses a UV centred at (0.5, 0.5) so the
        // cap reads as a disc rather than picking up the wall UV.
        Vertex loA{
            {.97f * std::cos(a), -1, .97f * std::sin(a)},
            {0, -1, 0},
            {.5f + .5f * std::cos(a), .5f + .5f * std::sin(a)}
        };
        Vertex loB{
            {.97f * std::cos(b), -1, .97f * std::sin(b)},
            {0, -1, 0},
            {.5f + .5f * std::cos(b), .5f + .5f * std::sin(b)}
        };

        tri(out, {{0, -1, 0}, {0, -1, 0}, {.5f, .5f}}, loB, loA);

        // Top cap: only for the cylinder; the cone has an apex instead,
        // produced by the last ring pair above.
        if (!cone) {
            loA.p.y = loB.p.y = 1;
            loA.n = loB.n = {0, 1, 0};
            tri(out, {{0, 1, 0}, {0, 1, 0}, {.5f, .5f}}, loA, loB);
        }
    }

    return out;
}


/**
 * Initialize the vertex buffer and instance buffer for an instanced mesh.
 *
 * @param vertices Source vertex data.
 */
InstancedMesh::InstancedMesh(const std::vector<Vertex>& vertices) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vertex);
    glGenBuffers(1, &instances);
    count = static_cast<GLsizei>(vertices.size());

    glBindVertexArray(vao);

    // --- Static Mesh Data ---
    glBindBuffer(GL_ARRAY_BUFFER, vertex);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    // Map the Vertex struct fields (position, normal, UVs) to shader attribute locations 0-3.
    for (GLuint i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(i);
        std::size_t offset =
            i == 0 ? offsetof(Vertex, p) :
            i == 1 ? offsetof(Vertex, n) :
            i == 2 ? offsetof(Vertex, uv) : offsetof(Vertex, uv1);

        // Position and Normal are vec3 (3 floats); UVs are vec2 (2 floats).
        glVertexAttribPointer(i, i < 2 ? 3 : 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offset));
    }

    // --- Instanced Data ---
    glBindBuffer(GL_ARRAY_BUFFER, instances);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Instance) * settings::Collisions::Storage::MAX_BODIES, nullptr, GL_STREAM_DRAW);

    // Map the world matrix (mat4) to attribute locations 4-7.
    for (GLuint i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(4 + i);
        glVertexAttribPointer(
            4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
            reinterpret_cast<void*>(std::size_t(i) * sizeof(glm::vec4))
        );
        // Advance this attribute once per instance, not once per vertex.
        glVertexAttribDivisor(4 + i, 1);
    }

    // Map the color and visual properties to attribute locations 8-9.
    for (GLuint i = 8; i < 10; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(
            i, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
            reinterpret_cast<void*>(i == 8 ? offsetof(Instance, color) : offsetof(Instance, visual))
        );
        glVertexAttribDivisor(i, 1);
    }
}


/**
 * Release the vertex, instance, and array buffers.
 */
InstancedMesh::~InstancedMesh() {
    glDeleteBuffers(1, &instances);
    glDeleteBuffers(1, &vertex);
    glDeleteVertexArrays(1, &vao);
}


/**
 * Upload instance data and draw the mesh.
 *
 * @param data Instance data to render.
 */
void InstancedMesh::draw(const std::vector<Instance>& data) {
    if (data.empty()) {
        return;
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instances);

    // Overwrite the previous frame's instance data without reallocating the buffer.
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(data.size() * sizeof(Instance)), data.data());
    // Draw 'count' vertices, repeating the operation 'data.size()' times (once per instance).
    glDrawArraysInstanced(GL_TRIANGLES, 0, count, static_cast<GLsizei>(data.size()));
}


#ifdef SIM_BUILD_MMO
namespace detail {

/**
 * Initialize the skinned mesh from the given animation primitive.
 *
 * @param primitive Source geometry and material data.
 */
SkinnedMesh::SkinnedMesh(const animation::Primitive& primitive) {
    initialize(primitive);
}


/**
 * Release the vertex, element, and array buffers.
 */
SkinnedMesh::~SkinnedMesh() {
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}


/**
 * Upload the primitive's vertex and index data to the GPU.
 *
 * @param primitive Source geometry and material data.
 */
void SkinnedMesh::initialize(const animation::Primitive& primitive) {
    color = primitive.color;
    texture = primitive.texture;
    normalTexture = primitive.normalTexture;
    alphaCutoff = primitive.alphaCutoff;
    count = static_cast<GLsizei>(primitive.indices.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(primitive.vertices.size() * sizeof(animation::Vertex)),
        primitive.vertices.data(), GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(primitive.indices.size() * sizeof(std::uint32_t)),
        primitive.indices.data(), GL_STATIC_DRAW
    );

    initializeAttributes();
}


/**
 * Bind the standard skinned vertex attribute layout.
 */
void SkinnedMesh::initializeAttributes() {
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(animation::Vertex),
        reinterpret_cast<void*>(offsetof(animation::Vertex, position))
    );

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(animation::Vertex),
        reinterpret_cast<void*>(offsetof(animation::Vertex, normal))
    );

    // Joint Indices (Integer attribute)
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(
        2, 4, GL_UNSIGNED_INT, sizeof(animation::Vertex),
        reinterpret_cast<void*>(offsetof(animation::Vertex, joints))
    );

    // Joint Weights
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 4, GL_FLOAT, GL_FALSE, sizeof(animation::Vertex),
        reinterpret_cast<void*>(offsetof(animation::Vertex, weights))
    );

    // UV Coordinates
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4, 2, GL_FLOAT, GL_FALSE, sizeof(animation::Vertex),
        reinterpret_cast<void*>(offsetof(animation::Vertex, uv))
    );
}


/**
 * Draw the skinned mesh using its indexed triangles.
 */
void SkinnedMesh::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}


/**
 * Generate a sharp unit cube adapted to the animation vertex format.
 *
 * @return Animation primitive containing the cube.
 */
animation::Primitive cube() {
    animation::Primitive result;

    for (const auto& vertex : render::makeCube(false)) {
        animation::Vertex converted;
        converted.position = vertex.p;
        converted.normal = vertex.n;
        converted.uv = vertex.uv;

        result.indices.push_back(static_cast<std::uint32_t>(result.vertices.size()));
        result.vertices.push_back(converted);
    }

    return result;
}

} // namespace detail
#endif

} // namespace render
