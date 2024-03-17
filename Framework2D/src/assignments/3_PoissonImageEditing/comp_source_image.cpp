#include "comp_source_image.h"

#include <algorithm>
#include <cmath>
#include <queue>

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
    start_ = end_ = ImVec2(-1, -1);
    edge_points_.clear();
    draw_status_ = false;
    flag_solver_ready_ = false;
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
    if (is_hovered_ && !draw_status_ &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        draw_status_ = true;
        ImVec2 start(
            std::clamp<float>(
                io.MousePos.x - position_.x, 0, (float)image_width_),
            std::clamp<float>(
                io.MousePos.y - position_.y, 0, (float)image_height_));
        edge_points_.clear();
        edge_points_.push_back(start);
        start_ = start;
        end_ = start;
    }
    else if (draw_status_)
    {
        flag_solver_ready_ = false;
        now_ = ImVec2(
            std::clamp<float>(
                io.MousePos.x - position_.x, 0, (float)image_width_),
            std::clamp<float>(
                io.MousePos.y - position_.y, 0, (float)image_height_));
        if (region_type_ == kFreehand)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (fabs(now_.x - edge_points_.back().x) > 5 ||
                    fabs(now_.y - edge_points_.back().y) > 5)
                {
                    if (now_.x < start_.x)
                    {
                        start_.x = now_.x;
                    }
                    if (now_.y < start_.y)
                    {
                        start_.y = now_.y;
                    }
                    if (now_.x > end_.x)
                    {
                        end_.x = now_.x;
                    }
                    if (now_.y > end_.y)
                    {
                        end_.y = now_.y;
                    }
                    edge_points_.push_back(now_);
                }
            }
            else
            {
                now_ = edge_points_.front();
                edge_points_.push_back(now_);
                draw_status_ = false;
                now_ = ImVec2(-1, -1);

                // Start the timer
                clock_t start, end;
                start = clock();

                // Update the selected region.
                init_selections();
                init_id();
                init_matrix();

                // End the timer
                end = clock();
                printf(
                    "Scanning and Computing A Time: %f\n",
                    (double)(end - start) / CLOCKS_PER_SEC);
            }
        }
        if (region_type_ == kRect && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            end_ = now_;
            edge_points_.push_back(end_);
            draw_status_ = false;
            now_ = ImVec2(-1, -1);
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
            for (int i = (int)(start_.x); i < (int)(end_.x); ++i)
            {
                for (int j = (int)(start_.y); j < (int)(end_.y); ++j)
                {
                    selected_region_->set_pixel(i, j, { 255 });
                    // Select the region by setting the pixel value to
                    // 255.
                }
            }

            // Start the timer
            clock_t start, end;
            start = clock();

            init_id();
            init_matrix();

            // End the timer
            end = clock();
            printf(
                "Scanning and Computing A Time: %f\n",
                (double)(end - start) / CLOCKS_PER_SEC);
        }
        if (region_type_ == kPolygon)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (now_.x < start_.x)
                {
                    start_.x = now_.x;
                }
                if (now_.y < start_.y)
                {
                    start_.y = now_.y;
                }
                if (now_.x > end_.x)
                {
                    end_.x = now_.x;
                }
                if (now_.y > end_.y)
                {
                    end_.y = now_.y;
                }
                edge_points_.push_back(now_);
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                now_ = edge_points_.front();
                edge_points_.push_back(now_);
                draw_status_ = false;
                now_ = ImVec2(-1, -1);

                // Start the timer
                clock_t start, end;
                start = clock();

                // Update the selected region.
                init_selections();
                init_id();
                init_matrix();

                // End the timer
                end = clock();
                printf(
                    "Scanning and Computing A Time: %f\n",
                    (double)(end - start) / CLOCKS_PER_SEC);
            }
        }
    }

    // Visualization
    auto draw_list = ImGui::GetWindowDrawList();
    float thickness = 2.0f;

    switch (region_type_)
    {
        case USTC_CG::CompSourceImage::kDefault: break;
        case USTC_CG::CompSourceImage::kRect:
        {
            if (start_.x != -1)
            {
                if (now_.x != -1 && start_.x < now_.x && start_.y < now_.y)
                {
                    draw_list->AddRect(
                        ImVec2(start_.x + position_.x, start_.y + position_.y),
                        ImVec2(now_.x + position_.x, now_.y + position_.y),
                        IM_COL32(255, 0, 0, 255),
                        thickness);
                }
                else if (end_.x != -1 && start_.x < end_.x && start_.y < end_.y)
                {
                    draw_list->AddRect(
                        ImVec2(start_.x + position_.x, start_.y + position_.y),
                        ImVec2(end_.x + position_.x, end_.y + position_.y),
                        IM_COL32(255, 0, 0, 255),
                        thickness);
                }
            }
            break;
        }
        case USTC_CG::CompSourceImage::kPolygon:
        {
            if (!edge_points_.empty())
            {
                for (int i = 0; i < edge_points_.size() - 1; i++)
                {
                    draw_list->AddLine(
                        ImVec2(
                            edge_points_[i].x + position_.x,
                            edge_points_[i].y + position_.y),
                        ImVec2(
                            edge_points_[i + 1].x + position_.x,
                            edge_points_[i + 1].y + position_.y),
                        IM_COL32(255, 0, 0, 255),
                        thickness);
                }
                if (now_.x != -1)
                {
                    draw_list->AddLine(
                        ImVec2(
                            edge_points_.back().x + position_.x,
                            edge_points_.back().y + position_.y),
                        ImVec2(now_.x + position_.x, now_.y + position_.y),
                        IM_COL32(255, 0, 0, 255),
                        thickness);
                    draw_list->AddLine(
                        ImVec2(now_.x + position_.x, now_.y + position_.y),
                        ImVec2(
                            edge_points_.front().x + position_.x,
                            edge_points_.front().y + position_.y),
                        IM_COL32(255, 0, 0, 255),
                        thickness / 2);
                }
            }
            break;
        }
        case USTC_CG::CompSourceImage::kFreehand:
        {
            if (!edge_points_.empty())
            {
                for (int i = 0; i < edge_points_.size() - 1; i++)
                {
                    draw_list->AddLine(
                        ImVec2(
                            edge_points_[i].x + position_.x,
                            edge_points_[i].y + position_.y),
                        ImVec2(
                            edge_points_[i + 1].x + position_.x,
                            edge_points_[i + 1].y + position_.y),
                        IM_COL32(255, 0, 0, 255),
                        thickness);
                }
                if (now_.x != -1)
                {
                    draw_list->AddLine(
                        ImVec2(
                            edge_points_.back().x + position_.x,
                            edge_points_.back().y + position_.y),
                        ImVec2(
                            edge_points_.front().x + position_.x,
                            edge_points_.front().y + position_.y),
                        IM_COL32(255, 0, 0, 255),
                        thickness / 2);
                }
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
 * @brief Scanning line algorithm to fill the selected region.
 */
void CompSourceImage::init_selections()
{
    for (int i = 0; i < selected_region_->width(); ++i)
    {
        for (int j = 0; j < selected_region_->height(); ++j)
        {
            selected_region_->set_pixel(i, j, { 0 });
        }
    }
    // edge_points_.push_back(edge_points_[1]);  // For easy iteration
    std::vector<
        std::priority_queue<float, std::vector<float>, std::greater<float>>>
        point_table(selected_region_->height());
    /*
    for (int i = 1; i < edge_points_.size() - 1; i++)
    {
        // Tackle with each point.
        if ((edge_points_[i - 1].y - edge_points_[i].y) *
                (edge_points_[i + 1].y - edge_points_[i].y) <
            0)
        {
            // In different sides of the horizontal line.
            point_table[(int)edge_points_[i].y].push(edge_points_[i].x);
        }
        // Tackle with horizontal line. There are four cases.
        else if (
            (edge_points_[i - 1].y == edge_points_[i].y &&
             edge_points_[i + 1].y < edge_points_[i].y) ||
            (edge_points_[i - 1].y < edge_points_[i].y &&
             edge_points_[i + 1].y == edge_points_[i].y))
        {
            point_table[(int)edge_points_[i].y].push(edge_points_[i].x);
        }
        // If edge_points_[i - 1].y == edge_points_[i].y and edge_points_[i +
        // 1].y == edge_points_[i].y, do nothing.
    }
    edge_points_.pop_back();
    for (int i = 0; i < edge_points_.size() - 1; i++)
    {
        // Tackle with each line.
        float k = (edge_points_[i + 1].y - edge_points_[i].y) /
                  (edge_points_[i + 1].x - edge_points_[i].x);
        float b = edge_points_[i].y - k * edge_points_[i].x;
        float y1 = edge_points_[i].y, y2 = edge_points_[i + 1].y;
        if (y1 > y2)
        {
            std::swap(y1, y2);
        }
        for (int y = (int)y1 + 1; y < (int)y2; y++)
        {
            float x = (y - b) / k;
            x = std::clamp(x, 0.0f, (float)selected_region_->width());
            point_table[y].push(x);
        }
    }
    */

    // Consider that y may not be integer, the method above considering
    // intersection at point may not work. Add 0.1 to each y to make sure that
    // every y is float.
    float d = 0.1f;
    for (int i = 0; i < edge_points_.size(); i++)
    {
        edge_points_[i].y += d;
    }
    // For each edge, we add intersections with horizontal lines.
    for (int i = 0; i < edge_points_.size() - 1; i++)
    {
        float y1 = edge_points_[i].y, y2 = edge_points_[i + 1].y;
        if (y1 > y2)
        {
            std::swap(y1, y2);
        }
        if (edge_points_[i + 1].x == edge_points_[i].x)
        {
            for (int y = (int)y1 + 1; y <= (int)y2; y++)
            {
                point_table[y].push(edge_points_[i].x);
            }
        }
        else
        {
            float k = (edge_points_[i + 1].y - edge_points_[i].y) /
                      (edge_points_[i + 1].x - edge_points_[i].x);
            float b = edge_points_[i].y - k * edge_points_[i].x;
            for (int y = (int)y1 + 1; y <= (int)y2; y++)
            {
                float x = (y - b) / k;
                point_table[y].push(x);
            }
        }
    }
    for (int i = 0; i < edge_points_.size(); i++)
    {
        edge_points_[i].y -= d;
    }

    // Point table is ready. Now fill the region.
    for (int i = (int)start_.y; i <= end_.y; i++)
    {
        while (!point_table[i].empty())
        {
            int x_1 = (int)point_table[i].top();
            point_table[i].pop();
            if (point_table[i].empty())
            {
                throw std::exception(
                    "Bug: point_table[%d] only have odd number of elements", i);
            }
            int x_2 = (int)point_table[i].top();
            point_table[i].pop();
            if (x_1 < 0 || x_2 < 0 || x_1 >= selected_region_->width() ||
                x_2 >= selected_region_->width())
            {
                printf("Bug: x_1 = %d, x_2 = %d, y = %d\n", x_1, x_2, i);
                continue;
            }
            for (int j = x_1; j <= x_2; j++)
            {
                selected_region_->set_pixel(j, i, { 255 });
            }
        }
    }
    return;
}

/**
 * @brief Initialize the selected region by giving every point an id.
 */
void CompSourceImage::init_id()
{
    point_to_id_.clear();
    id_to_point_.clear();
    point_to_id_.resize(selected_region_->width());
    for (int i = 0; i < selected_region_->width(); i++)
    {
        point_to_id_[i].resize(selected_region_->height());
    }
    int id = 1;
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

    int point_num = (int)id_to_point_.size();

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
                int id = get_id(ImVec2((float)(src_x + j), (float)(src_y + k)));
                if (id != 0)
                {
                    // Coefficient of f_q is -1
                    triplet.push_back(Eigen::Triplet<float>(i, id - 1, -1));
                }
                // Add coefficient of f_i
                triplet.push_back(Eigen::Triplet<float>(i, i, 1));
            }
        }
    }
    A_.setFromTriplets(triplet.begin(), triplet.end());

    solver_.compute(A_);
    if (solver_.info() != Eigen::Success)
    {
        // Decomposition failed
        throw std::exception("Decomposition failed");
    }
    flag_solver_ready_ = true;
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
    return flag_solver_ready_;
};
}  // namespace USTC_CG