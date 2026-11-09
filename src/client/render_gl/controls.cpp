/**
 * @file
 * Reference-style slider decoration around the unmodified Dear ImGui SliderFloat.
 *
 * Input behavior: Omar Cornut and contributors, Dear ImGui v1.91.8, MIT.
 * https://github.com/ocornut/imgui/blob/v1.91.8/imgui_widgets.cpp
 *
 * The internal TempInputIsActive query only suppresses decoration while native
 * text input is visible. The project supplies the thin track, white circular
 * handle and separate readout; no backend is copied.
 */

#include "settings/world/constants.h"
#include "settings/world/ui_constants.h"
#include "client/render_gl/controls.h"

#include <algorithm>
#include <cstdio>

#include <imgui_internal.h>


namespace render {

namespace {

/**
 * Draw the custom visual track, fill, and handle for the slider.
 *
 * @param value Current scalar value of the slider.
 * @param low   Minimum allowed value.
 * @param high  Maximum allowed value.
 * @param scale Global UI scale factor.
 */
void drawSliderTrack(float value, float low, float high, float scale) {
    const auto minimum = ImGui::GetItemRectMin();
    const auto maximum = ImGui::GetItemRectMax();
    const float inset = settings::World::Ui::Slider::NATIVE_PADDING + settings::World::Ui::Slider::HANDLE_RADIUS * scale;

    const float left = minimum.x + inset;
    const float right = maximum.x - inset;

    const float y = (minimum.y + maximum.y) * .5f;
    const float ratio = std::clamp((value - low) / (high - low), 0.f, 1.f);

    const ImVec2 handle{left + (right - left) * ratio, y};
    auto& draw = *ImGui::GetWindowDrawList();

    /* Native hit testing retains the full row height; only the visible rail is thin. */
    draw.AddLine(
        {left, y}, {right, y}, ImGui::GetColorU32(rgb(settings::World::Ui::Slider::TRACK)),
        settings::World::Ui::Slider::TRACK_HEIGHT * scale
    );
    draw.AddLine(
        {left, y}, handle, ImGui::GetColorU32(rgb(settings::World::Ui::Slider::FILL)),
        settings::World::Ui::Slider::TRACK_HEIGHT * scale
    );

    /* A quiet outline and one-pixel shadow keep the small white handle legible on the rail. */
    draw.AddCircleFilled(
        {handle.x, handle.y + scale}, (settings::World::Ui::Slider::HANDLE_RADIUS + .5f) * scale,
        ImGui::GetColorU32(rgb(settings::World::Ui::Slider::SHADOW, .12f))
    );
    draw.AddCircleFilled(
        handle, settings::World::Ui::Slider::HANDLE_RADIUS * scale,
        ImGui::GetColorU32(rgb(settings::World::Ui::Slider::HANDLE))
    );
    draw.AddCircle(
        handle, settings::World::Ui::Slider::HANDLE_RADIUS * scale,
        ImGui::GetColorU32(rgb(settings::World::Ui::Slider::OUTLINE))
    );
}

} // namespace (anonymous)


/**
 * Draw a text section header with padding and a separator.
 *
 * @param name  Text to display.
 * @param fonts Font collection used for rendering.
 */
void section(const char* name, const render::Fonts& fonts) {
    ImGui::Dummy({0, settings::World::Ui::Control::Section::BEFORE});
    ImGui::PushFont(fonts.section);
    ImGui::TextUnformatted(name);
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Dummy({0, settings::World::Ui::Control::Section::AFTER});
}


/**
 * Draw a standard button with optional primary accent styling.
 *
 * @param name    Button label.
 * @param width   Requested button width.
 * @param primary Whether to use the primary accent color.
 * @return True if the button was clicked.
 */
bool button(const char* name, float width, bool primary) {
    if (primary) {
        ImGui::PushStyleColor(ImGuiCol_Text, render::rgb(settings::World::Ui::Color::Accent::PRIMARY));
        ImGui::PushStyleColor(ImGuiCol_Border, render::rgb(settings::World::Ui::Color::Accent::PRIMARY));
    }

    const float available = ImGui::GetContentRegionAvail().x;
    const bool clicked = ImGui::Button(name, {std::min(width, available), settings::World::Ui::Control::Button::HEIGHT});
    if (primary) {
        ImGui::PopStyleColor(2);
    }

    return clicked;
}


/**
 * Draw a custom-decorated slider with a label, track, and numeric readout.
 *
 * @param name   Display label.
 * @param value  Pointer to the scalar value to modify.
 * @param low    Minimum allowed value.
 * @param high   Maximum allowed value.
 * @param format Printf-style format string for the numeric readout.
 * @param unit   Optional unit string appended to the readout.
 * @param fonts  Font collection used for rendering.
 * @return True if the value was changed.
 */
bool slider(
    const char* name, float* value,
    float low, float high,
    const char* format, const char* unit,
    const Fonts& fonts
) {
    constexpr float MAX_LABEL_FRACTION = .42f;
    constexpr float MAX_VALUE_FRACTION = .28f;

    const float scale = ImGui::GetIO().FontGlobalScale;

    const float available = ImGui::GetContentRegionAvail().x;
    const float labelWidth = std::min(settings::World::Ui::Slider::LABEL_WIDTH * scale, available * MAX_LABEL_FRACTION);
    const float valueWidth = std::min(settings::World::Ui::Slider::VALUE_WIDTH * scale, available * MAX_VALUE_FRACTION);

    const float gap = settings::World::Ui::Slider::GAP * scale;
    const float trackWidth = std::max(1.f, available - labelWidth - valueWidth - 2 * gap);
    const float origin = ImGui::GetCursorPosX();

    ImGui::PushID(name);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, settings::World::Ui::Slider::VERTICAL_PADDING * scale});

    ImGui::PushFont(fonts.parameterLabel ? fonts.parameterLabel : fonts.label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(name);
    ImGui::PopFont();

    ImGui::SameLine(origin + labelWidth + gap);
    ImGui::SetNextItemWidth(trackWidth);

    /* Keep the native scalar widget for mouse, keyboard, focus and numeric editing. */
    const bool editing = ImGui::TempInputIsActive(ImGui::GetID("##value"));
    ImGui::PushFont(fonts.parameterValue ? fonts.parameterValue : fonts.mono);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 2 * settings::World::Ui::Slider::HANDLE_RADIUS * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

    for (auto color : {
        ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive,
        ImGuiCol_SliderGrab, ImGuiCol_SliderGrabActive
    }) { ImGui::PushStyleColor(color, ImVec4(0, 0, 0, 0)); }

    ImGui::PushStyleColor(ImGuiCol_Text, editing ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(0, 0, 0, 0));

    const bool changed = ImGui::SliderFloat("##value", value, low, high, format, ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(2);

    if (!ImGui::TempInputIsActive(ImGui::GetItemID())) {
        drawSliderTrack(*value, low, high, scale);
    }

    /* The value has its own column, so a small value cannot be covered by the handle. */
    char number[settings::World::Ui::Slider::FORMAT_CAPACITY];
    std::snprintf(number, sizeof(number), format, double(*value));

    ImGui::SameLine(origin + labelWidth + gap + trackWidth + gap);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(rgb(settings::World::Ui::Color::Text::SECONDARY), "%s%s%s", number, unit[0] ? " " : "", unit);

    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopID();

    return changed;
}

} // namespace render