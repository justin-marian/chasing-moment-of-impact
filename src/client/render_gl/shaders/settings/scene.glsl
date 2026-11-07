/**
 * @file
 * Environment, ground, lighting, and background settings for world rendering.
 */

#pragma once


/** Environment, ground, and surface material settings. */
struct WorldSceneSettings {
    float FOG_DENSITY;
    float GROUND_VARIATION_FREQ_X;
    float GROUND_VARIATION_FREQ_Z;
    float GROUND_VARIATION_AMOUNT;
    float GROUND_GRID_STRENGTH;
    float RIM_POWER;
    float RIM_STRENGTH;
    float EDGE_SPEC_POWER;
    float EDGE_SPEC_STRENGTH;
    int SURFACE_GROUND;
    int SURFACE_ARENA;
    vec3 FILL_DIR;
    vec3 FLOOR_LINE_COLOR;
    float FLOOR_FINE_ALPHA;
    float FLOOR_BOLD_ALPHA;
    float FLOOR_MAJOR_STEP;
    float FLOOR_FWIDTH_MIN;
};


/** Primary light direction. */
struct WorldLightingSettings {
    vec3 KEY_DIR;
};


/** Background gradient colors. */
struct WorldBackgroundSettings {
    vec3 PAPER_BOTTOM;
    vec3 PAPER_TOP;
    vec3 PAPER_CAP;
    float PAPER_CAP_PX;
};
