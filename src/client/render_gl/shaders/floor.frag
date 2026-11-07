#version 460 core

#include "settings/world.glsl"

/**
 * Shade the glass scene floor using the configured material and lighting palette.
 */

in vec3 worldPosition_m;
in vec3 worldNormal;

in vec2 uv;
in float ao;

in vec4 color;
in vec4 visual;

out vec4 fragmentColor;

/**
 * Draw antialiased fine and major floor grid lines with transparent coverage.
 */
void main() {
    vec2 p = worldPosition_m.xz;
    vec2 d = abs(fract(p - 0.5) - 0.5) / max(fwidth(p), vec2(WORLD.SCENE.FLOOR_FWIDTH_MIN));
    vec2 q = p / WORLD.SCENE.FLOOR_MAJOR_STEP;
    vec2 major = abs(fract(q - 0.5) - 0.5) / max(fwidth(q), vec2(WORLD.SCENE.FLOOR_FWIDTH_MIN));

    float fine = 1.0 - clamp(min(d.x, d.y), 0.0, 1.0);
    float bold = 1.0 - clamp(min(major.x, major.y), 0.0, 1.0);
    fragmentColor = vec4(
        WORLD.SCENE.FLOOR_LINE_COLOR,
        max(fine * WORLD.SCENE.FLOOR_FINE_ALPHA, bold * WORLD.SCENE.FLOOR_BOLD_ALPHA)
    );
}
