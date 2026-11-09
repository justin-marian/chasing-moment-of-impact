/**
 * @file
 * Render the transparent contact-shadow contribution for a surface.
 *
 * Uses a Poisson disk sampling technique (Vogel spiral) to sample a soft
 * radial falloff, producing smooth and noise-free contact shadows.
 */

#version 460 core

#include "settings/world.glsl"


in vec3 worldPosition_m;
in vec3 worldNormal;

in vec2 uv;
in float ao;

in vec4 color;
in vec4 visual;

out vec4 fragmentColor;


void main() {
	// Map UV coordinates to the [-1, 1] range to center the shadow.
	vec2 p = uv * 2.0 - 1.0;
	float sum = 0.0;

	// Sample the contact shadow using a Vogel disk (golden angle spiral).
	for (int i = 0; i < WORLD.SHADOW.SHADOW_TAP_COUNT; ++i) {
		// Rotate each sample by the golden angle to ensure even distribution.
		float angle = float(i) * WORLD.SHADOW.GOLDEN_ANGLE;
		// Calculate the radial distance and scale the sample position within the footprint.
		vec2 tap =
            vec2(cos(angle), sin(angle)) *
            sqrt((float(i) + 0.5) / float(WORLD.SHADOW.SHADOW_TAP_COUNT)) *
            WORLD.SHADOW.SHADOW_FOOTPRINT_RADIUS;

		// Accumulate the exponential falloff based on the distance to the sample.
		sum += exp(-dot(p + tap, p + tap) * WORLD.SHADOW.SHADOW_FOOTPRINT_FALLOFF);
	}

	// Output the tint with the averaged shadow density as alpha.
	fragmentColor = vec4(
		WORLD.SHADOW.SHADOW_TINT,
		WORLD.SHADOW.SHADOW_FOOTPRINT_STRENGTH * visual.z * sum / float(WORLD.SHADOW.SHADOW_TAP_COUNT)
	);
}
