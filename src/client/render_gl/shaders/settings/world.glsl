/**
 * @file
 * World domain shader settings.
 *
 * Aggregates all environment, lighting, avatar, and impact telemetry 
 * parameters into a single global constant. Included by shaders that 
 * require access to global visual tuning parameters.
 */

#pragma once

#include "collisions.glsl"
#include "shadow.glsl"
#include "avatar.glsl"
#include "scene.glsl"
#include "enclosure.glsl"
#include "body.glsl"
#include "color.glsl"
#include "impacts.glsl"


/* ========================================================================= */
/* SHADOW (SHD)                                                              */
/* ========================================================================= */

/* Golden angle for Vogel disk Poisson sampling (uniform distribution). */
#define SHD_GOLDEN_ANGLE 2.39996323
/* 32 taps balances performance and visual smoothness. */
#define SHD_TAPS 32
/* Filter radius of 1 creates a 3x3 PCF kernel (9 samples). */
#define SHD_RADIUS 1
#define SHD_FOOT_SCALE vec2(0.6, 0.46)
#define SHD_FOOT_FALLOFF 2.0
#define SHD_FOOT_STRENGTH 0.32
#define SHD_FOOT_HEIGHT 0.8
#define SHD_LIGHT_DIR WORLD_RENDERING_SCENE_SHADOW_DIRECTION
/* Slope-scaled bias. The max() clamp prevents shadow acne on parallel surfaces.
   [Link: https://github.com/defold/defold/blob/dev/engine/graphics/src/opengl/shadows_glsl.md] */
#define SHD_BIAS_SCALE 0.0005
#define SHD_BIAS_MIN 0.00012
#define SHD_TINT vec3(201.0, 198.0, 191.0) / 255.0
#define SHD_FOOTPRINT_R 0.24
#define SHD_FOOTPRINT_F 4.0
#define SHD_FOOTPRINT_S 0.35


/* ========================================================================= */
/* AVATAR (AVT)                                                              */
/* ========================================================================= */

/* Synced with C++ world settings to ensure GPU/CPU capacity match. */
// #define AVT_SLOTS WORLD_RENDERING_SCENE_IMPACT_MAX_AVATAR


/* ========================================================================= */
/* SCENE (SCN)                                                               */
/* ========================================================================= */

/* Low fog density for subtle aerial perspective. */
#define SCN_FOG 0.003
/* Frequencies for natural-looking procedural ground noise. */
#define SCN_VAR_FREQ_X 1.1
#define SCN_VAR_FREQ_Z 0.8
#define SCN_VAR_AMT 0.04
#define SCN_GRID_STR 0.035
/* High powers create tight, sharp highlights rather than broad washouts. */
#define SCN_RIM_POW 4.0
#define SCN_RIM_STR 0.012
#define SCN_EDGE_POW 24.0
#define SCN_EDGE_STR 0.012
#define SCN_SURF_GROUND 1
#define SCN_SURF_ARENA 2
#define SCN_FILL_DIR normalize(vec3(0.8, 0.3, -0.5))
#define SCN_LINE_COLOR vec3(222.0, 220.0, 214.0) / 255.0
#define SCN_FINE_ALPHA 0.08
#define SCN_BOLD_ALPHA 0.16
#define SCN_MAJOR_STEP 10.0
#define SCN_FWIDTH_MIN 0.0001


/* ========================================================================= */
/* ENCLOSURE (ENC)                                                           */
/* ========================================================================= */

/* Low core alpha ensures glass transparency, edge alpha ramps up for boundaries. */
#define ENC_TINT vec3(0.64, 0.78, 0.94)
#define ENC_ALPHA_CORE 0.025
#define ENC_EDGE_ALPHA 0.20
#define ENC_EDGE_START 0.72
/* Fresnel power and strength control the edge highlight falloff. */
#define ENC_FRES_POW 4.0
#define ENC_FRES_STR 0.35
#define ENC_HEAT_ALPHA 0.75


/* ========================================================================= */
/* LIGHTING (LGT)                                                            */
/* ========================================================================= */

/* Normalized direction ensures consistent intensity regardless of vector magnitude. */
#define LGT_KEY_DIR normalize(WORLD_RENDERING_SCENE_SHADOW_DIRECTION)


/* ========================================================================= */
/* BACKGROUND (BGD)                                                          */
/* ========================================================================= */

/* Soft off-white paper tones (0.91-0.97 range) scaled to 0-1 for UI blending. */
#define BGD_PAPER_BOT vec3(233.0, 233.0, 231.0) / 255.0
#define BGD_PAPER_TOP vec3(247.0, 246.0, 243.0) / 255.0
#define BGD_PAPER_CAP vec3(1.0)
#define BGD_CAP_PX 40.0


/* ========================================================================= */
/* BODIES (BDY)                                                              */
/* ========================================================================= */

#define BDY_FLASH_HL 0.3
#define BDY_FLASH_SPEC 0.5
/* Fresnel F0 of 0.04 approximates standard dielectric (IOR ~1.5) reflection.
   [Link: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/Renderer/shaders/brdf.glsl] */
#define BDY_FRES_F0 0.04
/* Schlick exponent tightens the falloff curve. */
#define BDY_FRES_SCHLICK 5.0
#define BDY_FRES_STR 0.12
#define BDY_SPEC_STR 0.085
#define BDY_SURF_RESP 0.85


/* ========================================================================= */
/* COLOR (CLR)                                                               */
/* ========================================================================= */

