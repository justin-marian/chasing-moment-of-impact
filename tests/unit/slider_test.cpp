/**
 * @file
 * Exercise the decorated native slider without requiring an OpenGL window.
 *
 * Drives the control through simulated mouse and keyboard input and verifies
 * drag selection, Ctrl-click numeric entry, keyboard commit and range
 * clamping without ever creating a rendering context.
 */

#include "settings/world/constants.h"
#include "settings/world/ui_constants.h"
#include "client/render_gl/controls.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

#include <imgui_internal.h>

namespace {

constexpr float SLIDER_MIN = 0.f;
constexpr float SLIDER_MAX = 10.f;

/**
 * Screen-space description of the slider track used to aim synthetic input.
 */
struct Track {
    float left, right, y;
    ImGuiID id;
};

/**
 * Begin, draw and end one control frame, then return the track geometry the
 * slider laid out inside it.
 *
 * @param value Value bound to the slider; dragged or typed edits land here.
 * @return Track bounds and slider id in screen space for the next input event.
 */
Track frame(float& value) {
    constexpr const char* SLIDER_FORMAT = "%.2f";

    ImGui::NewFrame();
    ImGui::SetNextWindowPos({20, 20});
    ImGui::SetNextWindowSize({400, 160});
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    const auto cursor = ImGui::GetCursorScreenPos();
    const float available = ImGui::GetContentRegionAvail().x;
    const float width =
        available - settings::World::Ui::Slider::LABEL_WIDTH -
        settings::World::Ui::Slider::VALUE_WIDTH -
        2 * settings::World::Ui::Slider::GAP;
    const float inset =
        settings::World::Ui::Slider::NATIVE_PADDING +
        settings::World::Ui::Slider::HANDLE_RADIUS;

    ImGui::PushID("Strength");
    const auto id = ImGui::GetID("##value");
    ImGui::PopID();

    const float left =
        cursor.x +
        settings::World::Ui::Slider::LABEL_WIDTH +
        settings::World::Ui::Slider::GAP +
        inset;
    const float rowCenter =
        cursor.y +
        (ImGui::GetFontSize() + 2 * settings::World::Ui::Slider::VERTICAL_PADDING) / 2;
    const Track track{left, left + width - 2 * inset, rowCenter, id};

    render::slider("Strength", &value, SLIDER_MIN, SLIDER_MAX, SLIDER_FORMAT, "", {});

    ImGui::End();
    ImGui::Render();
    return track;
}


/**
 * Abort the current test when the condition is false.
 *
 * @param condition Checked precondition; a false value throws.
 * @param message   Diagnostic message attached to the thrown exception.
 */
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}


/**
 * Ctrl-click the middle of the slider, select all, type a replacement value
 * and commit it with Enter, advancing one frame per input stage.
 *
 * @param value Slider value edited by the simulated input.
 * @param track Screen-space geometry of the slider being edited.
 * @param text  Characters delivered to the native numeric input field.
 */
void typeValue(float& value, const Track& track, const char* text) {
    auto& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    io.AddMousePosEvent((track.left + track.right) / 2, track.y);
    io.AddMouseButtonEvent(0, true);
    frame(value);

    require(ImGui::TempInputIsActive(track.id), "Ctrl-click must open native numeric editing");
    io.AddMouseButtonEvent(0, false);
    io.AddKeyEvent(ImGuiKey_A, true);
    frame(value);

    io.AddKeyEvent(ImGuiKey_A, false);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    io.AddInputCharactersUTF8(text);
    frame(value);

    io.AddKeyEvent(ImGuiKey_Enter, true);
    frame(value);

    io.AddKeyEvent(ImGuiKey_Enter, false);
    frame(value);
}

} // namespace (anonymous)


/**
 * Entry point: build a headless ImGui context, drive the slider through
 * drag, numeric entry and clamping, and report failures through the exit
 * status.
 *
 * @return Zero when every slider behaviour holds, one otherwise.
 */
int main() {
    constexpr float DISPLAY_WIDTH = 800.f;
    constexpr float DISPLAY_HEIGHT = 400.f;
    constexpr float DELTA_TIME = 1.f / 60.f;

    constexpr float INITIAL_VALUE = 5.f;
    constexpr float NEAR_END = .1f;
    constexpr float VALUE_TOLERANCE = .001f;
    constexpr float TYPED_VALUE = 3.25f;

    ImGui::CreateContext();
    try {
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigInputTrickleEventQueue = false;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.DisplaySize = {DISPLAY_WIDTH, DISPLAY_HEIGHT};
        io.DeltaTime = DELTA_TIME;
        io.Fonts->AddFontDefault();

        unsigned char* pixels{};
        int width{}, height{};
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        float value = INITIAL_VALUE;
        const auto track = frame(value);
        frame(value);

        io.AddMousePosEvent(track.left, track.y);
        io.AddMouseButtonEvent(0, true);
        frame(value);
        require(value < SLIDER_MIN + NEAR_END, "Mouse press must select the minimum");

        io.AddMousePosEvent(track.right, track.y);
        frame(value);
        require(value > SLIDER_MAX - NEAR_END, "Dragging must reach the maximum");

        io.AddMouseButtonEvent(0, false);
        frame(value);

        typeValue(value, track, "3.25");
        require(std::abs(value - TYPED_VALUE) < VALUE_TOLERANCE, "Numeric entry must preserve the typed value");

        typeValue(value, track, "999");
        require(value == SLIDER_MAX, "Numeric entry must respect the supported range");

        std::cout << "PASS slider dragging, Ctrl-click editing, keyboard commit and clamping\n";
        ImGui::DestroyContext();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        ImGui::DestroyContext();
        return 1;
    }
}
