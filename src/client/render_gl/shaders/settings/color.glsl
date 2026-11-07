/**
 * @file
 * Color space conversion and tone mapping constants.
 */

#pragma once


/** Color space conversion and tone mapping constants. */
struct WorldColorSettings {
    float SRGB_LINEAR_THRESHOLD;
    float SRGB_ENCODE_THRESHOLD;
    float SRGB_LINEAR_SLOPE;
    float SRGB_ENCODE_SLOPE;
    float SRGB_ENCODE_OFFSET;
    float SRGB_GAMMA;
    vec2 ACES_A;
    vec3 ACES_B;
    float SKY_GAMMA;
};
