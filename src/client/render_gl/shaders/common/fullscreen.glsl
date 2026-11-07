/**
 * @file
 * Full-screen triangle generator shared by the sky and background passes.
 *
 * Multiple shaders emit the same oversized NDC triangle at vertices 0, 1, and 2.
 * Lifting it here keeps the passes from drifting apart and centralises the
 * "big triangle" trick (3 vertices instead of 6, no diagonal seam) in one place.
 *
 * @see Sascha Willems, "Rendering a fullscreen quad without buffers"
 *      https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
 */

#pragma once


/**
 * Produce the NDC position of one corner of the full-screen triangle.
 *
 * Vertex 0 is the near corner, 1 and 2 are the two far corners placed at
 * x = +3 and y = +3 so the rasteriser clips them back to the viewport edge.
 * Coordinates outside [-1, 1] are legal: the clip stage trims them, and the
 * interior of the triangle still covers the whole framebuffer.
 *
 * @param vertexId The vertex index, which is 0, 1 or 2 for a three-vertex draw.
 * @return The NDC position of that corner.
 */
vec2 fullScreenTriangleNdc(int vertexId) {
    return vec2(vertexId == 1 ? 3.0 : -1.0, vertexId == 2 ? 3.0 : -1.0);
}


/**
 * Produce texture coordinates in [0, 2] that reach from the near corner to the
 * two far corners.
 *
 * Multiplying the NDC triangle by 0.5 and adding 0.5 maps the same three corners
 * onto the unit square with the seam pushed outside, which is what a full-screen
 * texture sample wants.
 *
 * @param vertexId The vertex index, which is 0, 1 or 2 for a three-vertex draw.
 * @return The unclamped [0, 2] texture coordinate of that corner.
 */
vec2 fullScreenTriangleUv(int vertexId) {
    return fullScreenTriangleNdc(vertexId) * 0.5 + 0.5;
}
