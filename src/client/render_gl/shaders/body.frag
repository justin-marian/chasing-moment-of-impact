/**
 * @file
 * Shade rigid bodies with linear lighting, tone mapping, and impact heat.
 *
 * Combines key/fill/rim diffuse lighting, hemispherical ambient light, a
 * Fresnel rim term, and a contact flash effect. Applies ACES tone mapping
 * and sRGB encoding before blending in the local impact heat map.
 */

#version 460 core

#include "settings/world.glsl"

#include "common/color.glsl"
#include "common/impact.glsl"
#include "common/patching.glsl"


/* --- Vertex Shader Inputs --- */
in vec3 worldNormal;          // Interpolated world-space surface normal.
in vec3 worldPosition_m;      // Interpolated world-space position (for view vector).
in vec2 uv;                   // Texture coordinates (optional material mapping).
in float ao;                  // Ambient occlusion factor (0 = dark, 1 = fully lit).
in vec4 color;                // Base albedo color of the instance.
in vec4 visual;               // Per-instance state; visual.y carries the local contact flash amount.

/* --- Local Impact Interpolation (for heatmap) --- */
in vec3 impactLocalPoint_m;   // Hit point in the instance's local space.
in vec3 impactLocalNormal;    // Surface normal in the instance's local space.
flat in int impactIndex;      // Flat instance index for looking up the SSBO patch block.

/* --- Fragment Shader Output --- */
out vec4 fragmentColor;


/* --- Camera --- */
uniform vec3 uCameraPos_m;    // World-space eye position for specular/Fresnel calculations.

/* --- Key Light (Primary directional light) --- */
uniform vec3 uKeyDir;         // World-space direction.
uniform vec3 uKeyColor;       // Linear-light RGB color.
uniform float uKeyIntensity;  // Brightness multiplier.

/* --- Fill Light (Softer, opposing light) --- */
uniform vec3 uFillDir;
uniform vec3 uFillColor;
uniform float uFillIntensity;

/* --- Rim Light (Backlight for edge separation) --- */
uniform vec3 uRimDir;
uniform vec3 uRimColor;
uniform float uRimIntensity;

/* --- Ambient Light (Hemispherical fill) --- */
uniform vec3 uAmbientSky;     // Color of the sky (applied to upward-facing normals).
uniform vec3 uAmbientGround;  // Color of the ground bounce (applied to downward normals).
uniform float uAmbientIntensity;

/* --- Effect State --- */
uniform float uContactFlash;  // Global contact flash (e.g., for wall impacts).
uniform bool uImpactEnabled;  // Toggles the SSBO impact heatmap lookup entirely.


/**
 * Evaluate the strongest retained local Gaussian impact patch for one instance.
 *
 * Reads the SSBO declared above through its instance name; the per-vertex
 * inputs are passed as parameters so the function has no hidden dependency
 * on the main function's locals.
 *
 * @param idx     Instance index into the patch block.
 * @param local   Hit point in the instance's local space.
 * @param localN  Surface normal in the instance's local space.
 * @param enabled False disables the lookup entirely.
 * @return Strongest heat in [0, 1].
 */
float impactHeatAll(int idx, vec3 local, vec3 localN, bool enabled) {
	if (!enabled) {
		return 0.0;
	}

	vec3 n = normalize(localN);
	float heat = 0.0;

	// Iterate through all retained patches for this instance.
	for (int k = 0; k < COLLISIONS.IMPACTS.PATCHES_PER_IMPACT; ++k) {
		// Calculate the absolute index into the flat SSBO array.
		int i = idx * COLLISIONS.IMPACTS.PATCHES_PER_IMPACT + k;
		// Overlapping patches: take the maximum heat to avoid double-brightening.
		heat = max(heat, impactHeat(patches.entries[i].pointRadius, patches.entries[i].normalStrength, local, n));
	}

	return heat;
}


void main() {
	// Surface response controls the broadness of the specular highlight (lower = sharper).
	float surfaceResponse = WORLD.BODIES.SURFACE_RESPONSE;

	// Prepare normalized basis vectors for the lighting calculations.
	vec3 N = normalize(worldNormal);
	vec3 V = normalize(uCameraPos_m - worldPosition_m);
	vec3 L_key = normalize(uKeyDir);
	vec3 L_fill = normalize(uFillDir);
	vec3 L_rim = normalize(uRimDir);

	// Halfway vector for Blinn-Phong specular (between view and key light).
	vec3 H_key = normalize(V + L_key);

	// Blend the per-instance flash with the global contact flash (e.g., for wall hits).
	float flash = max(uContactFlash, visual.y);

	// Modulate the specular exponent (tighter highlight) and intensity based on the flash.
	float spec = pow(
        max(dot(N, H_key), 0.0),
        (1.0 - WORLD.BODIES.FLASH_HIGHLIGHT_MOD * flash) / surfaceResponse) *
		(1.0 - surfaceResponse * WORLD.BODIES.FLASH_SPEC_MOD);

	// Sum the diffuse contributions (N dot L) of the three directional lights.
	vec3 diffuse =
		uKeyColor * uKeyIntensity * max(dot(N, L_key), 0.0) +
		uFillColor * uFillIntensity * max(dot(N, L_fill), 0.0) +
		uRimColor * uRimIntensity * max(dot(N, L_rim), 0.0);

	// Hemispherical ambient light: blend sky and ground colors based on normal orientation.
	vec3 ambient = mix(uAmbientGround, uAmbientSky, N.y * 0.5 + 0.5) * uAmbientIntensity * ao;

	// Fresnel rim effect using the Schlick approximation (stronger at grazing angles).
	float fresnel =
		WORLD.BODIES.FRESNEL_F0 +
		(1.0 - WORLD.BODIES.FRESNEL_F0) * pow(1.0 - max(dot(N, V), 0.0), WORLD.BODIES.FRESNEL_SCHLICK);

	// Combine surface albedo with lighting, then add specular and fresnel highlights.
	vec3 result =
		linearColor(color.rgb) * (diffuse + ambient) +
		uKeyColor * spec * WORLD.BODIES.SPEC_STRENGTH +
		vec3(1.0) * fresnel * WORLD.BODIES.FRESNEL_STRENGTH;

	// Fetch the localized impact heat and the raw diffuse key term for the heatmap lighting.
	float heat = impactHeatAll(impactIndex, impactLocalPoint_m, impactLocalNormal, uImpactEnabled);
	float key = max(dot(N, L_key), 0.0);

	// Apply ACES tonemapping -> sRGB encoding -> impact heat -> final output.
	fragmentColor = vec4(applyImpact(srgb(aces(result)), heat, key), 1.0);
}
