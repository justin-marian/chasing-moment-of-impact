/**
 * @file
 * Private shader settings owned by the collision domain.
 *
 * Defines geometry and impact parameters that are passed from the host
 * application to the GPU shaders.
 */

#pragma once


/** Geometry-specific settings for collision shapes. */
struct CollisionsGeometrySettings {
    float PI;
};


/** Impact-specific settings for collision events. */
struct CollisionsImpactsSettings {
    int PATCHES_PER_IMPACT;
};


/** Aggregated collision shader settings. */
struct CollisionsShaderSettings {
    CollisionsGeometrySettings GEOMETRY;
    CollisionsImpactsSettings IMPACTS;
};


/** Global collision settings instance used by shaders. */
const CollisionsShaderSettings COLLISIONS = CollisionsShaderSettings(
    CollisionsGeometrySettings(COLLISIONS_GEOMETRY_PI),
    CollisionsImpactsSettings(COLLISIONS_IMPACTS_PATCHES_PER_SURFACE)
);
