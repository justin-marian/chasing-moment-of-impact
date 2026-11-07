/**
 * @file
 * Transform debug bounds and velocity lines using the active view projection.
 *
 * The view-projection matrix is pinned to location zero so the host can bind
 * the camera once per frame and reuse it across every pass that declares the
 * same location. Changing this number on one side only would silently break
 * the shared binding.
 */

#version 460 core


layout(location = 0) in vec3 aPosition_m;
layout(location = 1) in vec4 aColor;

out vec4 color;


/* Pinned to location zero so this pass shares the binding with the rigid-body pass. */
layout(location = 0) uniform mat4 projectionView;


/**
 * Project a debug-line vertex and forward its display color.
 *
 * The homogeneous coordinate is 1.0 because debug geometry is authored in
 * world-space metres with no projection ambiguity; the view-projection
 * matrix already carries any perspective the camera needs. Color is forwarded
 * verbatim so CPU-authored debug colors reach the framebuffer without an encode step.
 */
void main() {
	// Forward the CPU-authored color directly to the fragment stage.
	color = aColor;
	// Transform the world-space position into clip space.
	gl_Position = projectionView * vec4(aPosition_m, 1.0);
}
