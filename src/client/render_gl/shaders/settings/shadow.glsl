/**
 * @file
 * Shadow filtering and projection settings for world rendering.
 */

#pragma once


/** Shadow filtering and projection settings. */
struct WorldShadowSettings {
    float GOLDEN_ANGLE;
    int SHADOW_TAP_COUNT;
    int SHADOW_FILTER_RADIUS;
    vec2 FOOT_SHADOW_SCALE;
    float FOOT_SHADOW_FALLOFF;
    float FOOT_SHADOW_STRENGTH;
    float FOOT_SHADOW_HEIGHT_FALLOFF;
    vec3 SHADOW_LIGHT_DIR;
    float SHADOW_BIAS_SCALE;
    float SHADOW_BIAS_MIN;
    vec3 SHADOW_TINT;
    float SHADOW_FOOTPRINT_RADIUS;
    float SHADOW_FOOTPRINT_FALLOFF;
    float SHADOW_FOOTPRINT_STRENGTH;
};
