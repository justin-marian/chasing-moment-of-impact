/**
 * @file
 * Filtered shadow lookup with normal-dependent bias.
 *
 * Provides a shared light-space depth map sampler that applies a
 * normal-dependent bias to prevent shadow acne.
 */

#pragma once


#include "../settings/world.glsl"


/**
 * Sample the light-space depth map with normal-dependent bias and a 3-by-3 filter.
 *
 * Returns 1.0 fully lit, 0.0 fully shadowed. Samples outside the light
 * frustum are treated as lit.
 *
 * @see LearnOpenGL, "Shadow Mapping" — General overview of the shadow pipeline
 *      https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
 * @see TU Wien, "Shadow Map Filtering" — Explains the 3×3 Percentage-Closer
 *      Filtering (PCF) kernel used here[reference:0]
 * @see Defold Engine, "shadows_glsl.md" — Demonstrates slope/normal-dependent
 *      bias to prevent shadow acne[reference:1][reference:2]
 *
 * @param worldPos  Fragment position in world space.
 * @param n         World-space surface normal.
 * @param lightProj World-to-light-clip matrix.
 * @param shadowMap Depth map.
 * @return Visibility in [0, 1].
 */
float shadow(vec3 worldPos, vec3 n, mat4 lightProj, sampler2D shadowMap) {
    const float FILTER_SAMPLES = 9.0;

    vec4 light = lightProj * vec4(worldPos, 1.0);
    vec3 p = light.xyz / light.w * 0.5 + 0.5;
    
    if (any(lessThan(p, vec3(0))) || any(greaterThan(p, vec3(1)))) {
        return 1.0;
    }

    float bias = max(
        WORLD.SHADOW.SHADOW_BIAS_SCALE * (1.0 - dot(n, normalize(WORLD.SHADOW.SHADOW_LIGHT_DIR))),
        WORLD.SHADOW.SHADOW_BIAS_MIN
    );
    
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    float sum = 0.0;

    for (int x = -WORLD.SHADOW.SHADOW_FILTER_RADIUS; x <= WORLD.SHADOW.SHADOW_FILTER_RADIUS; ++x) {
        for (int y = -WORLD.SHADOW.SHADOW_FILTER_RADIUS; y <= WORLD.SHADOW.SHADOW_FILTER_RADIUS; ++y) {
            float sampleDepth = texture(shadowMap, p.xy + vec2(x, y) * texel).r;
            sum += (p.z - bias <= sampleDepth) ? 1.0 : 0.0;
        }
    }
    
    return sum / FILTER_SAMPLES;
}
