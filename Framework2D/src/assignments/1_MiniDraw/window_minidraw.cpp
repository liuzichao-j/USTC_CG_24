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
        // 按钮的大小
        ImVec2 button_size = ImVec2(0.08f * ImGui::GetColumnWidth(), 0);

        if (!p_canvas_->select_mode)  // 绘制模式下的UI
        {
            // Select button
            if (ImGui::Button("Select", button_size))
            {
                std::cout << "Set shape to Select" << std::endl;
                p_canvas_->set_select();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.1f * ImGui::GetColumnWidth());
            ImGui::Text("        ");
            ImGui::SameLine();

            // Buttons for shape types
            if (ImGui::Button("Line", button_size))
            {
                std::cout << "Set shape to Line" << std::endl;
                p_canvas_->set_line();
            }
            ImGui::SameLine();
            if (ImGui::Button("Rectangle", button_size))
            {
                std::cout << "Set shape to Rect" << std::endl;
                p_canvas_->set_rect();
            }

            // HW1_TODO: More primitives
            //    - Ellipse
            //    - Polygon
            //    - Freehand(optional)

            ImGui::SameLine();
            if (ImGui::Button("Ellipse", button_size))
            {
                std::cout << "Set shape to Ellipse" << std::endl;
                p_canvas_->set_ellipse();
            }
            ImGui::SameLine();
            if (ImGui::Button("Polygon", button_size))
            {
                std::cout << "Set shape to Polygon" << std::endl;
                p_canvas_->set_polygon();
            }
            ImGui::SameLine();
            if (ImGui::Button("Freehand", button_size))
            {
                std::cout << "Set shape to Freehand" << std::endl;
                p_canvas_->set_freehand();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.1f * ImGui::GetColumnWidth());
            ImGui::Text("        ");
            ImGui::SameLine();
            if (ImGui::Button("Delete", button_size))
            {
                std::cout << "Delete once" << std::endl;
                p_canvas_->set_delete();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset", button_size))
            {
                std::cout << "Reset once" << std::endl;
                p_canvas_->set_reset();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.1f * ImGui::GetColumnWidth());
            ImGui::Text("        ");
            ImGui::SameLine();
            if (ImGui::Button("Image", button_size))
            {
                std::cout << "Plugin Image" << std::endl;
                p_canvas_->set_image();
            }

            // Canvas component
            ImGui::Text(
                "Press left mouse to add shapes. Right Click to stop ongoing "
                "paint or Complete a Polygon. ");
            ImGui::Text(
                "Press Shift to draw square in Rectangle mode and draw circle "
                "in Ellipse mode. ");
        }
        else  // 选择模式下的UI
        {
            // Draw button and delete button
            if (ImGui::Button("Draw", button_size))
            {
                std::cout << "Draw mode" << std::endl;
                p_canvas_->set_draw();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.1f * ImGui::GetColumnWidth());
            ImGui::Text("        ");
            ImGui::SameLine();
            if (ImGui::Button("Delete", button_size))
            {
                std::cout << "Delete selected shape" << std::endl;
                p_canvas_->set_select_delete();
            }

            // Go up and go down button only when there is a selected shape
            if (p_canvas_->get_shape_type() != Canvas::ShapeType::kDefault)
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(0.1f * ImGui::GetColumnWidth());
                ImGui::Text("        ");
                ImGui::SameLine();
                if (ImGui::Button("Go up", button_size))
                {
                    std::cout << "Go up once" << std::endl;
                    p_canvas_->set_goup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Go down", button_size))
                {
                    std::cout << "Go down once" << std::endl;
                    p_canvas_->set_godown();
                }
            }

            // Canvas component
            ImGui::Text("Selete one item on the canvas, which will sparkle. ");
            ImGui::Text(
                "You can change shape as before. You can change the place and "
                "size of the selected image.");
        }

        // 对形状对象，设置颜色
        if (p_canvas_->get_shape_type() != Canvas::ShapeType::kImage)
        {
            ImGui::SetNextItemWidth(0.4f * ImGui::GetColumnWidth());
            ImGui::ColorEdit4(
                "", p_canvas_->draw_color, ImGuiColorEditFlags_PickerHueWheel);
        }
        else
        {
            // 对图像对象，设置颜色为默认值
            p_canvas_->draw_color[0] = 1.0f;
            p_canvas_->draw_color[1] = 0.0f;
            p_canvas_->draw_color[2] = 0.0f;
            p_canvas_->draw_color[3] = 1.0f;
        }

        // 设置粗细，如果填充了则隐藏该项
        if (p_canvas_->get_shape_type() != Canvas::ShapeType::kImage &&
            !p_canvas_->draw_filled)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.025f * ImGui::GetColumnWidth());
            ImGui::Text("     ");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.4f * ImGui::GetColumnWidth());
            ImGui::DragFloat(
                "Thickness",
                &p_canvas_->draw_thickness,
                0.05f,
                1.0f,
                FLT_MAX / INT_MAX,
                "%.3f",
                ImGuiSliderFlags_AlwaysClamp);
        }
        else
        {
            p_canvas_->draw_thickness = 2.0f;
        }

        // 是否填充的设置仅对矩形和椭圆有效，此时显示填充选项，粗细设为默认值
        if (p_canvas_->get_shape_type() == Canvas::ShapeType::kRect ||
            p_canvas_->get_shape_type() == Canvas::ShapeType::kEllipse)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.025f * ImGui::GetColumnWidth());
            ImGui::Text("     ");
            ImGui::SameLine();
            ImGui::Checkbox("Filled", &p_canvas_->draw_filled);
        }
        else
        {
            p_canvas_->draw_filled = false;
        }

        // 对图像对象，若选中，设置图像大小和位置
        if (p_canvas_->select_mode &&
            p_canvas_->get_shape_type() == Canvas::ShapeType::kImage)
        {
            ImGui::SetNextItemWidth(0.4f * ImGui::GetColumnWidth());
            ImGui::DragFloat(
                "Size of Image",
                &p_canvas_->image_size,
                0.01f,
                0.0f,
                FLT_MAX / INT_MAX,
                "%.3f",
                ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.025f * ImGui::GetColumnWidth());
            ImGui::Text("     ");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(0.4f * ImGui::GetColumnWidth());
            ImGui::DragFloat2(
                "Place of Image ( x , y )",
                p_canvas_->image_bia,
                0.01f,
                0.0f,
                1.0f);
        }
        else
        {
            // 对于其他形状对象，设置图像大小和位置为默认值
            p_canvas_->image_size = 1.0f;
            p_canvas_->image_bia[0] = 0.5f;
            p_canvas_->image_bia[1] = 0.5f;
        }

        ImGui::Text(
            "Press Alt to drag more accurate. Press Shift otherwisely. To "
            "enter value, press Ctrl or double click. ");

        // Set the canvas to fill the rest of the window
        const auto& canvas_min = ImGui::GetCursorScreenPos();
        const auto& canvas_size = ImGui::GetContentRegionAvail();
        p_canvas_->set_attributes(canvas_min, canvas_size);
        p_canvas_->draw();

        ImGui::End();
    }
}
}  // namespace USTC_CG