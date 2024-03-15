#include "comp_target_image.h"

#include <Eigen/Sparse>
#include <cmath>

namespace USTC_CG
{
CompTargetImage::CompTargetImage(
    const std::string& label,
    const std::string& filename)
    : ImageEditor(label, filename)
{
    if (data_)
    {
        back_up_ = std::make_shared<Image>(*data_);
    }
}

/**
 * @brief Draw the target image and the invisible button for mouse interaction.
 */
void CompTargetImage::draw()
{
    // Draw the image
    ImageEditor::draw();
    // Invisible button for interactions
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    bool is_hovered_ = ImGui::IsItemHovered();
    // When the mouse is clicked or moving, we would adapt clone function to
    // copy the selected region to the target.
    ImGuiIO& io = ImGui::GetIO();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        edit_status_ = true;
        mouse_position_ =
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
        clone();
    }
    if (edit_status_)
    {
        mouse_position_ =
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
        if (flag_realtime_updating)
        {
            clone();
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            edit_status_ = false;
        }
    }
}

/**
 * @brief Set the source image for the target image, for easy access when
 * cloning.
 */
void CompTargetImage::set_source(std::shared_ptr<CompSourceImage> source)
{
    source_image_ = source;
}

/**
 * @brief Set whether to update the target image in real time.
 */
void CompTargetImage::set_realtime(bool flag)
{
    flag_realtime_updating = flag;
}

/**
 * @brief Restore the target image to the original state.
 */
void CompTargetImage::restore()
{
    *data_ = *back_up_;
    update();
}

/**
 * @brief Set the type of cloning to paste.
 */
void CompTargetImage::set_paste()
{
    clone_type_ = kPaste;
}

/**
 * @brief Set the type of cloning to seamless.
 */
void CompTargetImage::set_seamless()
{
    clone_type_ = kSeamless;
}

/**
 * @brief Set the type of cloning to mixed seamless.
 */
void CompTargetImage::set_mixed_seamless()
{
    clone_type_ = kMixedSeamless;
}

/**
 * @brief Clone the selected region from the source image to the target image.
 * Use different methods for different types of cloning.
 */
