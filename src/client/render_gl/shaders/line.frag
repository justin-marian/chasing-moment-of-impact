/**
 * @file
 * Render the interpolated debug-line color.
 *
 * Relies on the host to select a blending mode (straight alpha or additive).
 * Performs no encoding or tone mapping so authored debug colors appear
 * exactly as they were set on the CPU side.
 */

#version 460 core


in vec4 color;

out vec4 fragmentColor;


/**
 * Write the interpolated debug-line color.
 *
 * Alpha is forwarded verbatim and relies on the host to select a blending
 * mode that uses it (straight alpha) or ignores it (additive).
 */
void main() {
	// Forward the interpolated debug color straight to the framebuffer.
	// The host selects the blending mode (straight alpha or additive).
	fragmentColor = color;
}