/* Exact IEC standard sRGB linearization thresholds. 
   [Link: https://registry.khronos.org/DataFormat/specs/1.2/dataformat.1.2.pdf#14#40#14] */
#define CLR_SRGB_LIN_THRESH 0.04045
#define CLR_SRGB_ENC_THRESH 0.0031308
#define CLR_SRGB_LIN_SLOPE 12.92
#define CLR_SRGB_ENC_SLOPE 1.055
#define CLR_SRGB_ENC_OFFSET 0.055
#define CLR_SRGB_GAMMA 2.4
/* ACES Narkowicz curve fit constants.
   [Link: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/] */
#define CLR_ACES_A vec2(2.51, 0.03)
#define CLR_ACES_B vec3(2.43, 0.59, 0.14)
#define CLR_SKY_GAMMA 2.2


/* ========================================================================= */
/* IMPACTS (IMP)                                                             */
/* ========================================================================= */

/* Falloff yields a smooth exponential heat drop-off. */
#define IMP_FALLOFF 3.0
#define IMP_COV_LO 0.004
#define IMP_COV_HI 0.065
#define IMP_COV_MAX 0.96
/* Facing limits force heat to fade naturally around geometric edges. */
#define IMP_FACE_LO -0.1
#define IMP_FACE_HI 0.6
#define IMP_BLUE vec3(0.015, 0.025, 0.48)
#define IMP_CYAN vec3(0.020, 0.630, 0.88)
#define IMP_GREEN vec3(0.180, 0.720, 0.28)
#define IMP_YELLOW vec3(0.980, 0.780, 0.06)
#define IMP_RED vec3(0.720, 0.015, 0.015)
/* Ambient and key light contribution to the impact heat color. */
#define IMP_AMBIENT 0.86
#define IMP_KEY_STR 0.14


/** Aggregated world shader settings. */
struct WorldShaderSettings {
    WorldShadowSettings SHADOW;
    // WorldAvatarSettings AVATAR;
    WorldSceneSettings SCENE;
    WorldEnclosureSettings ENCLOSURE;
    WorldLightingSettings LIGHTING;
    WorldBackgroundSettings BACKGROUND;
    WorldBodiesSettings BODIES;
    WorldColorSettings COLOR;
    WorldImpactsSettings IMPACTS;
};


/* Global shader settings initialized using the readable macros defined above. */
const WorldShaderSettings WORLD = WorldShaderSettings(
    /* SHADOW */
    WorldShadowSettings(
        SHD_GOLDEN_ANGLE, SHD_TAPS, SHD_RADIUS, SHD_FOOT_SCALE,
        SHD_FOOT_FALLOFF, SHD_FOOT_STRENGTH, SHD_FOOT_HEIGHT, SHD_LIGHT_DIR,
        SHD_BIAS_SCALE, SHD_BIAS_MIN, SHD_TINT, SHD_FOOTPRINT_R,
        SHD_FOOTPRINT_F, SHD_FOOTPRINT_S
    ),
    
    /* AVATAR */
    // WorldAvatarSettings(
    //     AVT_SLOTS
    // ),
    
    /* SCENE */
    WorldSceneSettings(
        SCN_FOG, SCN_VAR_FREQ_X, SCN_VAR_FREQ_Z, SCN_VAR_AMT, SCN_GRID_STR,
        SCN_RIM_POW, SCN_RIM_STR, SCN_EDGE_POW, SCN_EDGE_STR,
        SCN_SURF_GROUND, SCN_SURF_ARENA, SCN_FILL_DIR, SCN_LINE_COLOR,
        SCN_FINE_ALPHA, SCN_BOLD_ALPHA, SCN_MAJOR_STEP, SCN_FWIDTH_MIN
    ),
    
    /* ENCLOSURE */
    WorldEnclosureSettings(
        ENC_TINT, ENC_ALPHA_CORE, ENC_EDGE_ALPHA, ENC_EDGE_START,
        ENC_FRES_POW, ENC_FRES_STR, ENC_HEAT_ALPHA
    ),
    
    /* LIGHTING */
    WorldLightingSettings(
        LGT_KEY_DIR
    ),
    
    /* BACKGROUND */
    WorldBackgroundSettings(
        BGD_PAPER_BOT, BGD_PAPER_TOP, BGD_PAPER_CAP, BGD_CAP_PX
    ),
    
    /* BODIES */
    WorldBodiesSettings(
        BDY_FLASH_HL, BDY_FLASH_SPEC, BDY_FRES_F0, BDY_FRES_SCHLICK,
        BDY_FRES_STR, BDY_SPEC_STR, BDY_SURF_RESP
    ),
    
    /* COLOR */
    WorldColorSettings(
        CLR_SRGB_LIN_THRESH, CLR_SRGB_ENC_THRESH, CLR_SRGB_LIN_SLOPE,
        CLR_SRGB_ENC_SLOPE, CLR_SRGB_ENC_OFFSET, CLR_SRGB_GAMMA,
        CLR_ACES_A, CLR_ACES_B, CLR_SKY_GAMMA
    ),
    
    /* IMPACTS */
    WorldImpactsSettings(
        IMP_FALLOFF, IMP_COV_LO, IMP_COV_HI, IMP_COV_MAX, IMP_FACE_LO, IMP_FACE_HI,
        IMP_BLUE, IMP_CYAN, IMP_GREEN, IMP_YELLOW, IMP_RED,
        IMP_AMBIENT, IMP_KEY_STR
    )
);