void CompTargetImage::clone()
{
    // The implementation of different types of cloning
    // HW3_TODO: In this function, you should at least implement the "seamless"
    // cloning labeled by `clone_type_ ==kSeamless`.
    //
    // The realtime updating (update when the mouse is moving) is only available
    // when the checkboard is selected. It is required to improve the efficiency
    // of your seamless cloning to achieve realtime editing. (Use decomposition
    // of sparse matrix before solve the linear system)
    std::shared_ptr<Image> mask = source_image_->get_region();

    switch (clone_type_)
    {
        case USTC_CG::CompTargetImage::kDefault: break;
        case USTC_CG::CompTargetImage::kPaste:
        {
            restore();

            for (int i = 0; i < mask->width(); ++i)
            {
                for (int j = 0; j < mask->height(); ++j)
                {
                    int tar_x =
                        static_cast<int>(mouse_position_.x) + i -
                        static_cast<int>(source_image_->get_position().x);
                    int tar_y =
                        static_cast<int>(mouse_position_.y) + j -
                        static_cast<int>(source_image_->get_position().y);
                    if (0 <= tar_x && tar_x < image_width_ && 0 <= tar_y &&
                        tar_y < image_height_ && mask->get_pixel(i, j)[0] > 0)
                    {
                        data_->set_pixel(
                            tar_x,
                            tar_y,
                            source_image_->get_data()->get_pixel(i, j));
                    }
                }
            }
            break;
        }
        case USTC_CG::CompTargetImage::kSeamless:
        {
            // You should delete this block and implement your own seamless
            // cloning. For each pixel in the selected region, calculate the
            // final RGB color by solving Poisson Equations.
            restore();

            if (!source_image_->is_solver_ready())
            {
                break;
            }

            int point_num = source_image_->get_point_num();
            std::shared_ptr<Image> src_data = source_image_->get_data();

            // Calculate br, bg, bb and ba, which are the right sides of the
            // equation.
            Eigen::VectorXf b[4];
            for (int i = 0; i < 4; i++)
            {
                b[i] = Eigen::VectorXf::Zero(point_num);
            }

            for (int i = 0; i < point_num; i++)
            {
                // Make the formula No. i. Point is in the coordinate of the
                // source image.
                int src_x = (int)source_image_->get_point(i).x;
                int src_y = (int)source_image_->get_point(i).y;
                int tar_x = src_x + (int)(mouse_position_.x -
                                          source_image_->get_position().x);
                int tar_y = src_y + (int)(mouse_position_.y -
                                          source_image_->get_position().y);

                // For each neighbor of the point
                for (int j = -1; j <= 1; j++)
                {
                    for (int k = -1; k <= 1; k++)
                    {
                        if ((abs(j) + abs(k) != 1) || src_x + j < 0 ||
                            src_x + j >= src_data->width() || src_y + k < 0 ||
                            src_y + k >= src_data->height())
                        {
                            // Only consider 4 neighbors within the image
                            continue;
                        }
                        int id =
                            source_image_->get_id(ImVec2(src_x + j, src_y + k));
                        if (id == 0)
                        {
                            // Add f_q to the right side, which is the edge of
                            // the target image
                            if (tar_x + j < 0 || tar_x + j >= image_width_ ||
                                tar_y + k < 0 || tar_y + k >= image_height_)
                            {
                                // If it is out of the target image, use the
                                // source image instead
                                for (int l = 0; l < 4; l++)
                                {
                                    b[l](i) += src_data->get_pixel(
                                        src_x + j, src_y + k)[l];
                                }
                            }
                            else
                            {
                                for (int l = 0; l < 4; l++)
                                {
                                    b[l](i) += data_->get_pixel(
                                        tar_x + j, tar_y + k)[l];
                                }
                            }
                        }
                        // Add g_q to the right side, which represents the
                        // gradient of the source image
                        for (int l = 0; l < 4; l++)
                        {
                            b[l](i) +=
                                (src_data->get_pixel(src_x, src_y)[l] -
                                 src_data->get_pixel(src_x + j, src_y + k)[l]);
                        }
                    }
                }
            }

            // Then solve the linear system
            Eigen::VectorXf x[4];
            for (int i = 0; i < 4; i++)
            {
                source_image_->solver(b[i], x[i]);
            }

            // Set the result to the target image
            for (int i = 0; i < point_num; i++)
            {
                int src_x = (int)source_image_->get_point(i).x;
                int src_y = (int)source_image_->get_point(i).y;
                int tar_x =
                    (int)(mouse_position_.x - source_image_->get_position().x) +
                    src_x;
                int tar_y =
                    (int)(mouse_position_.y - source_image_->get_position().y) +
                    src_y;
                if (0 <= tar_x && tar_x < image_width_ && 0 <= tar_y &&
                    tar_y < image_height_)
                {
                    std::vector<unsigned char> c(4);
                    for (int l = 0; l < 4; l++)
                    {
                        c[l] = (unsigned char)(x[l](i) < 0
                                                   ? 0
                                                   : (x[l](i) > 255 ? 255
                                                                    : x[l](i)));
                    }
                    data_->set_pixel(tar_x, tar_y, c);
                }
            }
            break;
        }
        case USTC_CG::CompTargetImage::kMixedSeamless:
        {
            restore();

            if (!source_image_->is_solver_ready())
            {
                break;
            }

            int point_num = source_image_->get_point_num();
            std::shared_ptr<Image> src_data = source_image_->get_data();

            // Calculate br, bg, bb and ba, which are the right sides of the
            // equation.
            Eigen::VectorXf b[4];
            for (int i = 0; i < 4; i++)
            {
                b[i] = Eigen::VectorXf::Zero(point_num);
            }

            for (int i = 0; i < point_num; i++)
            {
                // Make the formula No. i. Point is in the coordinate of the
                // source image.
                int src_x = (int)source_image_->get_point(i).x;
                int src_y = (int)source_image_->get_point(i).y;
                int tar_x =
                    (int)(mouse_position_.x - source_image_->get_position().x) +
                    src_x;
                int tar_y =
                    (int)(mouse_position_.y - source_image_->get_position().y) +
                    src_y;

                // For each neighbor of the point
                for (int j = -1; j <= 1; j++)
                {
                    for (int k = -1; k <= 1; k++)
                    {
                        if ((abs(j) + abs(k) != 1) || src_x + j < 0 ||
                            src_x + j >= src_data->width() || src_y + k < 0 ||
                            src_y + k >= src_data->height())
                        {
                            // Only consider 4 neighbors within the image
                            continue;
                        }
                        int id =
                            source_image_->get_id(ImVec2(src_x + j, src_y + k));
                        if (id == 0)
                        {
                            // Add f_q to the right side, which is the edge of
                            // the target image
                            if (tar_x + j < 0 || tar_x + j >= image_width_ ||
                                tar_y + k < 0 || tar_y + k >= image_height_)
                            {
                                // If it is out of the target image, use the
                                // source image instead
                                for (int l = 0; l < 4; l++)
                                {
                                    b[l](i) += src_data->get_pixel(
                                        src_x + j, src_y + k)[l];
                                }
                            }
                            else
                            {
                                for (int l = 0; l < 4; l++)
                                {
                                    b[l](i) += data_->get_pixel(
                                        tar_x + j, tar_y + k)[l];
                                }
                            }
                        }
                        // Add g_q to the right side, which represents the
                        // gradient of the source image.

                        // For mixed situation, we choose the bigger one of the
                        // two gradients, as described in the paper.
                        if (tar_x + j < 0 || tar_x + j >= image_width_ ||
                            tar_y + k < 0 || tar_y + k >= image_height_ ||
                            tar_x < 0 || tar_x >= image_width_ || tar_y < 0 ||
                            tar_y >= image_height_)
                        {
                            // If not in the image, use source gradient
                            // instead
                            for (int l = 0; l < 4; l++)
                            {
                                b[l](i) +=
                                    (src_data->get_pixel(src_x, src_y)[l] -
                                     src_data->get_pixel(
                                         src_x + j, src_y + k)[l]);
                            }
                        }
                        else
                        {
                            for (int l = 0; l < 4; l++)
                            {
                                // For each point pair (p, q), we choose the one
                                // with bigger gradient.
                                if (fabs(
                                        data_->get_pixel(tar_x, tar_y)[l] -
                                        data_->get_pixel(
                                            tar_x + j, tar_y + k)[l]) >
                                    fabs(
                                        src_data->get_pixel(src_x, src_y)[l] -
                                        src_data->get_pixel(
                                            src_x + j, src_y + k)[l]))
                                {
                                    b[l](i) +=
                                        (data_->get_pixel(tar_x, tar_y)[l] -
                                         data_->get_pixel(
                                             tar_x + j, tar_y + k)[l]);
                                }
                                else
                                {
                                    b[l](i) +=
                                        (src_data->get_pixel(src_x, src_y)[l] -
                                         src_data->get_pixel(
                                             src_x + j, src_y + k)[l]);
                                }
                            }
                        }
                    }
                }
            }

            // Then solve the linear system
            Eigen::VectorXf x[4];
            for (int i = 0; i < 4; i++)
            {
                source_image_->solver(b[i], x[i]);
            }

            // Set the result to the target image
            for (int i = 0; i < point_num; i++)
            {
                int src_x = (int)source_image_->get_point(i).x;
                int src_y = (int)source_image_->get_point(i).y;
                int tar_x =
                    (int)(mouse_position_.x - source_image_->get_position().x) +
                    src_x;
                int tar_y =
                    (int)(mouse_position_.y - source_image_->get_position().y) +
                    src_y;
                if (0 <= tar_x && tar_x < image_width_ && 0 <= tar_y &&
                    tar_y < image_height_)
                {
                    std::vector<unsigned char> c(4);
                    for (int l = 0; l < 4; l++)
                    {
                        c[l] = (unsigned char)(x[l](i) < 0
                                                   ? 0
                                                   : (x[l](i) > 255 ? 255
                                                                    : x[l](i)));
                    }
                    data_->set_pixel(tar_x, tar_y, c);
                }
            }
            break;
        }
        default: break;
    }

    update();
}

}  // namespace USTC_CG