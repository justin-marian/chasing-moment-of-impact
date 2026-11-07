/**
 * @file
 * Impact heat telemetry and color palette settings.
 */

#pragma once


/** Impact heat telemetry and color palette. */
struct WorldImpactsSettings {
    float IMPACT_FALLOFF;
    float IMPACT_COVERAGE_LO;
    float IMPACT_COVERAGE_HI;
    float IMPACT_COVERAGE_MAX;
    float IMPACT_FACING_LO;
    float IMPACT_FACING_HI;
    vec3 BLUE;
    vec3 CYAN;
    vec3 GREEN;
    vec3 YELLOW;
    vec3 RED;
    float AMBIENT;
    float KEY_STRENGTH;
};
