/**
 * @file
 * Draw simulation, spawning and camera controls, including scene navigation.
 *
 * The Simulation panel provides the backend selection, scenario switching,
 * world parameters, spawn controls and every collapsible section that belongs
 * to the physics side.
 *
 * One entry point. The panel draws itself with the session's current state
 * and mutates it in place. The panel's return value signals whether the
 * caller should switch to the world view this frame.
 */
#pragma once

#include "client/physics_app/run_session.h"

namespace app::panels {

/**
 * Draw the main Simulation panel.
 *
 * @param s            Session to read from and mutate.
 * @param paused       Pause flag, updated in place.
 * @param presentation Presentation-mode flag, updated in place.
 * @return True when the user asked to switch to the networked world view.
 */
bool drawSimulationPanel(Session& s, bool& paused, bool& presentation);

} // namespace app::panels