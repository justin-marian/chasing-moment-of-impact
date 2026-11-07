/**
 * @file
 * Apply the shared restrained Dear ImGui style and manage panel windows.
 *
 * Provides the global ImGui theme configuration and helper wrappers for
 * creating toolbars and bounded floating windows without owning scene state.
 */

#include "settings/world/constants.h"
#include "settings/world/ui_constants.h"
#include "client/render_gl/interface_style.h"

#include <algorithm>


namespace render {

/**
 * Apply the default Dear ImGui light palette and override it with project settings.
 *
 * @see Dear ImGui's original light palette: Omar Cornut and contributors, MIT.
 *      https://github.com/ocornut/imgui/blob/v1.91.8/imgui_draw.cpp
 */
void applyInterfaceStyle() {
    ImGui::StyleColorsLight();

    auto& style = ImGui::GetStyle();
    
    style.WindowPadding = {
        settings::World::Ui::Style::Spacing::WINDOW, 
        settings::World::Ui::Style::Spacing::WINDOW
    };
    style.FramePadding = {
        settings::World::Ui::Style::Spacing::FRAME_X, 
        settings::World::Ui::Style::Spacing::FRAME_Y
    };
    style.ItemSpacing = {
        settings::World::Ui::Style::Spacing::ITEM_X, 
        settings::World::Ui::Style::Spacing::ITEM_Y
    };
    
    style.WindowRounding = settings::World::Ui::Style::Shape::WINDOW_RADIUS;
    
    style.FrameRounding = settings::World::Ui::Style::Shape::CONTROL_RADIUS;
    style.PopupRounding = settings::World::Ui::Style::Shape::CONTROL_RADIUS;
    style.GrabRounding = settings::World::Ui::Style::Shape::CONTROL_RADIUS;
    
    style.WindowBorderSize = settings::World::Ui::Style::Shape::BORDER_WIDTH;
    style.FrameBorderSize = settings::World::Ui::Style::Shape::BORDER_WIDTH;
    
    style.GrabMinSize = settings::World::Ui::Style::Shape::GRAB_SIZE;

    auto& colors = style.Colors;
    colors[ImGuiCol_WindowBg] = rgb(
        settings::World::Ui::Color::Surface::PANEL, 
        settings::World::Ui::Style::Shape::WINDOW_ALPHA
    );
    colors[ImGuiCol_Text] = rgb(settings::World::Ui::Color::Text::PRIMARY);
    colors[ImGuiCol_TextDisabled] = rgb(settings::World::Ui::Color::Text::SECONDARY);
    colors[ImGuiCol_Border] = rgb(settings::World::Ui::Color::Surface::BORDER_SOFT);
    colors[ImGuiCol_FrameBg] = rgb(settings::World::Ui::Color::Surface::PAGE);
    colors[ImGuiCol_FrameBgHovered] = rgb(settings::World::Ui::Color::Accent::SOFT);
    colors[ImGuiCol_FrameBgActive] = rgb(settings::World::Ui::Color::Accent::SOFT);
    
    colors[ImGuiCol_Button] = rgb(
        settings::World::Ui::Color::Accent::PRIMARY, 
        settings::World::Ui::Style::Feedback::NORMAL_ALPHA
    );
    colors[ImGuiCol_ButtonHovered] = rgb(
        settings::World::Ui::Color::Accent::PRIMARY, 
        settings::World::Ui::Style::Feedback::HOVER_ALPHA
    );
    colors[ImGuiCol_ButtonActive] = rgb(
        settings::World::Ui::Color::Accent::PRIMARY, 
        settings::World::Ui::Style::Feedback::ACTIVE_ALPHA
    );
    
    colors[ImGuiCol_CheckMark] = rgb(settings::World::Ui::Color::Accent::PRIMARY);
    colors[ImGuiCol_SliderGrab] = rgb(settings::World::Ui::Color::Accent::PRIMARY);
}


/**
 * Begin the compact command strip.
 *
 * Pair with ImGui::End().
 *
 * @param title Window title and persistent ImGui identity for this panel.
 */
void beginToolbar(const char* title) {
    const auto display = ImGui::GetIO().DisplaySize;
    
    ImGui::SetNextWindowPos({
        settings::World::Ui::Window::MARGIN, 
        settings::World::Ui::Window::MARGIN
    });
    
    ImGui::SetNextWindowSize({
        display.x - 2 * settings::World::Ui::Window::MARGIN, 
        settings::World::Ui::Window::TOOLBAR_HEIGHT
    });
    
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        {
            settings::World::Ui::Window::MARGIN, 
            settings::World::Ui::Window::MARGIN
        }
    );
    
    ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();
}


/**
 * Begin a movable/resizable window with a deterministic first-use position.
 *
 * @param title    Window title and persistent ImGui identity for this panel.
 * @param position Initial top-left window position in logical screen pixels.
 * @param size     Initial window width and height in logical screen pixels.
 * @param open     Optional visibility flag; closing the window clears it when supplied.
 * @return True when the window is expanded and its contents should be drawn.
 */
bool beginWindow(const char* title, ImVec2 position, ImVec2 size, bool* open) {
    const auto display = ImGui::GetIO().DisplaySize;
    
    const ImVec2 maximum{
        std::max(
            settings::World::Ui::Window::MINIMUM_WIDTH, 
            display.x - 2 * settings::World::Ui::Window::MARGIN
        ),
        std::max(
            settings::World::Ui::Window::MINIMUM_HEIGHT,
            display.y - settings::World::Ui::Window::FIRST - settings::World::Ui::Window::MARGIN
        )
    };

    size.x = std::clamp(size.x, settings::World::Ui::Window::MINIMUM_WIDTH, maximum.x);
    size.y = std::clamp(size.y, settings::World::Ui::Window::MINIMUM_HEIGHT, maximum.y);

    position.x = std::clamp(
        position.x, 
        settings::World::Ui::Window::MARGIN,
        std::max(
            settings::World::Ui::Window::MARGIN, 
            display.x - size.x - settings::World::Ui::Window::MARGIN
        )
    );
    
    position.y = std::clamp(
        position.y, 
        settings::World::Ui::Window::FIRST,
        std::max(
            settings::World::Ui::Window::FIRST, 
            display.y - size.y - settings::World::Ui::Window::MARGIN
        )
    );

    ImGui::SetNextWindowSizeConstraints(
        {
            settings::World::Ui::Window::MINIMUM_WIDTH, 
            settings::World::Ui::Window::MINIMUM_HEIGHT
        }, 
        maximum
    );
    
    ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
    
    return ImGui::Begin(title, open, ImGuiWindowFlags_NoSavedSettings);
}

} // namespace render
