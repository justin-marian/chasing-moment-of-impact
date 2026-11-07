/**
 * @file
 * Color space conversions and tone mapping for the rendering pipeline.
 *
 * Provides functions to transition between non-linear sRGB and linear light,
 * apply the ACES filmic tone curve, and handle simple gamma corrections.
 */

#pragma once

#include "../settings/world.glsl"


/**
 * Decode a non-linear sRGB triple into linear light.
 *
 * @see Khronos Data Format Specification (sRGB EOTF)
 *      https://registry.khronos.org/DataFormat/specs/1.2/dataformat.1.2.pdf#14#40#14
 * @see W3C SVG Specification (Color Interpolation)
 *      https://www.w3.org/TR/2012/WD-SVG2-20120828/painting.html#FillProperty
 *
 * @param c Encoded colour, components in [0, 1].
 * @return Linear-light colour with the same components.
 */
vec3 linearColor(vec3 c) {
    return mix(
        c / WORLD.COLOR.SRGB_LINEAR_SLOPE,
        pow((c + WORLD.COLOR.SRGB_ENCODE_OFFSET) / WORLD.COLOR.SRGB_ENCODE_SLOPE, vec3(WORLD.COLOR.SRGB_GAMMA)),
        step(vec3(WORLD.COLOR.SRGB_LINEAR_THRESHOLD), c)
    );
}


/**
 * Encode a linear-light colour for an sRGB framebuffer.
 *
 * @see Khronos Data Format Specification (sRGB Inverse EOTF)
 *      https://registry.khronos.org/DataFormat/specs/1.2/dataformat.1.2.pdf#14#40#14
 *
 * @param c Linear-light colour.
 * @return Encoded colour suitable for the display.
 */
vec3 srgb(vec3 c) {
    return mix(
        c * WORLD.COLOR.SRGB_LINEAR_SLOPE,
        WORLD.COLOR.SRGB_ENCODE_SLOPE * pow(c, vec3(1.0 / WORLD.COLOR.SRGB_GAMMA)) - WORLD.COLOR.SRGB_ENCODE_OFFSET,
        step(vec3(WORLD.COLOR.SRGB_ENCODE_THRESHOLD), c)
    );
}


/**
 * Fitted ACES filmic tone curve and clamp.
 *
 * Values above one are compressed rather than cut, which is why the final
 * clamp is needed only after the rational term.
 *
 * @see Krzysztof Narkowicz, "ACES Filmic Tone Mapping Curve"
 *      https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
 *
 * @param c Linear-light HDR colour.
 * @return Linear-light LDR colour in [0, 1].
 */
vec3 aces(vec3 c) {
    return clamp(
        (c * (WORLD.COLOR.ACES_A.x * c + WORLD.COLOR.ACES_A.y)) /
        (c * (WORLD.COLOR.ACES_B.x * c + WORLD.COLOR.ACES_B.y) + WORLD.COLOR.ACES_B.z),
        0.0, 1.0
    );
}


/**
 * Simple gamma 2.2 decode.
 *
 * Used by the panorama sky which is not tagged sRGB. In the absence of a
 * proper sRGB profile, a 2.2 gamma curve is a common approximation.
 *
 * @see Pixar RenderMan, "Color Management"
 *      https://renderman.pixar.com/color-management
 *
 * @param c Non-linear colour.
 * @return Linear-light colour.
 */
vec3 gammaDecode(vec3 c) {
    return pow(max(c, vec3(0)), vec3(WORLD.COLOR.SKY_GAMMA));
}


/**
 * Simple gamma 2.2 encode.
 *
 * Matches the simple gamma decode operation.
 *
 * @see Pixar RenderMan, "Color Management"
 *      https://renderman.pixar.com/color-management
 *
 * @param c Linear-light colour.
 * @return Non-linear colour.
 */
vec3 gammaEncode(vec3 c) {
    return pow(clamp(c, 0.0, 1.0), vec3(1.0 / WORLD.COLOR.SKY_GAMMA));
}
