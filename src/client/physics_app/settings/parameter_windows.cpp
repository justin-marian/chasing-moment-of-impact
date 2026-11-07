/**
 * @file
 * Draw the simulation, spawn, physics settings, and debug parameter windows.
 *
 * Provides the main inspector panels, including simulation backend selection,
 * scenario switching, physics parameters, spawn controls, and debug views.
 */
#include "shared/utils/command.h"

#include "settings/world/constants.h"
#include "settings/world/ui_constants.h"

#include "physics/scenarios/impact_demo.h"

#include "client/physics_app/settings/parameter_windows.h"
#include "client/render_gl/controls.h"

#include <algorithm>
#include <initializer_list>
#include <cmath>
#include <filesystem>

#include <imgui.h>

namespace app::panels {

namespace {

/**
 * Fixed UI layout metrics.
 *
 * Button widths accommodate their longest expected labels so adjacent
 * controls align cleanly. Heat map values size the impact gradient widget.
 */
constexpr float BUTTON_WIDTH_STEP = 130.f;
constexpr float BUTTON_WIDTH_EXPORT = 134.f;
constexpr float BUTTON_WIDTH_CLEAR_IMPACTS = 166.f;
constexpr float BUTTON_WIDTH_BACKEND = 78.f;
constexpr float BUTTON_WIDTH_ACTION = 85.f;
constexpr float BUTTON_WIDTH_SPAWN = 272.f;

constexpr int HEAT_SEGMENT_COUNT = 4;
constexpr float HEAT_PADDING = 4.f;

/**
 * Description of a single slider row.
 */
struct SliderRow {
    const char* label;
    float* value;
    float minimum, maximum;
    const char* format;
    const char* unit;
};

/**
 * Draw a sequence of slider rows.
 *
 * @param fonts Font collection used for rendering the sliders.
 * @param rows  Initializer list of slider configurations to draw.
 */
void drawSliderRows(const render::Fonts& fonts, std::initializer_list<SliderRow> rows) {
    for (const auto& row : rows)
        render::slider(row.label, row.value, row.minimum, row.maximum, row.format, row.unit, fonts);
}

/**
 * Place the next widget on the same row as the previous one.
 */
void keepActionsOnOneRow() {
    ImGui::SameLine();
}

/**
 * Align the next widget with the parameter value column.
 */
void alignValueWithLabel() {
    ImGui::SameLine(settings::World::Ui::Physics::Controls::LABEL_X);
}

/**
 * Draw contact response sliders and clamp dynamic friction.
 *
 * @param session Viewer session containing the world state.
 * @param fonts   Font collection used for rendering.
 */
void drawContactResponse(Session& session, const render::Fonts& fonts) {
    auto& world = session.scene.world;
    drawSliderRows(fonts, 
        {{"Restitution", &world.restitution_unitless, 0, 1, "%.2f", ""},
        {"Friction static", &world.frictionStatic_unitless, 0, 1, "%.2f", ""}}
    );

    /* Compute this range after static friction has been edited in this same frame. */
    world.frictionDynamic_unitless = std::min(world.frictionDynamic_unitless, world.frictionStatic_unitless);
    render::slider("Friction kinetic", &world.frictionDynamic_unitless, 0, world.frictionStatic_unitless + .0001f, "%.2f", "", fonts);
}

/**
 * Draw solver iteration and feature toggle controls.
 *
 * @param session Viewer session containing the world state.
 * @param fonts   Font collection used for rendering.
 */
void drawSolverControls(Session& session, const render::Fonts& fonts) {
    auto& world = session.scene.world;
    float passes = float(world.solverIterations_i);
    if (render::slider("Solver passes", &passes, 1, 60, "%.0f", "", fonts))
        world.solverIterations_i = int(std::round(passes));
    ImGui::Checkbox("Sleeping", &world.enableSleep);
    ImGui::Checkbox("Sphere / wall CCD", &world.enableCcd);
}

/**
 * Draw single-step and motion export buttons.
 *
 * @param session Viewer session to step or export.
 */
void drawStepAndExport(Session& session) {
    if (render::button("Step once", BUTTON_WIDTH_STEP))
        stepSimulation(session);

    keepActionsOnOneRow();

    if (render::button("Export motion", BUTTON_WIDTH_EXPORT)) {
        std::filesystem::create_directories("generated/reports/viewer");
        sim::printMotion("generated/reports/viewer/motion.csv", session.simulation.state);
        session.log("Saved generated/reports/viewer/motion.csv");
    }
}

/**
 * Draw the physics settings collapsible header and its contents.
 *
 * @param session Viewer session containing the world state.
 * @param fonts   Font collection used for rendering.
 */
void physicsSettings(Session& session, const render::Fonts& fonts) {
    /* Contact response, solver switches and single-step/export actions retain their original order. */
    if (!ImGui::CollapsingHeader("Physics settings"))
        return;

    drawContactResponse(session, fonts);
    drawSolverControls(session, fonts);
    drawStepAndExport(session);
}

/**
 * Draw the impact energy gradient and recent impact statistics.
 *
 * @param fonts   Font collection used for rendering.
 * @param impacts Impact history to display.
 */
void drawImpactPalette(const render::Fonts& fonts, const render::ImpactHistory& impacts) {
    auto* draw = ImGui::GetWindowDrawList();
    const auto p = ImGui::GetCursorScreenPos();

    for (int i = 0; i < HEAT_SEGMENT_COUNT; ++i) {
        draw->AddRectFilledMultiColor(
            {p.x + float(i) * settings::World::Ui::Physics::Controls::HEAT_SEGMENT_WIDTH, p.y},
            {p.x + float(i + 1) * settings::World::Ui::Physics::Controls::HEAT_SEGMENT_WIDTH, p.y + settings::World::Ui::Physics::Controls::HEAT_HEIGHT},
            settings::World::Ui::Physics::Controls::COLORS[i],
            settings::World::Ui::Physics::Controls::COLORS[i + 1],
            settings::World::Ui::Physics::Controls::COLORS[i + 1],
            settings::World::Ui::Physics::Controls::COLORS[i]
        );
    }

    ImGui::Dummy({
        settings::World::Ui::Physics::Controls::HEAT_SEGMENT_WIDTH * float(HEAT_SEGMENT_COUNT),
        settings::World::Ui::Physics::Controls::HEAT_HEIGHT + HEAT_PADDING
    });
    ImGui::TextUnformatted("Gentle Violent");

    ImGui::PushFont(fonts.mono);
    ImGui::Text("Last hit %.2f J / %.2f m/s", double(impacts.lastEnergy_J), double(impacts.lastClosing_mps));
    ImGui::Text("Normal impulse %.2f N s", double(impacts.lastImpulse_Ns));
    ImGui::PopFont();
}

/**
 * Draw the view and debug collapsible header and its contents.
 *
 * @param s     Viewer session containing view state and impact history.
 * @param fonts Font collection used for rendering.
 */
void viewDebug(Session& s, const render::Fonts& fonts) {
    if (!ImGui::CollapsingHeader("View & debug"))
        return;

    render::slider("Orbit", &s.view.camera.yaw, -3.14f, 3.14f, "%.2f", "", fonts);
    render::slider("Elevation", &s.view.camera.pitch, -.4f, 1.2f, "%.2f", "", fonts);
    render::slider(
        "Zoom", &s.view.camera.distance, s.scene.id == "impact" ? 
        settings::World::Ui::Physics::Controls::IMPACT_ZOOM_MIN : 
        settings::World::Ui::Physics::Controls::DEFAULT_ZOOM_MIN,
        settings::World::Ui::Physics::Controls::ZOOM_MAX, 
        "%.0f", "m", fonts
    );

    ImGui::Checkbox("AABB", &s.debug.aabb);
    keepActionsOnOneRow();
    ImGui::Checkbox("OBB", &s.debug.obb);
    ImGui::Checkbox("Velocity", &s.debug.velocity);

    ImGui::SeparatorText("Collision impacts");

    auto& impacts = s.impacts();
    ImGui::Checkbox("Impact heat coloring", &impacts.enabled);
    ImGui::Checkbox("Keep impact marks", &impacts.hold);
    render::slider("Fade time", &impacts.lifetime_s, .5f, 30, "%.1f", "s", fonts);
    render::slider("Red at energy", &impacts.redEnergy_J, .1f, 100, "%.1f", "J", fonts);

    if (render::button("Clear impact marks", BUTTON_WIDTH_CLEAR_IMPACTS))
        impacts.clear();

    /* Same five palette stops used by the impact shader. */
    drawImpactPalette(fonts, impacts);
}

/**
 * Switch the active physics backend and upload the current simulation state.
 *
 * @param s       Viewer session to update.
 * @param backend Name of the backend to select.
 */
void selectBackend(Session& s, const char* backend) {
    if (backend == s.backendName)
        return;

    auto next = std::string_view(backend) == "cuda" ? sim::createCudaBackend() : sim::createCpuBackend();

    next->upload(s.simulation.state);
    s.backend = std::move(next);
    s.backendName = backend;
    s.log(std::string(backend == std::string_view("cuda") ? "CUDA" : "CPU") + " backend selected");
}

/**
 * Map a scenario identifier to its combo box index.
 *
 * @param id Scenario identifier string.
 * @return Zero-based combo box index for the scenario.
 */
int scenarioIndex(const std::string& id) {
    switch (utils::commandKey(id)) {
        case utils::commandKey("A"): return 0;
        case utils::commandKey("B"): return 1;
        case utils::commandKey("C"): return 2;
        case utils::commandKey("impact"): return 4;
        default: return 3;
    }
}

/**
 * Map a combo box index to its scenario identifier.
 *
 * @param index Zero-based combo box index.
 * @return Scenario identifier string.
 */
const char* scenarioId(int index) {
    switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 4: return "impact";
        default: return "demo";
    }
}

/**
 * Draw simulation backend, scenario, seed, and playback controls.
 *
 * @param s      Viewer session containing simulation state.
 * @param paused Playback state modified by the controls.
 * @param fonts  Font collection used for rendering.
 */
void drawSimulationControls(Session& s, bool& paused, const render::Fonts& fonts) {
    render::section("Simulation", fonts);

    ImGui::TextUnformatted("Backend");
    alignValueWithLabel();

    if (render::button("CPU", BUTTON_WIDTH_BACKEND, s.backendName == "cpu"))
        selectBackend(s, "cpu");

    keepActionsOnOneRow();
    ImGui::BeginDisabled(!sim::hasCudaDevice());

    if (render::button("CUDA", BUTTON_WIDTH_BACKEND, s.backendName == "cuda"))
        selectBackend(s, "cuda");

    ImGui::EndDisabled();

    int selected = scenarioIndex(s.scene.id);
    ImGui::SetNextItemWidth(settings::World::Ui::Physics::Controls::COMBO_WIDTH);

    if (ImGui::Combo("Scenario", &selected, settings::World::Ui::Physics::Controls::NAMES, 5)) {
        s.scene = viewerScenario(scenarioId(selected), std::uint64_t(s.spawn.seedInput));

        const bool impact = s.scene.id == "impact";
        s.view.camera.distance = impact ? 
            settings::World::Ui::Physics::Controls::CAMERA_DISTANCE_IMPACT : 
            settings::World::Ui::Physics::Controls::CAMERA_DISTANCE_DEFAULT;
        s.view.camera.yaw = impact ? 
            settings::World::Ui::Physics::Controls::CAMERA_YAW_IMPACT : 
            settings::World::Ui::Physics::Controls::CAMERA_YAW_DEFAULT;

        resetSession(s);
    }

    ImGui::SetNextItemWidth(settings::World::Ui::Physics::Controls::COMBO_WIDTH);
    ImGui::InputInt("Seed", &s.spawn.seedInput, 0, 0);
    s.spawn.seedInput = std::max(0, s.spawn.seedInput);

    if (render::button("Run", BUTTON_WIDTH_ACTION, true))
        paused = false;

    keepActionsOnOneRow();

    if (render::button("Pause", BUTTON_WIDTH_ACTION))
        paused = true;

    keepActionsOnOneRow();

    if (render::button("Reset", BUTTON_WIDTH_ACTION))
        resetSession(s);
}

/**
 * Draw global world physics sliders.
 *
 * @param s     Viewer session containing the world state.
 * @param fonts Font collection used for rendering.
 */
void drawWorldSettings(Session& s, const render::Fonts& fonts) {
    render::section("World", fonts);

    auto& world = s.scene.world;

    render::slider("Gravity", &world.gravity_mps2, 0, 20, "%.2f", "", fonts);

    float dt = world.dt_s * 1000;
    if (render::slider("Time step", &dt, 4, 33, "%.0f", "ms", fonts)) {
        world.dt_s = std::round(dt) / 1000;
        s.simulation.accumulator = 0;
    }

    float steps = float(s.spawn.substeps);
    if (render::slider("Substeps", &steps, 1, 8, "%.0f", "", fonts))
        s.spawn.substeps = int(std::round(steps));

    render::slider("Linear drag", &world.linearDamping_per_s, 0, .5f, "%.2f", "", fonts);
    render::slider("Angular drag", &world.angularDamping_per_s, 0, .5f, "%.2f", "", fonts);
}

/**
 * Draw object spawning controls and handle spawn actions.
 *
 * @param s     Viewer session containing spawn state and the backend.
 * @param fonts Font collection used for rendering.
 */
void drawSpawnControls(Session& s, const render::Fonts& fonts) {
    render::section("Spawn", fonts);

    ImGui::SetNextItemWidth(settings::World::Ui::Physics::Controls::COMBO_WIDTH);
    ImGui::Combo("Shape", &s.spawn.shape, settings::World::Ui::Physics::Controls::SHAPES, 4);

    float count = float(s.spawn.count);
    if (render::slider("Count", &count, 1, 2500, "%.0f", "", fonts))
        s.spawn.count = int(std::round(count));

    if (render::button("Spawn", BUTTON_WIDTH_SPAWN, true)) {
        const int first = s.simulation.state.count_i;

        for (int i = 0; i < s.spawn.count; ++i)
            if (!sim::spawnRandom(s.simulation.state, static_cast<sim::Shape>(s.spawn.shape), s.scene, s.spawn.generator))
                break;

        s.backend->upload(s.simulation.state);
        s.log("Spawned " + std::to_string(s.simulation.state.count_i - first) + " bodies");
    }
}

/**
 * Draw the physics widgets popup menu.
 *
 * @param session Viewer session containing view state.
 */
void widgetMenu(Session& session) {
    auto& view = session.view;
    if (!ImGui::BeginPopup("Physics widgets"))
        return;

    ImGui::MenuItem("Simulation", nullptr, &view.simulationWidget);
    ImGui::MenuItem("Spawn objects", nullptr, &view.spawnWidget);
    ImGui::MenuItem("Physics settings", nullptr, &view.physicsWidget);
    ImGui::MenuItem("Camera & impact display", nullptr, &view.cameraWidget);
    ImGui::MenuItem("Performance", nullptr, &view.performanceWidget);
    ImGui::MenuItem("Bodies", nullptr, &view.rightPanel);
    ImGui::MenuItem("Session log", nullptr, &view.logWidget);

    render::slider(
        "Text scale", &ImGui::GetIO().FontGlobalScale, settings::World::Ui::Window::MINIMUM_TEXT_SCALE,
        settings::World::Ui::Window::MAXIMUM_TEXT_SCALE, "%.1f", "x", session.renderer->fonts()
    );
    ImGui::EndPopup();
}

/**
 * Draw the physics toolbar and handle its actions.
 *
 * @param session Viewer session to act upon.
 * @param paused  Playback state modified by the toolbar.
 * @return True if the shared world button was clicked.
 */
bool toolbar(Session& session, bool& paused) {
    render::beginToolbar("Physics toolbar");

#ifdef SIM_BUILD_MMO
    const bool world = ImGui::SmallButton("Shared world");
    keepActionsOnOneRow();
#else
    const bool world = false;
#endif

    if (ImGui::SmallButton(paused ? "Run" : "Pause"))
        paused = !paused;

    keepActionsOnOneRow();

    if (ImGui::SmallButton("Reset"))
        resetSession(session);

    keepActionsOnOneRow();

    if (ImGui::SmallButton("Widgets"))
        ImGui::OpenPopup("Physics widgets");

    widgetMenu(session);
    keepActionsOnOneRow();

    ImGui::TextDisabled("%s / %d bodies / F1 hide", session.backendName.c_str(), session.simulation.state.count_i);

    ImGui::End();
    return world;
}

/**
 * Draw a parameter window and handle its visibility state.
 *
 * @param title    Window title.
 * @param position Initial window position.
 * @param size     Initial window size.
 * @param visible  Visibility flag bound to the window close button.
 * @param draw     Callback invoked to draw the window contents.
 */
template <class Draw>
void drawParameterWindow(const char* title, ImVec2 position, ImVec2 size, bool& visible, Draw draw) {
    if (!visible)
        return;
    if (render::beginWindow(title, position, size, &visible))
        draw();
    ImGui::End();
}

/**
 * Draw performance metrics.
 *
 * @param session Viewer session containing the performance data.
 */
void performance(Session& session) {
    ImGui::Text("Physics  %.2f ms", session.simulation.result.total_wall_ms);
    ImGui::Text("Render   %.2f ms", session.renderer->renderTime_ms());
    ImGui::Text("Contacts %d", session.simulation.result.diagnostics.contacts_i);
}

} // namespace (anonymous)

