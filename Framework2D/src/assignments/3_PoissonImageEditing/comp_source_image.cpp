#include "comp_source_image.h"

#include <algorithm>
#include <cmath>

namespace USTC_CG
{
/**
 * @brief Construct a new CompSourceImage::CompSourceImage object. Set
 * selected_region_ as a binary image (0 or 255) as the same size as the image.
 */
CompSourceImage::CompSourceImage(
    const std::string& label,
    const std::string& filename)
    : ImageEditor(label, filename)
{
    if (data_)
    {
        selected_region_ =
            std::make_shared<Image>(data_->width(), data_->height(), 1);
    }
}

/**
 * @brief Draw the source image and the selected region.
 */
void CompSourceImage::draw()
{
    // Draw the image
    ImageEditor::draw();
    // Draw selected region
    if (flag_enable_selecting_region_)
    {
        select_region();
    }
}

/**
 * @brief Enable or disable the region selection.
 * @param flag True for enabling, false for disabling.
 */
void CompSourceImage::enable_selecting(bool flag)
{
    flag_enable_selecting_region_ = flag;
}

/**
 * @brief Set the region type for selection.
 * @param type The type of region to be selected.
 */
void CompSourceImage::set_region_type(RegionType type)
{
    region_type_ = type;
}

/**
 * @brief Select the region in the source image.
 */
void CompSourceImage::select_region()
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
    // HW3_TODO(optional): You can add more shapes for region selection. You can
    // also consider using the implementation in HW1. (We use rectangle for
    // example)
    ImGuiIO& io = ImGui::GetIO();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        draw_status_ = true;
        start_ = end_ = ImVec2(
            std::clamp<float>(
                io.MousePos.x - position_.x, 0, (float)image_width_),
            std::clamp<float>(
                io.MousePos.y - position_.y, 0, (float)image_height_));
    }
    if (draw_status_)
    {
        end_ = ImVec2(
            std::clamp<float>(
                io.MousePos.x - position_.x, 0, (float)image_width_),
            std::clamp<float>(
                io.MousePos.y - position_.y, 0, (float)image_height_));
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            draw_status_ = false;
            // Update the selected region.

            // HW3_TODO(optional): For other types of closed shapes, the most
            // important part in region selection is to find the interior pixels
            // of the region.
            // We give an example of rectangle here.

            // For polygon or freehand regions, you should inplement the
            // "scanning line" algorithm, which is a well-known algorithm in CG.

            for (int i = 0; i < selected_region_->width(); ++i)
            {
                for (int j = 0; j < selected_region_->height(); ++j)
                {
                    selected_region_->set_pixel(i, j, { 0 });
                }
            }
            switch (region_type_)
            {
                case USTC_CG::CompSourceImage::kDefault: break;
                case USTC_CG::CompSourceImage::kRect:
                {
                    for (int i = static_cast<int>(start_.x);
                         i < static_cast<int>(end_.x);
                         ++i)
                    {
                        for (int j = static_cast<int>(start_.y);
                             j < static_cast<int>(end_.y);
                             ++j)
                        {
                            selected_region_->set_pixel(i, j, { 255 });
                            // Select the region by setting the pixel value to
                            // 255.
                        }
                    }
                    break;
                }
                default: break;
            }
            init_id(region_type_);
            init_matrix();
        }
    }

    // Visualization
    auto draw_list = ImGui::GetWindowDrawList();
    ImVec2 s(start_.x + position_.x, start_.y + position_.y);
    ImVec2 e(end_.x + position_.x, end_.y + position_.y);

    switch (region_type_)
    {
        case USTC_CG::CompSourceImage::kDefault: break;
        case USTC_CG::CompSourceImage::kRect:
        {
            if (e.x > s.x && e.y > s.y)
            {
                draw_list->AddRect(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
            }
            break;
        }
        default: break;
    }
}

/**
 * @brief Get the selected region.
 * @return The Image selected_region_ with binary values (0 or 255).
 */
std::shared_ptr<Image> CompSourceImage::get_region()
{
    return selected_region_;
}

/**
 * @brief Get original image data.
 * @return The Image data_.
 */
std::shared_ptr<Image> CompSourceImage::get_data()
{
    return data_;
}

/**
 * @brief Get the start position of the source image.
 * @return The start position of the source image, in the form of ImVec2.
 */
ImVec2 CompSourceImage::get_position() const
{
    return start_;
}

/**
 * @brief Initialize the selected region by giving every point an id.
 * @param type The type of region to be selected.
 */
