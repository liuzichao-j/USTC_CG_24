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
            Eigen::VectorXf br(point_num);
            Eigen::VectorXf bg(point_num);
            Eigen::VectorXf bb(point_num);
            Eigen::VectorXf ba(point_num);
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

                // Initialize
                br(i) = bg(i) = bb(i) = ba(i) = 0;

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
                            if ((tar_x + j < 0) ||
                                (tar_x + j >= image_width_) ||
                                (tar_y + k < 0) || (tar_y + k >= image_height_))
                            {
                                // If it is out of the target image, use the
                                // source image instead
                                br(i) += src_data->get_pixel(
                                    src_x + j, src_y + k)[0];
                                bg(i) += src_data->get_pixel(
                                    src_x + j, src_y + k)[1];
                                bb(i) += src_data->get_pixel(
                                    src_x + j, src_y + k)[2];
                                ba(i) += src_data->get_pixel(
                                    src_x + j, src_y + k)[3];
                            }
                            else
                            {
                                br(i) +=
                                    data_->get_pixel(tar_x + j, tar_y + k)[0];
                                bg(i) +=
                                    data_->get_pixel(tar_x + j, tar_y + k)[1];
                                bb(i) +=
                                    data_->get_pixel(tar_x + j, tar_y + k)[2];
                                ba(i) +=
                                    data_->get_pixel(tar_x + j, tar_y + k)[3];
                            }
                        }
                        // Add g_q to the right side, which represents the
                        // gradient of the source image
                        br(i) +=
                            (src_data->get_pixel(src_x, src_y)[0] -
                             src_data->get_pixel(src_x + j, src_y + k)[0]);
                        bg(i) +=
                            (src_data->get_pixel(src_x, src_y)[1] -
                             src_data->get_pixel(src_x + j, src_y + k)[1]);
                        bb(i) +=
                            (src_data->get_pixel(src_x, src_y)[2] -
                             src_data->get_pixel(src_x + j, src_y + k)[2]);
                        ba(i) +=
                            (src_data->get_pixel(src_x, src_y)[3] -
                             src_data->get_pixel(src_x + j, src_y + k)[3]);
                    }
                }
            }

            // Then solve the linear system
            Eigen::VectorXf xr;
            Eigen::VectorXf xg;
            Eigen::VectorXf xb;
            Eigen::VectorXf xa;
            source_image_->solver(br, xr);
            source_image_->solver(bg, xg);
            source_image_->solver(bb, xb);
            source_image_->solver(ba, xa);

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
                    unsigned char r =
                        (unsigned char)(xr(i) < 0
                                            ? 0
                                            : (xr(i) > 255 ? 255 : xr(i)));
                    unsigned char g =
                        (unsigned char)(xg(i) < 0
                                            ? 0
                                            : (xg(i) > 255 ? 255 : xg(i)));
                    unsigned char b =
                        (unsigned char)(xb(i) < 0
                                            ? 0
                                            : (xb(i) > 255 ? 255 : xb(i)));
                    unsigned char a =
                        (unsigned char)(xa(i) < 0
                                            ? 0
                                            : (xa(i) > 255 ? 255 : xa(i)));
                    data_->set_pixel(tar_x, tar_y, { r, g, b, a });
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
            Eigen::VectorXf br(point_num);
            Eigen::VectorXf bg(point_num);
            Eigen::VectorXf bb(point_num);
            Eigen::VectorXf ba(point_num);
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

                // Initialize
                br(i) = bg(i) = bb(i) = ba(i) = 0;

                // We need to know if the gradient of the source image is bigger
                // than the gradient of the target image
                int d_f = 0, d_g = 0;

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
                        for (int l = 0; l < 3; l++)
                        {
                            if ((tar_x + j < 0) ||
                                (tar_x + j >= image_width_) ||
                                (tar_y + k < 0) || (tar_y + k >= image_height_))
                            {
                                d_f += std::pow(
                                    src_data->get_pixel(
                                        src_x + j, src_y + k)[l] -
                                        src_data->get_pixel(src_x, src_y)[l],
                                    2);
                            }
                            else
                            {
                                d_f += std::pow(
                                    data_->get_pixel(tar_x + j, tar_y + k)[l] -
                                        data_->get_pixel(tar_x, tar_y)[l],
                                    2);
                            }
                            d_g += std::pow(
                                src_data->get_pixel(src_x + j, src_y + k)[l] -
                                    src_data->get_pixel(src_x, src_y)[l],
                                2);
                        }
                    }
                }

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
                            if ((tar_x + j < 0) ||
                                (tar_x + j >= image_width_) ||
                                (tar_y + k < 0) || (tar_y + k >= image_height_))
                            {
                                // If it is out of the target image, use the
                                // source image instead
                                br(i) += src_data->get_pixel(
                                    src_x + j, src_y + k)[0];
                                bg(i) += src_data->get_pixel(
                                    src_x + j, src_y + k)[1];
                                bb(i) += src_data->get_pixel(
                                    src_x + j, src_y + k)[2];
                                ba(i) += src_data->get_pixel(
                                    src_x + j, src_y + k)[3];
                            }
                            else
                            {
                                br(i) +=
                                    data_->get_pixel(tar_x + j, tar_y + k)[0];
                                bg(i) +=
                                    data_->get_pixel(tar_x + j, tar_y + k)[1];
                                bb(i) +=
                                    data_->get_pixel(tar_x + j, tar_y + k)[2];
                                ba(i) +=
                                    data_->get_pixel(tar_x + j, tar_y + k)[3];
                            }
                        }
                        // Add g_q to the right side, which represents the
                        // gradient of the source image.

                        // For mixed situation, we choose the bigger one of the
                        // two gradients
                        if (d_f < d_g)
                        {
                            // As the original seamless method
                            br(i) +=
                                (src_data->get_pixel(src_x, src_y)[0] -
                                 src_data->get_pixel(src_x + j, src_y + k)[0]);
                            bg(i) +=
                                (src_data->get_pixel(src_x, src_y)[1] -
                                 src_data->get_pixel(src_x + j, src_y + k)[1]);
                            bb(i) +=
                                (src_data->get_pixel(src_x, src_y)[2] -
                                 src_data->get_pixel(src_x + j, src_y + k)[2]);
                            ba(i) +=
                                (src_data->get_pixel(src_x, src_y)[3] -
                                 src_data->get_pixel(src_x + j, src_y + k)[3]);
                        }
                        else
                        {
                            // Replace g_q with f_q
                            if (tar_x + j < 0 || tar_x + j >= image_width_ ||
                                tar_y + k < 0 || tar_y + k >= image_height_)
                            {
                                // If not in the image, use source gradient
                                // instead
                                br(i) +=
                                    (src_data->get_pixel(src_x, src_y)[0] -
                                     src_data->get_pixel(
                                         src_x + j, src_y + k)[0]);
                                bg(i) +=
                                    (src_data->get_pixel(src_x, src_y)[1] -
                                     src_data->get_pixel(
                                         src_x + j, src_y + k)[1]);
                                bb(i) +=
                                    (src_data->get_pixel(src_x, src_y)[2] -
                                     src_data->get_pixel(
                                         src_x + j, src_y + k)[2]);
                                ba(i) +=
                                    (src_data->get_pixel(src_x, src_y)[3] -
                                     src_data->get_pixel(
                                         src_x + j, src_y + k)[3]);
                            }
                            else
                            {
                                br(i) +=
                                    (data_->get_pixel(tar_x, tar_y)[0] -
                                     data_->get_pixel(tar_x + j, tar_y + k)[0]);
                                bg(i) +=
                                    (data_->get_pixel(tar_x, tar_y)[1] -
                                     data_->get_pixel(tar_x + j, tar_y + k)[1]);
                                bb(i) +=
                                    (data_->get_pixel(tar_x, tar_y)[2] -
                                     data_->get_pixel(tar_x + j, tar_y + k)[2]);
                                ba(i) +=
                                    (data_->get_pixel(tar_x, tar_y)[3] -
                                     data_->get_pixel(tar_x + j, tar_y + k)[3]);
                            }
                        }
                    }
                }
            }

            // Then solve the linear system
            Eigen::VectorXf xr;
            Eigen::VectorXf xg;
            Eigen::VectorXf xb;
            Eigen::VectorXf xa;
            source_image_->solver(br, xr);
            source_image_->solver(bg, xg);
            source_image_->solver(bb, xb);
            source_image_->solver(ba, xa);

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
                    unsigned char r =
                        (unsigned char)(xr(i) < 0
                                            ? 0
                                            : (xr(i) > 255 ? 255 : xr(i)));
                    unsigned char g =
                        (unsigned char)(xg(i) < 0
                                            ? 0
                                            : (xg(i) > 255 ? 255 : xg(i)));
                    unsigned char b =
                        (unsigned char)(xb(i) < 0
                                            ? 0
                                            : (xb(i) > 255 ? 255 : xb(i)));
                    unsigned char a =
                        (unsigned char)(xa(i) < 0
                                            ? 0
                                            : (xa(i) > 255 ? 255 : xa(i)));
                    data_->set_pixel(tar_x, tar_y, { r, g, b, a });
                }
            }
            break;
        }
        default: break;
    }

    update();
}

}  // namespace USTC_CG