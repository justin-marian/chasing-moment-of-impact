/**
 * @file
 * Transform rigid-body instances from local space to world space.
 *
 * Transforms vertex attributes using the per-instance world matrix, handles
 * normal correction for non-uniform scaling, and extracts per-axis scaling
 * for local impact patch mapping.
 */

#version 460 core

#include "settings/world.glsl"


/* --- Vertex Attributes --- */
layout(location = 0) in vec3 aPosition_m;        // Local space position in metres.
layout(location = 1) in vec3 aNormal;            // Local space normal.
layout(location = 2) in vec2 aUv;                // Primary UV coordinates.
layout(location = 3) in vec2 aUv1;               // Secondary UVs (ambient occlusion is packed in .x).
layout(location = 4) in mat4 aInstanceWorld_m;   // Instance transform (position + full quaternion + scale).
layout(location = 8) in vec4 aInstanceColor;     // Per-instance base color.
layout(location = 9) in vec4 aInstanceVisual;    // Per-instance state (visual.w holds the impact index).


/* --- Varyings to Fragment Shader --- */
out vec3 worldNormal;          // World-space normal (corrected for scale).
out vec3 worldPosition_m;      // World-space position (for view vector).

out vec2 uv;                   // Forwarded UV coordinates.
out float ao;                  // Ambient occlusion extracted from aUv1.x.

out vec4 color;                // Base color.
out vec4 visual;               // Visual state flags (e.g., local contact flash).

out vec3 impactLocalPoint_m;   // Local hit point (scaled) for impact heat calculation.
out vec3 impactLocalNormal;    // Local hit normal (scaled) for impact heat calculation.

flat out int impactIndex;      // Instance index for SSBO lookup (flat because it's an integer).


/* --- Uniforms --- */
layout(location = 0) uniform mat4 projectionView; // Combined camera view-projection matrix.


/**
 * Extract per-axis scale from an affine transform.
 *
 * Used to move between instance and object-local metres so impact patches
 * can be authored in the object frame regardless of instance scaling.
 *
 * @param m The instance world matrix.
 * @return A vec3 containing the length (scale) of the X, Y, and Z basis vectors.
 */
vec3 instanceScale(mat4 m) {
	return vec3(length(m[0].xyz), length(m[1].xyz), length(m[2].xyz));
}


void main() {
	// Transform local position to world space.
	vec4 p = aInstanceWorld_m * vec4(aPosition_m, 1);
	worldPosition_m = p.xyz;

	// Correct normal using the inverse-transpose to handle non-uniform scaling.
	worldNormal = transpose(inverse(mat3(aInstanceWorld_m))) * aNormal;

	// Forward vertex attributes.
	uv = aUv;
	ao = aUv1.x;
	color = aInstanceColor;
	visual = aInstanceVisual;

	// Extract scale to normalize the local impact coordinates.
	vec3 scale = instanceScale(aInstanceWorld_m);

	// Position is scaled up to match world space; normal is scaled inversely.
	impactLocalPoint_m = aPosition_m * scale;
	impactLocalNormal = aNormal / scale;

	// Extract the impact index from the visual payload (rounded to nearest int).
	impactIndex = int(aInstanceVisual.w + 0.5);

	// Final projection to clip space.
	gl_Position = projectionView * p;
}