void CompSourceImage::init_id(RegionType type)
{
    point_to_id_.clear();
    id_to_point_.clear();
    point_to_id_.resize(selected_region_->width());
    for (int i = 0; i < selected_region_->width(); i++)
    {
        point_to_id_[i].resize(selected_region_->height());
    }
    int id = 1;

    switch (type)
    {
        case kRect:
            for (int i = 0; i < selected_region_->width(); i++)
            {
                for (int j = 0; j < selected_region_->height(); j++)
                {
                    if (selected_region_->get_pixel(i, j)[0] > 0)
                    {
                        point_to_id_[i][j] = id;
                        id_to_point_.push_back(ImVec2((float)i, (float)j));
                        id++;
                    }
                }
            }
            break;

        default: break;
    }
}

/**
 * @brief Get the id of a point in the selected region.
 * @param point The position of the point.
 * @return The id of the point, starting from 1. 0 means the point is not in the
 * selected region.
 */
int CompSourceImage::get_id(ImVec2 point)
{
    if (point.x >= 0 && point.x < selected_region_->width() && point.y >= 0 &&
        point.y < selected_region_->height())
    {
        return point_to_id_[(int)point.x][(int)point.y];
    }
    return 0;
}

/**
 * @brief Get the position of an id in the selected region.
 * @param id The id of the point.
 * @return The position of the point. If the id is invalid, return (-1, -1).
 */
ImVec2 CompSourceImage::get_point(int id)
{
    if (id < 0 || id >= id_to_point_.size())
    {
        return ImVec2(-1, -1);
    }
    return id_to_point_[id];
}

/**
 * @brief Get the number of points in the selected region.
 * @return The number of points in the selected region.
 */
int CompSourceImage::get_point_num()
{
    return (int)id_to_point_.size();
}

/**
 * @brief Initialize matrix A in advance.
 */
void CompSourceImage::init_matrix()
{
    // We have formula: 4*f_p - Sum_{q in N(p) and Omega} f_q = Sum_{q
    // in N(p) and NonOmega} f_q + 4*g_p - Sum_{q in N(p)} g_q, where
    // g_p is the source image and f_p is the target image.

    // Things in the left side of the equation are variables.

    int point_num = id_to_point_.size();

    // Sparse matrix A and Triplet to build A.
    A_.resize(point_num, point_num);
    // Each point has 5 coefficients at most.
    std::vector<Eigen::Triplet<float>> triplet;
    triplet.reserve(point_num * 5);

    for (int i = 0; i < point_num; i++)
    {
        // Make the formula coefficient No. i
        int src_x = (int)id_to_point_[i].x;
        int src_y = (int)id_to_point_[i].y;

        // Count the number of neighbors
        int np = 0;

        // For each neighbor of the point
        for (int j = -1; j <= 1; j++)
        {
            for (int k = -1; k <= 1; k++)
            {
                if ((abs(j) + abs(k) != 1) || src_x + j < 0 ||
                    src_x + j >= data_->width() || src_y + k < 0 ||
                    src_y + k >= data_->height())
                {
                    // Only consider 4 neighbors within the image
                    continue;
                }
                int id = get_id(ImVec2(src_x + j, src_y + k));
                if (id != 0)
                {
                    // Coefficient of f_q is -1
                    triplet.push_back(Eigen::Triplet<float>(i, id - 1, -1));
                }
                np++;
            }
        }
        // Coefficient of f_i is np
        triplet.push_back(Eigen::Triplet<float>(i, i, np));
    }
    A_.setFromTriplets(triplet.begin(), triplet.end());

    solver_.compute(A_);
    if (solver_.info() != Eigen::Success)
    {
        // Decomposition failed
        throw std::exception("Decomposition failed");
    }
    return;
}

/**
 * @brief The solver of A.
 * @param b The right-hand side of the equation.
 * @param x The solution of the equation.
 */
void CompSourceImage::solver(Eigen::VectorXf& b, Eigen::VectorXf& x)
{
    x = solver_.solve(b);
    if (solver_.info() != Eigen::Success)
    {
        // Solve failed
        throw std::exception("Solve failed");
    }
    return;
}

/**
 * @brief Check if the solver is ready.
 * @return True if the solver is ready, false if not.
 */
bool CompSourceImage::is_solver_ready()
{
    if (solver_.info() != Eigen::Success)
    {
        return false;
    }
    return true;
};
}  // namespace USTC_CG