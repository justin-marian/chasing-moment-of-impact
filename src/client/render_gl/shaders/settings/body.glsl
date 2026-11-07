/**
 * @file
 * Specular and Fresnel settings for rigid bodies.
 */

#pragma once


/** Specular and Fresnel settings for rigid bodies. */
struct WorldBodiesSettings {
    float FLASH_HIGHLIGHT_MOD;
    float FLASH_SPEC_MOD;
    float FRESNEL_F0;
    float FRESNEL_SCHLICK;
    float FRESNEL_STRENGTH;
    float SPEC_STRENGTH;
    float SURFACE_RESPONSE;
};
