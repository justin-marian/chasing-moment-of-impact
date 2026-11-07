/**
 * @file
 * Rendering settings owned by the 3D world domain.
 *
 * Defines visual tuning parameters for impacts, rigid bodies, meshes,
 * GPU binding slots, scene cameras, lighting, and overlays.
 */

#pragma once

#include "settings/world/constants.h"

#include <array>


namespace settings {

struct World::Rendering {

    struct Impacts {

        struct History {
            /* Merge nearby patches only when their surface normals face a similar direction. */
            inline static constexpr float MERGE_NORMAL_DOT = .8f;
            inline static constexpr float MERGE_RADIUS_FRACTION = .3f;

            /* Protect a new patch briefly before a newer collision may replace it. */
            inline static constexpr float REPLACE_AGE_S = .2f;

            /* Sublinear fading leaves lower-energy colors visible as the retained patch ages. */
            inline static constexpr float DECAY_EXPONENT = .6f;

            /* Radius grows with impact strength, then clamps in metres to remain local to the hit. */
            inline static constexpr float PATCH_RADIUS_BASE = .7f;
            inline static constexpr float PATCH_RADIUS_SLOPE = 1.1f;
            inline static constexpr float PATCH_RADIUS_MIN_M = .08f;
            inline static constexpr float PATCH_RADIUS_MAX_M = 3.f;

            /* Slightly larger wall patches make contact regions legible through the enclosure. */
            inline static constexpr float WALL_RADIUS_GROWTH = 1.3f;
        };
    };


    struct RigidBodies {

        struct Appearance {
            inline static constexpr double CONTACT_FLASH_S = .12;

            /* Neighboring body IDs receive well-separated hues around the color wheel. */
            inline static constexpr unsigned BODY_HUE_STEP_DEG = 137;
            inline static constexpr float BODY_SATURATION = .66f;
            inline static constexpr float BODY_VALUE = .78f;
        };


        struct Lighting {
            inline static constexpr std::uint32_t KEY_COLOR_SRGB = 0xFBF7EF;       /* Warm off-white */
            inline static constexpr std::uint32_t FILL_COLOR_SRGB = 0xE8ECEF;      /* Cool light gray */
            inline static constexpr std::uint32_t RIM_COLOR_SRGB = 0xF2EDE4;       /* Warm light beige */
            inline static constexpr std::uint32_t AMBIENT_SKY_SRGB = 0xEFEFEF;     /* Near white */
            inline static constexpr std::uint32_t AMBIENT_GROUND_SRGB = 0xD3D1CB;  /* Light warm gray */

            inline static constexpr float KEY_INTENSITY = .85f, FILL_INTENSITY = .25f;
            inline static constexpr float RIM_INTENSITY = .15f, AMBIENT_INTENSITY = .35f;
        };
    };


    struct Meshes {
        /* Rounded-cube subdivision count; the flat world-box adapter skips this rounding. */
        inline static constexpr int CUBE_SEGMENTS = 8;

        /* Stable outward triangle winding for the icosahedron before sphere subdivision. */
        inline static constexpr int FACES[20][3] = {
            {0, 11, 5}, {0, 5, 1},   {0, 1, 7},  {0, 7, 10}, {0, 10, 11},
            {1, 5, 9},  {5, 11, 4},  {11, 10, 2},{10, 7, 6}, {7, 1, 8},
            {3, 9, 4},  {3, 4, 2},   {3, 2, 6},  {3, 6, 8},  {3, 8, 9},
            {4, 9, 5},  {2, 4, 11},  {6, 2, 10}, {8, 6, 7},  {9, 8, 1}
        };

        /* Radial segments shared by the cylinder and cone to keep their silhouette smooth. */
        inline static constexpr int ROUND_SEGMENTS = 48;
    };


    struct Gpu {
        inline static constexpr int IMPACT_BINDING = 3;
        
        inline static constexpr std::size_t WALL_OFFSET = 
            settings::Collisions::Storage::MAX_BODIES * settings::Collisions::Impacts::PATCHES_PER_SURFACE;
            
        inline static constexpr std::size_t IMPACT_SLOT_COUNT = 
            WALL_OFFSET + settings::Collisions::Geometry::BOX_FACE_COUNT * settings::Collisions::Impacts::PATCHES_PER_SURFACE;
    };


    struct Scene {

        struct Targets {
            static inline constexpr int MSAA_SAMPLES = 4;
            
            static inline constexpr std::array<float, 4> FULL_CLEAR_COLOR{.97f, .969f, .96f, 1.f};   /* Off-white */
            static inline constexpr std::array<float, 4> CANVAS_CLEAR_COLOR{.91f, .927f, .938f, 1.f}; /* Cool light gray */
        };


        struct Camera {
            static inline constexpr float FOV_DEGREES = 48.f;
            static inline constexpr float NEAR_PLANE = .05f;
            static inline constexpr float FAR_PLANE = 180.f;

