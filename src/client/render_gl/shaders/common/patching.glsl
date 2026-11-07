/**
 * @file
 * Define the shared storage buffer layout for impact patches.
 */

#pragma once


/**
 * A single impact patch record.
 *
 * The std430 array layout matches the interleaved 32-byte host application records.
 */
struct ImpactPatch {
    vec4 pointRadius;
    vec4 normalStrength;
};


/**
 * Read-only storage buffer containing the active impact patches.
 */
layout(std430, binding = WORLD_RENDERING_GPU_IMPACT_BINDING) readonly buffer ImpactPatches {
    ImpactPatch entries[];
} patches;
