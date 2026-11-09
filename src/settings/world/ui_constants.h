/**
 * @file
 * UI settings owned by the 3D world domain.
 *
 * Defines logical pixel dimensions, colors, typography, and layout
 * constants for the ImGui-based world interface.
 */

#pragma once

#include "settings/world/constants.h"

#include <imgui.h>


namespace settings {

struct World::Ui {

    struct Control {

        /**
         * Vertical spacing that separates related settings without creating oversized panel gaps.
         */
        struct Section {
            static constexpr float BEFORE = 8, AFTER = 4;
        };


        /**
         * Consistent logical-pixel height for native action controls.
         */
        struct Button {
            static constexpr float HEIGHT = 30;
        };
    };


    struct Slider {
        static constexpr float LABEL_WIDTH = 114, VALUE_WIDTH = 62, GAP = 6;
        static constexpr float TRACK_HEIGHT = 3, HANDLE_RADIUS = 5, VERTICAL_PADDING = 5;

        /* Native scalar tracks have two logical pixels of inner padding at each endpoint. */
        static constexpr float NATIVE_PADDING = 2;

        static constexpr unsigned TRACK = 0xD2D2CF, FILL = 0x96AFCB;
        static constexpr unsigned HANDLE = 0xFFFFFF, OUTLINE = 0xE2E4E7, SHADOW = 0x87909A;
        static constexpr std::size_t FORMAT_CAPACITY = 64;
    };


    struct Window {
        /* Logical pixels; preserve a small usable control surface when clamping windows to the viewport. */
        static constexpr float MARGIN = 8, TOOLBAR_HEIGHT = 40, FIRST = 60;
        static constexpr float MINIMUM_WIDTH = 150, MINIMUM_HEIGHT = 64;
        static constexpr float MINIMUM_TEXT_SCALE = .8f, MAXIMUM_TEXT_SCALE = 1.5f;
    };


    struct Style {

        /**
         * Logical-pixel padding between related controls and neighbouring windows.
         */
        struct Spacing {
            static constexpr float WINDOW = 12, FRAME_X = 9, FRAME_Y = 6;
            static constexpr float ITEM_X = 10, ITEM_Y = 8;
        };


        /**
         * Native control rounding, border and minimum grab dimensions, with an almost opaque window background.
         */
        struct Shape {
            static constexpr float WINDOW_RADIUS = 8, CONTROL_RADIUS = 5, BORDER_WIDTH = 1;
            static constexpr float GRAB_SIZE = 12, WINDOW_ALPHA = .98f;
        };


        /**
         * Increasing accent opacity distinguishes normal, hovered and active controls.
         */
        struct Feedback {
            static constexpr float NORMAL_ALPHA = .12f, HOVER_ALPHA = .22f, ACTIVE_ALPHA = .32f;
        };
    };


    struct Color {

        /**
         * Neutral background and border colors that keep scene controls visually quiet.
         */
        struct Surface {
            static constexpr unsigned PAGE = 0xFFFFFF;        /* Pure white */
            static constexpr unsigned PANEL = 0xF7F6F3;       /* Off-white / warm gray */
            static constexpr unsigned BORDER = 0xD3D1CB;      /* Light warm gray */
            static constexpr unsigned BORDER_SOFT = 0xE9E9E7; /* Very light warm gray */
        };


        /**
         * Text contrast levels for normal, supporting and unavailable controls.
         */
        struct Text {
            static constexpr unsigned PRIMARY = 0x37352F;   /* Dark charcoal */
            static constexpr unsigned SECONDARY = 0x787774; /* Medium warm gray */
            static constexpr unsigned TERTIARY = 0x9B9A97;  /* Light warm gray */
        };


        /**
         * Action and status colors; these do not encode collision energy.
         */
        struct Accent {
            static constexpr unsigned PRIMARY = 0x2383E2; /* Bright blue */
            static constexpr unsigned SOFT = 0xDDEBF8;    /* Pale blue */
            static constexpr unsigned SUCCESS = 0x0F7B6C; /* Dark teal / green */
        };
    };


    struct Hud {

        struct LogPolicy {
            static constexpr std::size_t ENTRIES = 80;
        };


        struct Movement {
            inline static constexpr float MIN_ZOOM_M = 4.f, MAX_ZOOM_M = 35.f;
            inline static constexpr float MIN_FADE_TIME_S = .5f, MAX_FADE_TIME_S = 30.f;
        };


        struct Layout {

            /* Logical-pixel spacing for the world bar only; other panel controls retain their own style. */
            struct Toolbar {
                static constexpr float PADDING_X = 14, BUTTON_PADDING_X = 12, BUTTON_PADDING_Y = 4;
                static constexpr float GAP = 8, GROUP_GAP = 20, ROUNDING = 7, OPACITY = .96f;
            };


