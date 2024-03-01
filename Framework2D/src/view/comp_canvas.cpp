#include "view/comp_canvas.h"

#include <ImGuiFileDialog.h>

#include <cmath>
#include <iostream>

#include "imgui.h"
#include "view/shapes/ellipse.h"
#include "view/shapes/freehand.h"
#include "view/shapes/images.h"
#include "view/shapes/line.h"
#include "view/shapes/polygon.h"
#include "view/shapes/rect.h"

namespace USTC_CG
{
void Canvas::draw()
{
    draw_background();

    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        mouse_click_event();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        mouse_right_click_event();
    mouse_move_event();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        mouse_release_event();

    draw_shapes();

    if (flag_open_file_dialog_)
    {
        draw_open_image_file_dialog();
    }
}

void Canvas::set_attributes(const ImVec2& min, const ImVec2& size)
{
    canvas_min_ = min;
    canvas_size_ = size;
    canvas_minimal_size_ = size;
    canvas_max_ =
        ImVec2(canvas_min_.x + canvas_size_.x, canvas_min_.y + canvas_size_.y);
}

void Canvas::show_background(bool flag)
{
    show_background_ = flag;
}

void Canvas::set_default()
{
    draw_status_ = false;
    shape_type_ = kDefault;
}

void Canvas::set_line()
{
    draw_status_ = false;
    shape_type_ = kLine;
}

void Canvas::set_rect()
{
    draw_status_ = false;
    shape_type_ = kRect;
}

void Canvas::set_ellipse()
{
    draw_status_ = false;
    shape_type_ = kEllipse;
}

void Canvas::set_polygon()
{
    draw_status_ = false;
    shape_type_ = kPolygon;
}

void Canvas::set_freehand()
{
    draw_status_ = false;
    shape_type_ = kFreehand;
}

void Canvas::set_undo()
{
    if (!draw_status_ && !shape_list_.empty())
    {
        shape_list_.pop_back();
    }
    draw_status_ = false;
}

void Canvas::set_reset()
{
    draw_status_ = false;
    shape_list_.clear();
}

void Canvas::set_image()
{
    draw_status_ = false;
    shape_type_ = kImage;
    flag_open_file_dialog_ = true;
}

void Canvas::clear_shape_list()
{
    shape_list_.clear();
}

void Canvas::draw_background()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (show_background_)
    {
        // Draw background recrangle
        draw_list->AddRectFilled(canvas_min_, canvas_max_, background_color_);
        // Draw background border
        draw_list->AddRect(canvas_min_, canvas_max_, border_color_);
    }
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(canvas_min_);
    ImGui::InvisibleButton(
        label_.c_str(), canvas_size_, ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    is_hovered_ = ImGui::IsItemHovered();
    is_active_ = ImGui::IsItemActive();
}

void Canvas::draw_shapes()
{
    Shape::Config s = { .bias = { canvas_min_.x, canvas_min_.y } };
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // ClipRect can hide the drawing content outside of the rectangular area
    draw_list->PushClipRect(canvas_min_, canvas_max_, true);
    for (const auto& shape : shape_list_)
    {
        shape->draw(s);
    }
    if (draw_status_ && current_shape_)
    {
        current_shape_->draw(s);
    }
    draw_list->PopClipRect();
}

void Canvas::draw_open_image_file_dialog()
{
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseImageOpenFileDlg", "Choose Image File", ".png,.jpg", config);
    if (ImGuiFileDialog::Instance()->Display("ChooseImageOpenFileDlg"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePathName =
                ImGuiFileDialog::Instance()->GetFilePathName();
            std::string label = filePathName;
            current_shape_ =
                std::make_shared<Images>(filePathName, canvas_size_);
        }
        ImGuiFileDialog::Instance()->Close();

        flag_open_file_dialog_ = false;
        if (current_shape_)
        {
            shape_list_.push_back(current_shape_);
            shape_type_ = kDefault;
            current_shape_.reset();
        }
    }
}

void Canvas::mouse_click_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (!draw_status_)
    {
        draw_status_ = true;
        start_point_ = end_point_ = mouse_pos_in_canvas();
        switch (shape_type_)
        {
            case USTC_CG::Canvas::kDefault:
            {
                break;
            }
            case USTC_CG::Canvas::kLine:
            {
                current_shape_ = std::make_shared<Line>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kRect:
            {
                current_shape_ = std::make_shared<Rect>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kEllipse:
            {
                current_shape_ = std::make_shared<Ellipse>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kPolygon:
            {
                current_shape_ = std::make_shared<Polygon>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kFreehand:
            {
                current_shape_ =
                    std::make_shared<Freehand>(start_point_.x, start_point_.y);
                break;
            }
            default: break;
        }
    }
    else
    {
        if (shape_type_ == kPolygon)
        {
            end_point_ = mouse_pos_in_canvas();
            current_shape_->addpoint(end_point_.x, end_point_.y);
        }
        else if (shape_type_ != kImage)
        {
            draw_status_ = false;
            if (current_shape_ && shape_type_ != kFreehand)
            {
                shape_list_.push_back(current_shape_);
                current_shape_.reset();
            }
        }
    }
}

void Canvas::mouse_right_click_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (draw_status_)
    {
        draw_status_ = false;
        if (current_shape_ && shape_type_ == kPolygon)
        {
            current_shape_->addpoint(end_point_.x, end_point_.y);
            current_shape_->addpoint(start_point_.x, start_point_.y);
            shape_list_.push_back(current_shape_);
            current_shape_.reset();
        }
    }
}

void Canvas::mouse_move_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (draw_status_)
    {
        end_point_ = mouse_pos_in_canvas();
        if (current_shape_ && shape_type_ != kImage)
        {
            for (int i = 0; i < 4; i++)
            {
                current_shape_->conf.line_color[i] =
                    (unsigned char)(draw_color[i] * 255);
            }
            current_shape_->conf.line_thickness = draw_thickness;
            current_shape_->conf.filled = draw_filled;
            current_shape_->update(end_point_.x, end_point_.y);
        }
    }
}

void Canvas::mouse_release_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (draw_status_ && current_shape_ && shape_type_ == kFreehand)
    {
        draw_status_ = false;
        shape_list_.push_back(current_shape_);
        current_shape_.reset();
    }
}

ImVec2 Canvas::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse_pos_in_canvas(
        io.MousePos.x - canvas_min_.x, io.MousePos.y - canvas_min_.y);
    return mouse_pos_in_canvas;
}
}  // namespace USTC_CG