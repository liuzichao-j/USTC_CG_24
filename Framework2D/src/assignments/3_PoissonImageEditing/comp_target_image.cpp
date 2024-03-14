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

            // We have formula: 4*f_p - Sum_{q in N(p) and Omega} f_q = Sum_{q
            // in N(p) and NonOmega} f_q + 4*g_p - Sum_{q in N(p)} g_q, where
            // g_p is the source image and f_p is the target image.

            // Things in the left side of the equation are variables.

            // Sparse matrix A and Triplet to build A.
            Eigen::SparseMatrix<double> A(
                source_image_->get_point_num(), source_image_->get_point_num());
            // Each point has 5 coefficients at most.
            std::vector<Eigen::Triplet<double>> triplet;
            triplet.reserve(source_image_->get_point_num() * 5);

            // Vector br, bg, bb for the right side of the equation.
            Eigen::VectorXd br(source_image_->get_point_num());
            Eigen::VectorXd bg(source_image_->get_point_num());
            Eigen::VectorXd bb(source_image_->get_point_num());
            for (int i = 0; i < source_image_->get_point_num(); i++)
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

                // Coefficient of f_i is 4
                triplet.push_back(Eigen::Triplet<double>(i, i, 4));
                // Add to right side
                br(i) =
                    4 * source_image_->get_data()->get_pixel(src_x, src_y)[0];
                bg(i) =
                    4 * source_image_->get_data()->get_pixel(src_x, src_y)[1];
                bb(i) =
                    4 * source_image_->get_data()->get_pixel(src_x, src_y)[2];

                // For each neighbor of the point
                for (int j = -1; j <= 1; j++)
                {
                    for (int k = -1; k <= 1; k++)
                    {
                        if ((abs(j) + abs(k) != 1) || (tar_x + j < 0) ||
                            (tar_x + j >= image_width_) || (tar_y + k < 0) ||
                            (tar_y + k >= image_height_))
                        {
                            // Only consider 4 neighbors within the image
                            continue;
                        }
                        int id =
                            source_image_->get_id(ImVec2(src_x + j, src_y + k));
                        if (id != 0)
                        {
                            // Coefficient of f_q is -1
                            triplet.push_back(
                                Eigen::Triplet<double>(i, id - 1, -1));
                        }
                        else
                        {
                            // Add f_q to the right side, which is the edge of
                            // the target image
                            br(i) += data_->get_pixel(tar_x + j, tar_y + k)[0];
                            bg(i) += data_->get_pixel(tar_x + j, tar_y + k)[1];
                            bb(i) += data_->get_pixel(tar_x + j, tar_y + k)[2];
                        }
                        // Add g_q to the right side, which represents the
                        // gradient of the source image
                        br(i) -= source_image_->get_data()->get_pixel(
                            src_x + j, src_y + k)[0];
                        bg(i) -= source_image_->get_data()->get_pixel(
                            src_x + j, src_y + k)[1];
                        bb(i) -= source_image_->get_data()->get_pixel(
                            src_x + j, src_y + k)[2];
                    }
                }
            }
            A.setFromTriplets(triplet.begin(), triplet.end());

            // A and b are ready, first decompose A
            Eigen::SimplicialLLT<Eigen::SparseMatrix<double>> solver;
            solver.compute(A);
            if (solver.info() != Eigen::Success)
            {
                // Decomposition failed
                printf("Decomposition failed\n");
                return;
            }

            // Then solve the linear system
            Eigen::VectorXd xr = solver.solve(br);
            Eigen::VectorXd xg = solver.solve(bg);
            Eigen::VectorXd xb = solver.solve(bb);
            if (solver.info() != Eigen::Success)
            {
                // Solve failed
                printf("Solve failed\n");
                return;
            }

            // Set the result to the target image
            for (int i = 0; i < source_image_->get_point_num(); i++)
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
                    data_->set_pixel(
                        tar_x,
                        tar_y,
                        { (unsigned char)xr(i),
                          (unsigned char)xg(i),
                          (unsigned char)xb(i) });
                }
            }
            break;
        }
        default: break;
    }

    update();
}

}  // namespace USTC_CG