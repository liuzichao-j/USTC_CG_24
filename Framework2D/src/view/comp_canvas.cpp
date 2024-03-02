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

/**
 * @brief 绘制画布的方法。
 *
 * 绘制画布的背景、图形和文件对话框。同时处理鼠标事件。
 */
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

/**
 * @brief 设置画布的位置和大小。
 *
 * @param min 画布的左上角坐标。
 * @param size 画布的大小。
 */
void Canvas::set_attributes(const ImVec2& min, const ImVec2& size)
{
    canvas_min_ = min;
    canvas_size_ = size;
    canvas_minimal_size_ = size;
    canvas_max_ =
        ImVec2(canvas_min_.x + canvas_size_.x, canvas_min_.y + canvas_size_.y);
}

/**
 * @brief 控制画布背景的可见性。
 *
 * @param flag 是否显示背景。
 */
void Canvas::show_background(bool flag)
{
    show_background_ = flag;
}

/**
 * @brief 设置画布的默认状态。
 */
void Canvas::set_default()
{
    draw_status_ = false;
    shape_type_ = kDefault;
}

/**
 * @brief 设置画布的绘制状态为直线。
 */
void Canvas::set_line()
{
    draw_status_ = false;
    shape_type_ = kLine;
}

/**
 * @brief 设置画布的绘制状态为矩形。
 */
void Canvas::set_rect()
{
    draw_status_ = false;
    shape_type_ = kRect;
}

/**
 * @brief 设置画布的绘制状态为椭圆。
 */
void Canvas::set_ellipse()
{
    draw_status_ = false;
    shape_type_ = kEllipse;
}

/**
 * @brief 设置画布的绘制状态为多边形。
 */
void Canvas::set_polygon()
{
    draw_status_ = false;
    shape_type_ = kPolygon;
}

/**
 * @brief 设置画布的绘制状态为自由绘制。
 */
void Canvas::set_freehand()
{
    draw_status_ = false;
    shape_type_ = kFreehand;
}

/**
 * @brief 撤销上一步的绘制。
 */
void Canvas::set_delete()
{
    if (!draw_status_ && !shape_list_.empty())
    {
        shape_list_.pop_back();
    }
    draw_status_ = false;
}

/**
 * @brief 重置画布。
 */
void Canvas::set_reset()
{
    draw_status_ = false;
    shape_list_.clear();
}

/**
 * @brief 插入图片。
 */
void Canvas::set_image()
{
    draw_status_ = false;
    shape_type_ = kImage;
    flag_open_file_dialog_ = true;
}

/**
 * @brief 选择图形。
 */
void Canvas::set_select()
{
    draw_status_ = false;
    current_shape_.reset();
    shape_type_ = kDefault;

    select_mode_ = true;
    if (selected_shape_)
    {
        selected_shape_->conf.time = 0;
    }
    selected_shape_.reset();
    selected_shape_index_ = -1;
}

/**
 * @brief 删除选中的图形。
 */
void Canvas::set_select_delete()
{
    if (selected_shape_ && selected_shape_index_ >= 0 &&
        selected_shape_index_ < shape_list_.size())
    {
        selected_shape_->conf.time = 0;
        selected_shape_.reset();
        shape_list_.erase(shape_list_.begin() + selected_shape_index_);
        selected_shape_index_ = -1;
    }
}

/**
 * @brief 设置画布处于绘制状态。
 */
void Canvas::set_draw()
{
    select_mode_ = false;
    if (selected_shape_)
    {
        selected_shape_->conf.time = 0;
    }
    selected_shape_.reset();
    selected_shape_index_ = -1;

    draw_status_ = false;
    current_shape_.reset();
    shape_type_ = kDefault;

    draw_color[0] = 1.0f;
    draw_color[1] = 0.0f;
    draw_color[2] = 0.0f;
    draw_color[3] = 1.0f;
    draw_thickness = 2.0f;
    draw_filled = false;
    image_size = 1;
    image_bia[0] = 0.5f;
    image_bia[1] = 0.5f;
}

/**
 * @brief 将选中的图形上移一层。
 */
void Canvas::set_goup()
{
    if (selected_shape_ && selected_shape_index_ < shape_list_.size() - 1)
    {
        std::swap(shape_list_[selected_shape_index_],
                  shape_list_[selected_shape_index_ + 1]);
        selected_shape_index_++;
    }
}

/**
 * @brief 将选中的图形下移一层。
 */
void Canvas::set_godown()
{
    if (selected_shape_ && selected_shape_index_ > 0)
    {
        std::swap(shape_list_[selected_shape_index_],
                  shape_list_[selected_shape_index_ - 1]);
        selected_shape_index_--;
    }
}

/**
 * @brief 清除画布上的所有图形。
 */
void Canvas::clear_shape_list()
{
    shape_list_.clear();
}

Canvas::ShapeType Canvas::get_shape_type() const
{
    if(!select_mode_)
    {
        return shape_type_;
    }
    else
    {
        if (selected_shape_)
        {
            return (Canvas::ShapeType)selected_shape_->get_shape_type();
        }
        else
        {
            return kDefault;
        }
    }
}

