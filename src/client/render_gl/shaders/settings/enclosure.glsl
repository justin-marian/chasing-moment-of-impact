/**
 * @file
 * Glass wall visual settings for world rendering.
 */

#pragma once


/** Glass wall visual settings. */
struct WorldEnclosureSettings {
    vec3 GLASS_TINT;
    float GLASS_ALPHA_CORE;
    float GLASS_EDGE_ALPHA;
    float GLASS_EDGE_START;
    float GLASS_FRESNEL_POWER;
    float GLASS_FRESNEL_STRENGTH;
    float WALL_HEAT_ALPHA;
};
