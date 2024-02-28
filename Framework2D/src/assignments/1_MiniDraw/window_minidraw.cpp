#include "window_minidraw.h"

#include <iostream>

namespace USTC_CG
{
MiniDraw::MiniDraw(const std::string& window_name) : Window(window_name)
{
    p_canvas_ = std::make_shared<Canvas>("Cmpt.Canvas");
}

MiniDraw::~MiniDraw()
{
}

void MiniDraw::draw()
{
    draw_canvas();
}

void MiniDraw::draw_canvas()
{
    // Set a full screen canvas view
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    if (ImGui::Begin(
            "Canvas",
            &flag_show_canvas_view_,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
    {
        // Buttons for shape types
        if (ImGui::Button("Line"))
        {
            std::cout << "Set shape to Line" << std::endl;
            p_canvas_->set_line();
        }
        ImGui::SameLine();
        if (ImGui::Button("Rect"))
        {
            std::cout << "Set shape to Rect" << std::endl;
            p_canvas_->set_rect();
        }

        // HW1_TODO: More primitives
        //    - Ellipse
        //    - Polygon
        //    - Freehand(optional)

        ImGui::SameLine();
        if (ImGui::Button("Ellipse"))
        {
            std::cout << "Set shape to Ellipse" << std::endl;
            p_canvas_->set_ellipse();
        }
        ImGui::SameLine();
        if (ImGui::Button("Polygon"))
        {
            std::cout << "Set shape to Polygon" << std::endl;
            p_canvas_->set_polygon();
        }
        ImGui::SameLine();
        if (ImGui::Button("Freehand"))
        {
            std::cout << "Set shape to Freehand" << std::endl;
            p_canvas_->set_freehand();
        }
        ImGui::SameLine();
        if (ImGui::Button("Undo"))
        {
            std::cout << "Undo once" << std::endl;
            p_canvas_->set_undo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            std::cout << "Reset once" << std::endl;
            p_canvas_->set_reset();
        }

        // Canvas component
        ImGui::Text(
            "Press left mouse to add shapes. Right Click to stop ongoing paint "
            "or Complete a Polygon. ");
        ImGui::Text("Press Shift to draw circle in Ellipse mode. ");

        ImGui::ColorEdit4("Choose Paint Color", p_canvas_->draw_color);
        ImGui::SliderFloat(
            "Choose Paint Thickness", &p_canvas_->draw_thickness, 0.0f, 10.0f);

        // Set the canvas to fill the rest of the window
        const auto& canvas_min = ImGui::GetCursorScreenPos();
        const auto& canvas_size = ImGui::GetContentRegionAvail();
        p_canvas_->set_attributes(canvas_min, canvas_size);
        p_canvas_->draw();
    }
    ImGui::End();
}
}  // namespace USTC_CG