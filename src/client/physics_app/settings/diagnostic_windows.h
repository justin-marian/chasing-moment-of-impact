/**
 * @file
 * Draw read-only simulation diagnostics, performance and body inspection controls.
 *
 * The Bodies and log inspector provides a virtualized list of every body
 * with its position and speed, plus a filterable message log.
 */
#pragma once

#include "client/physics_app/run_session.h"

#include <imgui.h>
#include <string>

namespace app::panels {

/**
 * Check whether a log message contains the active filter substring.
 *
 * @param message The log message to test.
 * @param filter  The active filter string.
 * @return True when the message contains the filter as a substring.
 */
inline bool matchesFilter(const std::string& message, const char* filter) {
    return message.find(filter) != std::string::npos;
}

/**
 * Draw the filterable message log window.
 * Caller is responsible for checking s.view.logWidget.
 *
 * @param s           Session containing the log messages and filter state.
 * @param displaySize Current ImGui display dimensions for anchoring.
 */
void drawSimulationLog(Session& s, ImVec2 displaySize);

/**
 * Draw the virtualized body list window.
 * Caller is responsible for checking s.view.rightPanel.
 *
 * @param s           Session containing the simulation state and renderer.
 * @param displaySize Current ImGui display dimensions for layout.
 */
void drawBodiesPanel(Session& s, ImVec2 displaySize);

/**
 * Draw the inspector when the session has it enabled.
 * The panel is a no-op when neither rightPanel nor presentation is set.
 *
 * @param s            Session to read and mutate (selectedBody is written).
 * @param presentation Presentation flag; suppresses the panel when set.
 */
void drawInspector(Session& s, bool presentation);

} // namespace app::panels