/**
 * @brief 绘制画布的背景。
 */
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

/**
 * @brief 绘制画布上的图形。
 */
void Canvas::draw_shapes()
{
    Shape::Config s = { .bias = { canvas_min_.x, canvas_min_.y } };
    // 传入画布的左上角坐标作为偏移量，设置图像大小。
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 对选中的元素，进行闪烁或缩放处理。
    if (selected_shape_)
    {
        selected_shape_->conf.time += 0.05f;
    }

    // ClipRect can hide the drawing content outside of the rectangular area
    draw_list->PushClipRect(canvas_min_, canvas_max_, true);
    for (const auto& shape : shape_list_)
    {
        shape->draw(s);
        // 对每一个图形使用相同的调用方式进行绘制。
    }
    if (draw_status_ && current_shape_)
    {
        current_shape_->draw(s);
    }
    draw_list->PopClipRect();
}

/**
 * @brief 绘制打开文件对话框。
 */
void Canvas::draw_open_image_file_dialog()
{
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.flags = ImGuiFileDialogFlags_Modal;
    // 使得文件对话框以弹出窗口的形式显示
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseImageOpenFileDlg", "Choose Image File", ".png,.jpg", config);
    if (ImGuiFileDialog::Instance()->Display("ChooseImageOpenFileDlg"))
    {
        // 此时用户已经选择了一个文件或者关闭了文件对话框
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

/**
 * @brief 处理鼠标左键点击事件。
 */
void Canvas::mouse_click_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (select_mode_)
    {
        if (selected_shape_)
        {
            selected_shape_->conf.time = 0;
            selected_shape_index_ = -1;
            selected_shape_.reset();
        }
        ImVec2 mouse_pos = mouse_pos_in_canvas();
        for (int i = (int)shape_list_.size() - 1; i >= 0; i--)
        {
            if (shape_list_[i]->is_select_on(mouse_pos.x, mouse_pos.y))
            {
                selected_shape_ = shape_list_[i];
                selected_shape_index_ = i;
                break;
            }
        }
        if (selected_shape_)
        {
            selected_shape_->conf.time = 0;
            for (int i = 0; i < 4; i++)
            {
                draw_color[i] = selected_shape_->conf.line_color[i] / 255.0f;
            }
            draw_thickness = selected_shape_->conf.line_thickness;
            draw_filled = selected_shape_->conf.filled;
            image_size = selected_shape_->conf.image_size;
            image_bia[0] = selected_shape_->conf.image_bia[0];
            image_bia[1] = selected_shape_->conf.image_bia[1];
        }
    }
    else if (!draw_status_)
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
            case USTC_CG::Canvas::kImage:
            {
                break;
            }
            default: break;
        }
    }
    else if (shape_type_ != kPolygon)
    {
        draw_status_ = false;
        if (current_shape_ && shape_type_ != kFreehand)
        {
            shape_list_.push_back(current_shape_);
            current_shape_.reset();
        }
    }
}

/**
 * @brief 处理鼠标右键点击事件。
 */
void Canvas::mouse_right_click_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (draw_status_)
    {
        draw_status_ = false;
        if (current_shape_ && shape_type_ == kPolygon)
        {
            // 对于多边形，右键点击表示结束绘制
            current_shape_->update(start_point_.x, start_point_.y);
            shape_list_.push_back(current_shape_);
            current_shape_.reset();
        }
    }
}

/**
 * @brief 处理鼠标移动事件。
 */
void Canvas::mouse_move_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (draw_status_)
    {
        end_point_ = mouse_pos_in_canvas();
        if (current_shape_)
        {
            // 使用update函数更新图形信息。更新图形的参数，包括线条颜色、线条粗细、填充。
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
    if (select_mode_)
    {
        if (selected_shape_)
        {
            // 更新图形的参数，包括线条颜色、线条粗细、填充，图片大小和偏移量。
            for (int i = 0; i < 4; i++)
            {
                selected_shape_->conf.line_color[i] =
                    (unsigned char)(draw_color[i] * 255);
            }
            selected_shape_->conf.line_thickness = draw_thickness;
            selected_shape_->conf.filled = draw_filled;
            selected_shape_->conf.image_size = image_size;
            selected_shape_->conf.image_bia[0] = image_bia[0];
            selected_shape_->conf.image_bia[1] = image_bia[1];
        }
    }
}

/**
 * @brief 处理鼠标释放事件。
 */
void Canvas::mouse_release_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (draw_status_ && current_shape_ && shape_type_ == kFreehand)
    {
        // 对于自由绘制，鼠标释放表示结束绘制
        draw_status_ = false;
        shape_list_.push_back(current_shape_);
        current_shape_.reset();
    }
}

/**
 * @brief 计算鼠标在画布中的相对位置。
 *
 * @return 鼠标在画布中的相对位置。
 */
ImVec2 Canvas::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse_pos_in_canvas(
        io.MousePos.x - canvas_min_.x, io.MousePos.y - canvas_min_.y);
    return mouse_pos_in_canvas;
}
}  // namespace USTC_CG