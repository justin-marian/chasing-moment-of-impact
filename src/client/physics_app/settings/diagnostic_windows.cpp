/**
 * @file
 * Draw the simulation log and body inspector panels.
 *
 * Provides the filterable physics log and the scrollable body list used by
 * the inspector overlay. Panels are hidden during presentation mode or when
 * the overlay is disabled.
 */

#include "settings/world/constants.h"
#include "settings/world/ui_constants.h"

#include "client/physics_app/settings/diagnostic_windows.h"
#include "client/render_gl/controls.h"

#include <imgui.h>

namespace app::panels {
namespace {

constexpr const char* POSITION_FORMAT = "%6.2f %6.2f %6.2f";
constexpr const char* SPEED_FORMAT = "speed %6.2f m/s";

/**
 * Draw a single row in the body list showing index, position and speed.
 *
 * Row drawing needs the concrete font type (opaque via `auto` in the header),
 * so it stays internal to this translation unit.
 *
 * @param s     Viewer session containing body state and selection.
 * @param index Body index to draw.
 * @param fonts Font collection for mono-spaced numeric output.
 */
template <class Fonts>
void drawBodyRow(Session& s, int index, const Fonts& fonts) {
    ImGui::PushID(index);

    const std::string label = "Body " + std::to_string(index);
    if (ImGui::Selectable(label.c_str(), s.debug.selectedBody == index))
        s.debug.selectedBody = index;

    const auto& state = s.simulation.state;
    const auto position = state.position_m[index];

    ImGui::PushFont(fonts.mono);
    ImGui::TextColored(
        render::rgb(settings::World::Ui::Color::Text::SECONDARY),
        POSITION_FORMAT, double(position.x), double(position.y), double(position.z));
    ImGui::TextColored(
        render::rgb(settings::World::Ui::Color::Text::TERTIARY),
        SPEED_FORMAT, double(sim::length(state.velocity_mps[index])));
    ImGui::PopFont();

    ImGui::PopID();
}

} // namespace (anonymous)

/**
 * Draw the filterable simulation log window.
 *
 * @param s           Viewer session containing log messages and filter state.
 * @param displaySize Current ImGui display dimensions for anchoring.
 */
void drawSimulationLog(Session& s, ImVec2 displaySize) {
    const float x = settings::World::Ui::Physics::Inspector::MARGIN;
    const float y = displaySize.y - (
        settings::World::Ui::Physics::Inspector::LOG_SIZE.y +
        settings::World::Ui::Physics::Inspector::MARGIN
    );

    ImGui::SetNextWindowPos({x, y}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(settings::World::Ui::Physics::Inspector::LOG_SIZE, ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation log", &s.view.logWidget, ImGuiWindowFlags_NoSavedSettings);

    ImGui::InputTextWithHint("##filter", "Filter log", s.debug.filter, sizeof(s.debug.filter));

    for (const auto& message : s.view.messages) {
        if (matchesFilter(message, s.debug.filter))
            ImGui::TextWrapped("%s", message.c_str());
    }

    ImGui::End();
}

/**
 * Draw the scrollable body list window.
 *
 * @param s           Viewer session containing simulation state and renderer.
 * @param displaySize Current ImGui display dimensions for layout.
 */
void drawBodiesPanel(Session& s, ImVec2 displaySize) {
    ImGui::SetNextWindowPos(settings::World::Ui::Physics::Inspector::BODIES_POSITION, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(settings::World::Ui::Physics::Inspector::BODIES_SIZE, ImGuiCond_FirstUseEver);

    ImGui::Begin("Bodies", &s.view.rightPanel, ImGuiWindowFlags_NoSavedSettings);

    const auto& fonts = s.renderer->fonts();
    render::section("Bodies", fonts);

    const float childHeight = displaySize.y * settings::World::Ui::Physics::Inspector::BODY_LIST_HEIGHT_FRACTION;
    ImGui::BeginChild("list", {0, childHeight});

    ImGuiListClipper clipper;
    clipper.Begin(s.simulation.state.count_i, settings::World::Ui::Physics::Inspector::BODY_ROW_HEIGHT);

    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            drawBodyRow(s, i, fonts);
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

/**
 * Draw the inspector panels (simulation log and body list).
 *
 * @param s            Viewer session controlling panel visibility.
 * @param presentation True if the viewer is in presentation mode.
 */
void drawInspector(Session& s, bool presentation) {
    if (presentation || !s.view.overlayVisible)
        return;

    const auto displaySize = ImGui::GetIO().DisplaySize;
    if (s.view.logWidget)
        drawSimulationLog(s, displaySize);
    if (s.view.rightPanel)
        drawBodiesPanel(s, displaySize);
}

} // namespace app::panels
