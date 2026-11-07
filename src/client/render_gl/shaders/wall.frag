#version 460 core

#include "settings/world.glsl"

#include "common/impact.glsl"
#include "common/patching.glsl"

in vec3 worldNormal;
in vec3 worldPosition_m;

in vec2 uv;
in float ao;

in vec4 color;
in vec4 visual;

flat in int impactIndex;

out vec4 fragmentColor;

uniform vec3 uCameraPos_m;
uniform bool uImpactEnabled;


/**
 * Radial falloff of the glass tint; walls read as edges rather than slabs.
 *
 * @param faceUv Wall face coordinates, expected in [0, 1].
 * @return Alpha coefficient in [WORLD.ENCLOSURE.GLASS_ALPHA_CORE, WORLD.ENCLOSURE.GLASS_ALPHA_CORE + WORLD.ENCLOSURE.GLASS_EDGE_ALPHA].
 */
float glassRadialAlpha(vec2 faceUv) {
    float edge = max(abs(faceUv.x * 2.0 - 1.0), abs(faceUv.y * 2.0 - 1.0));
    return WORLD.ENCLOSURE.GLASS_ALPHA_CORE +
           WORLD.ENCLOSURE.GLASS_EDGE_ALPHA * 
           smoothstep(WORLD.ENCLOSURE.GLASS_EDGE_START, 1.0, edge);
}


float glassFresnel(vec3 N, vec3 V) {
    return pow(1.0 - abs(dot(N, V)), WORLD.ENCLOSURE.GLASS_FRESNEL_POWER) * WORLD.ENCLOSURE.GLASS_FRESNEL_STRENGTH;
}


/**
 * Evaluate the strongest retained wall impact patch for one instance.
 *
 * Reads the SSBO declared above through its instance name; the per-fragment
 * inputs (instance index, world position, world normal, enable flag) are
 * passed as parameters so the function has no hidden dependency on main()'s
 * locals. Passing the SSBO arrays themselves is not possible in GLSL because
 * unsized arrays cannot be function parameters.
 *
 * @param idx     Instance index into the patch block.
 * @param pos     Fragment position in world space.
 * @param n       World-space surface normal.
 * @param enabled False disables the lookup entirely.
 * @return Strongest heat in [0, 1].
 */
float wallImpactHeat(int idx, vec3 pos, vec3 n, bool enabled) {
    if (!enabled) {
        return 0.0;
    }

    float heat = 0.0;
    for (int k = 0; k < COLLISIONS.IMPACTS.PATCHES_PER_IMPACT; ++k) {
        int i = idx * COLLISIONS.IMPACTS.PATCHES_PER_IMPACT + k;
        heat = max(heat, impactHeat(patches.entries[i].pointRadius, patches.entries[i].normalStrength, pos, n));
    }

    return heat;
}


void main() {
    vec3 N = normalize(worldNormal);
    vec3 V = normalize(uCameraPos_m - worldPosition_m);

    float alpha = glassRadialAlpha(uv);
    float fresnel = glassFresnel(N, V);
    float heat = wallImpactHeat(impactIndex, worldPosition_m, N, uImpactEnabled);

    float coverage = smoothstep(WORLD.IMPACTS.IMPACT_COVERAGE_LO, WORLD.IMPACTS.IMPACT_COVERAGE_HI, heat);
    vec3 glass = clamp(WORLD.ENCLOSURE.GLASS_TINT + vec3(fresnel), 0.0, 1.0);

    fragmentColor = vec4(
        mix(glass, impactPalette(heat), coverage),
        max(alpha + fresnel * 0.15, coverage * WORLD.ENCLOSURE.WALL_HEAT_ALPHA)
    );
}
