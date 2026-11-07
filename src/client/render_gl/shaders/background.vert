/**
 * @file
 * Generate clip-space geometry for the full-screen background.
 *
 * Uses the same oversized triangle as the sky pass, but forwards the [0, 2]
 * texture coordinate so the fragment stage can read the vertical gradient
 * without an additional remap.
 */

#version 460 core

#include "common/fullscreen.glsl"


out vec2 uv;


void main() {
	// Fetch the unclamped [0, 2] UVs mapped to the corners of the oversized triangle.
	uv = fullScreenTriangleUv(gl_VertexID);
	// Emit the normalized device coordinates (NDC) for the full-screen triangle.
	gl_Position = vec4(fullScreenTriangleNdc(gl_VertexID), 0.0, 1.0);
}
