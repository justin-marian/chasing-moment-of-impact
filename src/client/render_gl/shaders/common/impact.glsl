/**
 * @file
 * Impact-heat palette and per-hit math.
 *
 * Provides pure functions and constants for calculating impact heat.
 * The storage buffer that drives the fragment shaders lives in a separate
 * header so this file parses even in front-ends that do not track
 * user-defined struct types.
 */

#pragma once

#include "../settings/world.glsl"


/**
 * Return the n-th stop of the impact severity palette.
 *
 * Written as a function rather than a const array because the sized array
 * constructor is mis-parsed by some GLSL front-ends.
 *
 * @param i Stop index in [0, 4]; out-of-range values clamp to the last stop.
 * @return The palette colour at that stop, in linear light.
 */
vec3 impactStop(int i) {
    if (i <= 0) { return WORLD.IMPACTS.BLUE; }
    if (i == 1) { return WORLD.IMPACTS.CYAN; }
    if (i == 2) { return WORLD.IMPACTS.GREEN; }
    if (i == 3) { return WORLD.IMPACTS.YELLOW; }
    return WORLD.IMPACTS.RED;
}


/**
 * Interpolate the blue, cyan, green, yellow and red severity stops from
 * normalized heat.
 *
 * @param heat Normalised heat in [0, 1].
 * @return The blended palette colour.
 */
vec3 impactPalette(float heat) {
    float ramp = clamp(heat, 0.0, 1.0) * 4.0;
    int i = min(int(ramp), 3);
    return mix(impactStop(i), impactStop(i + 1), smoothstep(0.0, 1.0, ramp - float(i)));
}


/**
 * Evaluate one patch against a local hit point and normal.
 *
 * The patch is passed as two vec4s (point + radius, normal + strength) rather
 * than a struct so lightweight GLSL front-ends that do not track user-defined
 * types can still parse this file.
 *
 * @param pointRadius    xyz = hit point, w = radius.
 * @param normalStrength xyz = hit normal, w = strength (0 = empty slot).
 * @param local          Hit point in the patch's local space.
 * @param n              Surface normal at the sample, in the same space.
 * @return Heat contribution in [0, 1].
 */
float impactHeat(vec4 pointRadius, vec4 normalStrength, vec3 local, vec3 n) {
    if (normalStrength.w <= 0.0 || pointRadius.w <= 0.0) {
        return 0.0;
    }

    vec3 delta = (local - pointRadius.xyz) / pointRadius.w;

    float facing = smoothstep(WORLD.IMPACTS.IMPACT_FACING_LO, WORLD.IMPACTS.IMPACT_FACING_HI, dot(n, normalStrength.xyz));

    return normalStrength.w * exp(-WORLD.IMPACTS.IMPACT_FALLOFF * dot(delta, delta)) * facing;
}


/**
 * Evaluate a single avatar-space uniform hit against a skinned vertex.
 *
 * Avatar hits carry no facing term because the skinned vertex has no useful
 * normal in the same local frame after the skin blend.
 *
 * @param hit   xyz = hit point in avatar space, w = radius.
 * @param nrm   xyz unused, w = strength.
 * @param local Vertex point in the same avatar space.
 * @return Heat contribution in [0, 1].
 */
float impactHeatAvatar(vec4 hit, vec4 nrm, vec3 local) {
    if (nrm.w <= 0.0) {
        return 0.0;
    }

    vec3 delta = (local - hit.xyz) / hit.w;
    return nrm.w * exp(-WORLD.IMPACTS.IMPACT_FALLOFF * dot(delta, delta));
}


/**
 * Blend a shaded colour toward the impact palette by the current heat.
 *
 * @param shaded Linear-light shaded colour.
 * @param heat   Normalised heat.
 * @param key    Diffuse key-light term for the sample.
 * @return Shaded colour with the heat blend applied.
 */
vec3 applyImpact(vec3 shaded, float heat, float key) {
    float coverage = smoothstep(WORLD.IMPACTS.IMPACT_COVERAGE_LO, WORLD.IMPACTS.IMPACT_COVERAGE_HI, heat) * WORLD.IMPACTS.IMPACT_COVERAGE_MAX;

    vec3 litHeat = impactPalette(heat) * (WORLD.IMPACTS.AMBIENT + WORLD.IMPACTS.KEY_STRENGTH * key);

    return mix(shaded, litHeat, coverage);
}