            /**
             * Logical-pixel padding between related controls and neighbouring windows.
             */
            struct Spacing {
                static constexpr float MARGIN = 8, TOP = 60, GAP = 12;
            };


            /**
             * Initial sizes for the independent health and player-list windows, in logical pixels.
             */
            struct Status {
                static constexpr float HEALTH_WIDTH = 280, HEALTH_HEIGHT = 106;
                static constexpr float PLAYERS_HEIGHT = 150, BAR_HEIGHT = 20;
            };


            /**
             * Initial sizes for environment, character and movement windows, in logical pixels.
             */
            struct Settings {
                static constexpr float WIDTH = 280, TERRAIN_HEIGHT = 138, SKY_HEIGHT = 174;
                static constexpr float CHARACTER_HEIGHT = 430, MOVEMENT_HEIGHT = 300;
            };


            /**
             * Initial right-side chat and private-lobby sizes, in logical pixels.
             */
            struct Social {
                static constexpr float WIDTH = 320, CHAT_HEIGHT = 210, LOBBY_HEIGHT = 260;
            };


            /**
             * Initial bottom log-window size in logical pixels, independent of chat scrolling.
             */
            struct Log {
                static constexpr float WIDTH = 600, HEIGHT = 140;
            };
        };


        struct Chat {
            /* Leave room for the private-room action next to the lobby name input. */
            static constexpr float LOBBY_NAME_RESERVE = 65;
            static constexpr float COMPOSER_HEIGHT = 38, SEND_WIDTH = 52, MINIMUM_INPUT_WIDTH = 80;
            static constexpr float SCROLL_THRESHOLD = 30, COLOR_SIZE = 12;
        };
    };


    struct Font {

        struct Sizes {
            inline static constexpr float HINT_PX = 12, LABEL_PX = 13, BODY_PX = 14;
            inline static constexpr float PARAMETER_PX = 15, SECTION_PX = 16, TITLE_PX = 24;
        };


        struct Resources {
            inline static constexpr const char* FONT_DIRECTORY = "assets/fonts/";
            inline static constexpr int FONT_OVERSAMPLE = 2;
        };
    };


    struct Physics {

        struct Inspector {
            /* Window placement and row stride are logical pixels; row height also drives list clipping. */
            inline static constexpr float MARGIN = 8;
            inline static constexpr ImVec2 LOG_SIZE{320, 150};
            inline static constexpr ImVec2 BODIES_POSITION{324, 12}, BODIES_SIZE{300, 320};
            inline static constexpr float BODY_LIST_HEIGHT_FRACTION = .49f, BODY_ROW_HEIGHT = 58;
        };


        struct Controls {
            /* Logical-pixel dimensions for parameter labels, native controls and compact presentation mode. */
            inline static constexpr float LABEL_X = 108.f;
            inline static constexpr float COMBO_WIDTH = 170.f;

            /* Camera distances are metres and yaw is radians. The impact preset frames local contact regions. */
            inline static constexpr float CAMERA_DISTANCE_DEFAULT = 83.64f;
            inline static constexpr float CAMERA_DISTANCE_IMPACT = 22.f;
            inline static constexpr float CAMERA_YAW_DEFAULT = .7853982f;
            inline static constexpr float CAMERA_YAW_IMPACT = .28f;
            inline static constexpr float IMPACT_ZOOM_MIN = 10.f;
            inline static constexpr float DEFAULT_ZOOM_MIN = 41.82f;
            inline static constexpr float ZOOM_MAX = 334.56f;

            /* Heat legend dimensions in logical pixels; palette runs from weak blue to violent red. */
            inline static constexpr float HEAT_SEGMENT_WIDTH = 65.f;
            inline static constexpr float HEIGHT = 12.f;

            inline static constexpr ImU32 COLORS[] = {
                IM_COL32(4, 6, 122, 255),   /* Dark blue */
                IM_COL32(5, 161, 224, 255), /* Light blue */
                IM_COL32(46, 184, 71, 255), /* Green */
                IM_COL32(250, 199, 15, 255),/* Yellow / gold */
                IM_COL32(184, 4, 4, 255)    /* Red */
            };

            /* Menu order must agree with the corresponding scenario/shape selection switches. */
            inline static constexpr const char* NAMES[] = {
                "Matrix A / 850",
                "Matrix B / 1750",
                "Matrix C / 4000",
                "Custom / 120",
                "Impact strength test"
            };

            inline static constexpr const char* SHAPES[] = {"Sphere", "Cube", "Cylinder", "Cone"};
        };
    };
};

} // namespace settings
