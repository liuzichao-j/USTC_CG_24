#include "comp_warping.h"

#include <cmath>

#include "warping_fisheye.h"
#include "warping_idw.h"
#include "warping_rbf.h"

namespace USTC_CG
{
using uchar = unsigned char;

/**
 * @brief Construct a new CompWarping object
 * Init, Stored in Father Class ImageEditor, Backup in CompWarping
 * @param label The label of the image
 * @param filename The filename of the image
 */
CompWarping::CompWarping(const std::string& label, const std::string& filename)
    : ImageEditor(label, filename)
{
    if (data_)
    {
        back_up_ = std::make_shared<Image>(*data_);
    }
}

void CompWarping::draw()
{
    // Draw the image, not about the toolbar
    ImageEditor::draw();
    // Draw the canvas
    if (flag_enable_selecting_points_)
    {
        select_points();
    }
}

/**
 * @brief Invert the color of the image
 * i.e., (r, g, b) -> (255 - r, 255 - g, 255 - b)
 */
void CompWarping::invert()
{
    // Invert the color, change both the hue and the brightness
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            data_->set_pixel(
                i,
                j,
                { static_cast<uchar>(255 - color[0]),
                  static_cast<uchar>(255 - color[1]),
                  static_cast<uchar>(255 - color[2]) });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
/**
 * @brief Mirror the image
 * Simply just swap the pixels
 * @param is_horizontal Whether to mirror horizontally
 * @param is_vertical Whether to mirror vertically
 */
void CompWarping::mirror(bool is_horizontal, bool is_vertical)
{
    Image image_tmp(*data_);
    int width = data_->width();
    int height = data_->height();

    if (is_horizontal)
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i,
                        j,
                        image_tmp.get_pixel(width - 1 - i, height - 1 - j));
                }
            }
        }
        else
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(width - 1 - i, j));
                }
            }
        }
    }
    else
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(i, height - 1 - j));
                }
            }
        }
    }

    // After change the image, we should reload the image data to the renderer
    update();
}

/**
 * @brief Convert the image to gray scale
 * i.e., (r, g, b) -> (gray, gray, gray), of which gray = (r + g + b) / 3
 * Other algorithms: Gray = R*0.299 + G*0.587 + B*0.114
 */
void CompWarping::gray_scale()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            uchar gray_value = (color[0] + color[1] + color[2]) / 3;
            data_->set_pixel(i, j, { gray_value, gray_value, gray_value });
            // uchar gray_value = color[0] * 0.299 + color[1] * 0.587 + color[2]
            // * 0.114;

            // data_->set_pixel(i, j, { gray_value, gray_value,
            // gray_value });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}

/**
 * @brief Set the warping method by defining the category of warping_
 * @param method The method to set, 0 for FishEye, 1 for IDW, 2 for RBF
 */
void CompWarping::set_warping_method(int method)
{
    switch (method)
    {
        case 1: warping_ = std::make_shared<WarpingIDW>(); break;
        case 2: warping_ = std::make_shared<WarpingRBF>(); break;
        default: warping_ = std::make_shared<WarpingFishEye>(); break;
    }
}

/**
 * @brief Apply warping function to the image
 * @param Inverse_Flag Whether to use the inverse warping function
 */
void CompWarping::warping()
{
    // HW2_TODO: You should implement your own warping function that interpolate
    // the selected points.
    // You can design a class for such warping operations, utilizing the
    // encapsulation, inheritance, and polymorphism features of C++. More files
    // like "*.h", "*.cpp" can be added to this directory or anywhere you like.

    // Create a new image to store the result
    Image warped_image(*data_);
    // Initialize the color of result image
    for (int y = 0; y < data_->height(); ++y)
    {
        for (int x = 0; x < data_->width(); ++x)
        {
            warped_image.set_pixel(x, y, { 0, 0, 0 });
        }
    }
    // Apply warping function and store the result in warped_image
    warping_->warping(
        data_, warped_image, start_points_, end_points_, inverse_flag);
    *data_ = std::move(warped_image);
    update();
}

void CompWarping::restore()
{
    *data_ = *back_up_;
    update();
    if(start_points_.size() > 0)
    {
        enable_selecting(true);
    }
}

void CompWarping::enable_selecting(bool flag)
{
    flag_enable_selecting_points_ = flag;
}

/**
 * @brief Select points for warping
 * Drag the mouse to select the start and end points
 */
void CompWarping::select_points()
{
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    bool is_hovered_ = ImGui::IsItemHovered();
    // Selections
    ImGuiIO& io = ImGui::GetIO();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        draw_status_ = true;
        start_ = end_ =
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
    }
    if (draw_status_)
    {
        end_ = ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))  // Drag finished
        {
            start_points_.push_back(start_);
            end_points_.push_back(end_);
            draw_status_ = false;
        }
    }
    // Visualization
    auto draw_list = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < start_points_.size(); ++i)
    {
        ImVec2 s(
            start_points_[i].x + position_.x, start_points_[i].y + position_.y);
        ImVec2 e(
            end_points_[i].x + position_.x, end_points_[i].y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
        draw_list->AddCircleFilled(e, 4.0f, IM_COL32(0, 255, 0, 255));
    }
    if (draw_status_)
    {
        ImVec2 s(start_.x + position_.x, start_.y + position_.y);
        ImVec2 e(end_.x + position_.x, end_.y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
    }
}

void CompWarping::init_selections()
{
    start_points_.clear();
    end_points_.clear();
}
}  // namespace USTC_CG