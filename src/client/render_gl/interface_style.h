/**
 * @file
 * Apply the shared restrained Dear ImGui style and expose fonts without owning scene state.
 */

#pragma once

#include <imgui.h>


namespace render {

/**
 * Convert packed RGB and alpha to an ImGui color.
 *
 * @param hex   Packed non-linear RGB color in 0xRRGGBB format.
 * @param alpha Opacity in the unit range, passed through with the decoded RGB components.
 * @return Decoded RGB components and the requested alpha in an ImGui color.
 */
inline ImVec4 rgb(unsigned hex, float alpha = 1.f) {
    constexpr float MAX_COLOR_VALUE = 255.f;

    return {
        float((hex >> 16) & 255) / MAX_COLOR_VALUE,
        float((hex >> 8) & 255) / MAX_COLOR_VALUE,
        float(hex & 255) / MAX_COLOR_VALUE,
        alpha
    };
}


/**
 * Shared typography roles loaded from supplied font assets.
 */
struct Fonts {
    ImFont *body{}, *label{}, *section{}, *title{}, *mono{}, *hint{};

    /* Larger parameter readouts and semibold player names share the existing font families. */
    ImFont *parameterLabel{}, *parameterValue{}, *username{};
};


/**
 * Apply a restrained light theme based on the upstream Dear ImGui palette.
 */
void applyInterfaceStyle();


/**
 * Begin the compact command strip.
 *
 * Pair with ImGui::End().
 *
 * @param title Window title and persistent ImGui identity for this panel.
 */
void beginToolbar(const char* title);


/**
 * Begin a movable/resizable window with a deterministic first-use position.
 *
 * @param title    Window title and persistent ImGui identity for this panel.
 * @param position Initial top-left window position in logical screen pixels.
 * @param size     Initial window width and height in logical screen pixels.
 * @param open     Optional visibility flag; closing the window clears it when supplied.
 * @return True when the window is expanded and its contents should be drawn.
 */
bool beginWindow(const char* title, ImVec2 position, ImVec2 size, bool* open = nullptr);

} // namespace render