            static inline constexpr std::array<float, 3> UP{0.f, 1.f, 0.f};
            static inline constexpr std::array<float, 3> FOCUS_OFFSET{0.f, 1.f, 0.f};
        };


        // MMO specific: Sky rendering settings.
        // struct Sky {
        //     static inline constexpr std::size_t COUNT = 25;
        //     static inline constexpr float MIN_EXPOSURE = .25f;
        //     static inline constexpr float MAX_EXPOSURE = 2.f;
        //     static inline constexpr int TEXTURE_UNIT = 4;
        // };


        struct Shadow {
            static inline constexpr int RESOLUTION = 2048;
            static inline constexpr int TEXTURE_UNIT = 2;
            static inline constexpr float DISTANCE = 80.f;
            static inline constexpr float EXTENT = 48.f;
            static inline constexpr float NEAR_PLANE = 1.f;
            static inline constexpr float FAR_PLANE = 160.f;

            static inline constexpr std::array<float, 3> DIRECTION{-0.4f, 0.8f, 0.5f};
            static inline constexpr std::array<float, 4> BORDER{1.f, 1.f, 1.f, 1.f};
        };


        struct Surface {
            static inline constexpr int ALBEDO_TEXTURE_UNIT = 0;
            static inline constexpr int NORMAL_TEXTURE_UNIT = 1;

            static inline constexpr std::array<float, 3> GROUND_COLOR{.32f, .46f, .27f}; /* Olive green */
            
            // MMO specific: Avatar tinting.
            // static inline constexpr std::array<float, 3> SECONDARY_AVATAR_TINT{.85f, .83f, .79f};
            // static inline constexpr float SECONDARY_AVATAR_TINT_MIX = .6f;
        };


        struct Impact {
            static inline constexpr std::size_t MAX_SCENE_IMPACTS = 32;
            static inline constexpr float FADE_EXPONENT = .6f;

            static inline constexpr float WORLD_IMPACT_BASE_RADIUS = .4f;
            static inline constexpr float WORLD_IMPACT_RADIUS_SCALE = .35f;
            
            // MMO specific: Avatar impact visuals.
            // static inline constexpr std::size_t MAX_AVATAR = 4;
            // static inline constexpr float AVATAR_IMPACT_BASE_RADIUS = .3f;
            // static inline constexpr float AVATAR_IMPACT_RADIUS_SCALE = .3f;
            // static inline constexpr float AVATAR_IMPACT_HORIZONTAL_SCALE = .6f;
        };


        // MMO specific: Animation playback speeds and blends.
        // struct Animation {
        //     static inline constexpr float WALK_REFERENCE_SPEED = 2.4f;
        //     static inline constexpr float WALK_MIN_PLAYBACK = .6f;
        //     static inline constexpr float WALK_MAX_PLAYBACK = 1.5f;
        //
        //     static inline constexpr float RUN_REFERENCE_SPEED = 5.5f;
        //     static inline constexpr float RUN_MIN_PLAYBACK = .7f;
        //     static inline constexpr float RUN_MAX_PLAYBACK = 1.3f;
        //
        //     static inline constexpr float IDLE_WALK_BLEND = .2f;
        //     static inline constexpr float RUN_BLEND = .15f;
        //     static inline constexpr float FAST_BLEND = .08f;
        // };


        // MMO specific: Usernames, health bars, and player state overlays.
        // struct Overlay {
        //     static inline constexpr float LABEL_HEIGHT = 2.08f;
        //     static inline constexpr float LABEL_MINIMUM_HALF_WIDTH = 50.f;
        //     static inline constexpr float LABEL_HORIZONTAL_PADDING = 10.f;
        //
        //     /* Separate rounded username/state pills share a baseline and leave a small visible gap. */
        //     static inline constexpr float STATE_HORIZONTAL_PADDING = 9.f, STATE_GAP = 6.f;
        //     
        //     static inline constexpr unsigned NAME_BACKGROUND = 0xF9F8F5;  /* Off-white */
        //     static inline constexpr unsigned NAME_TEXT = 0x45494C;        /* Dark slate */
        //     static inline constexpr unsigned STATE_BACKGROUND = 0xDFEBF7; /* Light blue */
        //     static inline constexpr unsigned STATE_TEXT = 0x3C5975;       /* Medium blue */
        //
        //     static inline constexpr float LABEL_VERTICAL_PADDING = 5.f;
        //     static inline constexpr float LABEL_OVERLAP_PADDING = 24.f;
        //     static inline constexpr float LABEL_STACK_SPACING = 34.f;
        //     static inline constexpr float HEALTH_HALF_WIDTH = 48.f;
        //     static inline constexpr float HEALTH_HEIGHT = 14.f, HEALTH_FONT_SIZE = 11.f;
        // };


        // MMO specific: Avatar capsule generation.
        // struct Capsule {
        //     static inline constexpr int SEGMENTS = 32;
        //     static inline constexpr float FULL_TURN = 6.2831853f;
        // };
    };


    struct Shaders {
        static constexpr const char* VERSION = "#version 460 core";
    };

};

} // namespace settings
