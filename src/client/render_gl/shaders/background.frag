/**
 * @file
 * Render the full-screen background gradient and solid top cap.
 *
 * Interpolates vertically between two paper-like colors and applies a solid
 * color band at the top edge based on the viewport pixel height.
 */

#version 460 core

#include "settings/world.glsl"


in vec2 uv;

out vec4 fragmentColor;

uniform float uHeight_px;


void main() {
	// Interpolate from the bottom paper color to the top paper color along the Y-axis.
	vec3 c = mix(WORLD.BACKGROUND.PAPER_BOTTOM, WORLD.BACKGROUND.PAPER_TOP,  smoothstep(0.0, 1.0, uv.y));
	// Blend in the solid cap color near the top of the screen.
	// The cap height is scaled by the viewport resolution to maintain a consistent pixel size.
	c = mix(c, WORLD.BACKGROUND.PAPER_CAP, smoothstep(1.0 - WORLD.BACKGROUND.PAPER_CAP_PX / uHeight_px, 1.0, uv.y));
	fragmentColor = vec4(c, 1.0);
}