/**
 * Draw the main simulation parameter panels and toolbar.
 *
 * @param s            Viewer session containing all UI and simulation state.
 * @param paused       Playback state modified by the panels.
 * @param presentation True if presentation mode is active.
 * @return True if the shared world button was clicked.
 */
bool drawSimulationPanel(Session& s, bool& paused, bool& presentation) {
    /* Draw the existing physics windows and apply their immediate control actions. */
    auto& view = s.view;

    if (ImGui::IsKeyPressed(ImGuiKey_F1))
        view.overlayVisible = !view.overlayVisible;

    if (presentation || !view.overlayVisible)
        return false;

    const auto& fonts = s.renderer->fonts();
    const auto display = ImGui::GetIO().DisplaySize;
    const bool openSession = toolbar(s, paused);

    /* Ordered windows retain their original initial positions and immediate updates. */
    drawParameterWindow("Simulation", {8, 60}, {300, 270}, view.simulationWidget, [&] { drawSimulationControls(s, paused, fonts); });
    drawParameterWindow("Spawn objects", {8, 342}, {300, 230}, view.spawnWidget, [&] { drawSpawnControls(s, fonts); });
    drawParameterWindow("Physics settings", {320, 60}, {300, 380}, view.physicsWidget, [&] { drawWorldSettings(s, fonts); ImGui::SetNextItemOpen(true, ImGuiCond_Once); physicsSettings(s, fonts); });
    drawParameterWindow("Camera & impact display", {display.x - 308, 60}, {300, 360}, view.cameraWidget, [&] { ImGui::SetNextItemOpen(true, ImGuiCond_Once); viewDebug(s, fonts); });
    drawParameterWindow("Performance", {display.x - 208, 60}, {200, 108}, view.performanceWidget, [&] { performance(s); });

    return openSession;
}

} // namespace app::panels
