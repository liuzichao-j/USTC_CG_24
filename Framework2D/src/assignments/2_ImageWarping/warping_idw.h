#include <Eigen/Dense>
#include <cmath>

#include "comp_warping.h"

namespace USTC_CG
{
class WarpingIDW : public CompWarping
{
   public:
    explicit WarpingIDW(const std::string& label, const std::string& filename)
        : CompWarping(label, filename)
    {
    }
    virtual ~WarpingIDW() noexcept = default;

    /**
     * @brief Inverse Distance Weighting (IDW) warping function
     */
    void warping()
    {
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
        // Apply warping function and return the result
        warping_idw(warped_image);
        *data_ = std::move(warped_image);
        update();
    }

   private:
    /**
     * @brief Inverse Distance Weighting (IDW) warping function.
     */
    Image warping_idw(Image& warped_image)
    {
        /**
         * Given f(p_i) = (q_i), Use f(p) = Sum_i (w_i(p) * f_i(p)).
         *
         * Choose w_i(p) = sigma_i(p) / (Sum_j sigma_j(p)).
         * sigma_i(p) = 1 / ||p - p_i||^mu, mu > 1.
         *
         * f_i(p) = q_i + D_i(p - p_i).
         * To calculate matrix D_i, minimize square error.
         * min Sum_j (sigma_i(p_j) ||f_i(p_j) - q_j||^2)
         *  = min Sum_j (sigma_i(p_j) ((q_i_1 - q_j_1 + D_i_11(p_j_1 - p_i_1) +
         * D_i_12(p_j_2 - p_i_2))^2 + (q_i_2 - q_j_2 + D_i_21(p_j_1 - p_i_1) +
         * D_i_22(p_j_2 - p_i_2))^2))
         *
         * Differentiate the above equation, we get linear equations.
         * Sum_j (sigma_i(p_j) (q_i_1 - q_j_1 + D_i_11(p_j_1 - p_i_1) +
         * D_i_12(p_j_2 - p_i_2)) * (p_j_1 - p_i_1)) = 0 Sum_j (sigma_i(p_j)
         * (q_i_1 - q_j_1 + D_i_11(p_j_1 - p_i_1) + D_i_12(p_j_2 - p_i_2)) *
         * (p_j_2 - p_i_2)) = 0 Sum_j (sigma_i(p_j) (q_i_2 - q_j_2 +
         * D_i_21(p_j_1 - p_i_1) + D_i_22(p_j_2 - p_i_2)) * (p_j_1 - p_i_1)) = 0
         * Sum_j (sigma_i(p_j) (q_i_2 - q_j_2 + D_i_21(p_j_1 - p_i_1) +
         * D_i_22(p_j_2 - p_i_2)) * (p_j_2 - p_i_2)) = 0
         *
         * A * x = b, where:
         * A_11 = Sum_j (sigma_i(p_j) (p_j_1 - p_i_1)^2)
         * A_12 = Sum_j (sigma_i(p_j) (p_j_2 - p_i_2)(p_j_1 - p_i_1))
         * A_13 = 0
         * A_14 = 0
         * A_21 = Sum_j (sigma_i(p_j) (p_j_1 - p_i_1)(p_j_2 - p_i_2))
         * A_22 = Sum_j (sigma_i(p_j) (p_j_2 - p_i_2)^2)
         * A_23 = 0
         * A_24 = 0
         * A_31 = 0
         * A_32 = 0
         * A_33 = Sum_j (sigma_i(p_j) (p_j_1 - p_i_1)^2)
         * A_34 = Sum_j (sigma_i(p_j) (p_j_2 - p_i_2)(p_j_1 - p_i_1))
         * A_41 = 0
         * A_42 = 0
         * A_43 = Sum_j (sigma_i(p_j) (p_j_1 - p_i_1)(p_j_2 - p_i_2))
         * A_44 = Sum_j (sigma_i(p_j) (p_j_2 - p_i_2)^2)
         * x = [D_i_11, D_i_12, D_i_21, D_i_22]^T
         * b_1 = Sum_j (sigma_i(p_j) (q_j_1 - q_i_1)(p_j_1 - p_i_1))
         * b_2 = Sum_j (sigma_i(p_j) (q_j_1 - q_i_1)(p_j_2 - p_i_2))
         * b_3 = Sum_j (sigma_i(p_j) (q_j_2 - q_i_2)(p_j_1 - p_i_1))
         * b_4 = Sum_j (sigma_i(p_j) (q_j_2 - q_i_2)(p_j_2 - p_i_2))
         *
         * Having D_i, we can calculate f(p), which is the result of warping.
         */

        int n = (int) start_points_.size();
        double mu = 2;

        // Calculate the distance between start points (p_i, p_j)
        Eigen::MatrixXd distance_matrix(n, n);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                distance_matrix(i, j) = std::sqrt(
                    std::pow(start_points_[i].x - start_points_[j].x, 2) +
                    std::pow(start_points_[i].y - start_points_[j].y, 2));
            }
        }

        // Calculate sigma_i(p_j) = 1 / ||p_j - p_i||^mu
        Eigen::MatrixXd sigma_matrix(n, n);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                if (i == j)
                {
                    sigma_matrix(i, j) = 0;
                    // Avoid division by zero
                }
                else
                {
                    sigma_matrix(i, j) =
                        1 / std::pow(distance_matrix(i, j), mu);
                }
            }
        }

        // Calculate the D matrix
        std::vector<Eigen::MatrixXd> d_matrix(n, Eigen::MatrixXd::Zero(2, 2));
        for (int i = 0; i < n; i++)
        {
            // Calculate D_i matrix by solving linear equations of four
            // variables, D_i_11, D_i_12, D_i_21, D_i_22

            // Calculate A matrix
            Eigen::MatrixXd A = Eigen::MatrixXd::Zero(4, 4);
            for (int j = 0; j < n; j++)
            {
                A(0, 0) += sigma_matrix(i, j) *
                           std::pow(start_points_[j].x - start_points_[i].x, 2);
                A(0, 1) += sigma_matrix(i, j) *
                           (start_points_[j].x - start_points_[i].x) *
                           (start_points_[j].y - start_points_[i].y);
                A(1, 0) += sigma_matrix(i, j) *
                           (start_points_[j].x - start_points_[i].x) *
                           (start_points_[j].y - start_points_[i].y);
                A(1, 1) += sigma_matrix(i, j) *
                           std::pow(start_points_[j].y - start_points_[i].y, 2);
                A(2, 2) += sigma_matrix(i, j) *
                           std::pow(start_points_[j].x - start_points_[i].x, 2);
                A(2, 3) += sigma_matrix(i, j) *
                           (start_points_[j].x - start_points_[i].x) *
                           (start_points_[j].y - start_points_[i].y);
                A(3, 2) += sigma_matrix(i, j) *
                           (start_points_[j].x - start_points_[i].x) *
                           (start_points_[j].y - start_points_[i].y);
                A(3, 3) += sigma_matrix(i, j) *
                           std::pow(start_points_[j].y - start_points_[i].y, 2);
            }

            // Calculate b matrix
            Eigen::VectorXd b = Eigen::VectorXd::Zero(4);
            for (int j = 0; j < n; j++)
            {
                b(0) += sigma_matrix(i, j) *
                        (start_points_[j].x - start_points_[i].x) *
                        (end_points_[j].x - end_points_[i].x);
                b(1) += sigma_matrix(i, j) *
                        (start_points_[j].y - start_points_[i].y) *
                        (end_points_[j].x - end_points_[i].x);
                b(2) += sigma_matrix(i, j) *
                        (start_points_[j].x - start_points_[i].x) *
                        (end_points_[j].y - end_points_[i].y);
                b(3) += sigma_matrix(i, j) *
                        (start_points_[j].y - start_points_[i].y) *
                        (end_points_[j].y - end_points_[i].y);
            }

            // Solve the linear equations A * x = b
            Eigen::VectorXd x = A.colPivHouseholderQr().solve(b);

            // Store the result to D matrix
            d_matrix[i](0, 0) = x(0);
            d_matrix[i](0, 1) = x(1);
            d_matrix[i](1, 0) = x(2);
            d_matrix[i](1, 1) = x(3);
        }

        // Got D matrix, calculate f(p) for each pixel
        for (int old_x = 0; old_x < data_->width(); old_x++)
        {
            for (int old_y = 0; old_y < data_->height(); old_y++)
            {
                int new_x = 0;
                int new_y = 0;

                // Answer shows as sum of w_i(p) * f_i(p)
                for (int i = 0; i < n; i++)
                {
                    // Calculate w_i(p) = sigma_i(p) / (Sum_j sigma_j(p)), where
                    // p = (old_x, old_y)
                    double sigmasum = 0;
                    for (int j = 0; j < n; j++)
                    {
                        if (old_x != start_points_[j].x ||
                            old_y != start_points_[j].y)
                        {
                            sigmasum +=
                                1 /
                                std::pow(
                                    std::sqrt(
                                        std::pow(old_x - start_points_[j].x, 2) +
                                        std::pow(old_y - start_points_[j].y, 2)),
                                    mu);
                        }
                    }
                    double w;
                    if (old_x == start_points_[i].x &&
                        old_y == start_points_[i].y)
                    {
                        w = 1;
                    }
                    else
                    {
                        w = 1 /
                            std::pow(
                                std::sqrt(
                                    std::pow(old_x - start_points_[i].x, 2) +
                                    std::pow(old_y - start_points_[i].y, 2)),
                                mu) /
                            sigmasum;
                    }

                    // Calculate f_i(p) = q_i + D_i(p - p_i) and sum them up
                    new_x += (int)(w * (end_points_[i].x +
                                        d_matrix[i](0, 0) *
                                            (old_x - start_points_[i].x) +
                                        d_matrix[i](0, 1) *
                                            (old_y - start_points_[i].y)));
                    new_y += (int)(w * (end_points_[i].y +
                                        d_matrix[i](1, 0) *
                                            (old_x - start_points_[i].x) +
                                        d_matrix[i](1, 1) *
                                            (old_y - start_points_[i].y)));
                }
                // Set the color of the new pixel
                if (new_x >= 0 && new_x < data_->width() && new_y >= 0 &&
                    new_y < data_->height())
                {
                    warped_image.set_pixel(
                        new_x, new_y, data_->get_pixel(old_x, old_y));
                }
            }
        }

        return warped_image;
    }

    Image warping_idw_inverse(
        const std::shared_ptr<Image>& image,
        const std::vector<ImVec2>& start_points_,
        const std::vector<ImVec2>& end_points_)
    {
        // Create a new image to store the result
        Image warped_image(*image);
        // Initialize the color of result image
        for (int y = 0; y < image->height(); ++y)
        {
            for (int x = 0; x < image->width(); ++x)
            {
                warped_image.set_pixel(x, y, { 0, 0, 0 });
            }
        }

        int n = (int) start_points_.size();
        double mu = 2;

        // Calculate the distance between start points (p_i, p_j)
        Eigen::MatrixXd distance_matrix(n, n);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                distance_matrix(i, j) = std::sqrt(
                    std::pow(start_points_[i].x - start_points_[j].x, 2) +
                    std::pow(start_points_[i].y - start_points_[j].y, 2));
            }
        }

        // Calculate sigma_i(p_j) = 1 / ||p_j - p_i||^mu
        Eigen::MatrixXd sigma_matrix(n, n);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                if (i == j)
                {
                    sigma_matrix(i, j) = 0;
                    // Avoid division by zero
                }
                else
                {
                    sigma_matrix(i, j) =
                        1 / std::pow(distance_matrix(i, j), mu);
                }
            }
        }

        // Calculate the D matrix
        std::vector<Eigen::MatrixXd> d_matrix;
        for (int i = 0; i < n; i++)
        {
            // Calculate D_i matrix by solving linear equations of four
            // variables, D_i_11, D_i_12, D_i_21, D_i_22
            d_matrix[i] = Eigen::MatrixXd::Zero(2, 2);

            // Calculate A matrix
            Eigen::MatrixXd A = Eigen::MatrixXd::Zero(4, 4);
            for (int j = 0; j < n; j++)
            {
                A(0, 0) += sigma_matrix(i, j) *
                           std::pow(start_points_[j].x - start_points_[i].x, 2);
                A(0, 1) += sigma_matrix(i, j) *
                           (start_points_[j].x - start_points_[i].x) *
                           (start_points_[j].y - start_points_[i].y);
                A(1, 0) += sigma_matrix(i, j) *
                           (start_points_[j].x - start_points_[i].x) *
                           (start_points_[j].y - start_points_[i].y);
                A(1, 1) += sigma_matrix(i, j) *
                           std::pow(start_points_[j].y - start_points_[i].y, 2);
                A(2, 2) += sigma_matrix(i, j) *
                           std::pow(start_points_[j].x - start_points_[i].x, 2);
                A(2, 3) += sigma_matrix(i, j) *
                           (start_points_[j].x - start_points_[i].x) *
                           (start_points_[j].y - start_points_[i].y);
                A(3, 2) += sigma_matrix(i, j) *
                           (start_points_[j].x - start_points_[i].x) *
                           (start_points_[j].y - start_points_[i].y);
                A(3, 3) += sigma_matrix(i, j) *
                           std::pow(start_points_[j].y - start_points_[i].y, 2);
            }

            // Calculate b matrix
            Eigen::VectorXd b = Eigen::VectorXd::Zero(4);
            for (int j = 0; j < n; j++)
            {
                b(0) += sigma_matrix(i, j) *
                        (start_points_[j].x - start_points_[i].x) *
                        (end_points_[j].x - end_points_[i].x);
                b(1) += sigma_matrix(i, j) *
                        (start_points_[j].y - start_points_[i].y) *
                        (end_points_[j].x - end_points_[i].x);
                b(2) += sigma_matrix(i, j) *
                        (start_points_[j].x - start_points_[i].x) *
                        (end_points_[j].y - end_points_[i].y);
                b(3) += sigma_matrix(i, j) *
                        (start_points_[j].y - start_points_[i].y) *
                        (end_points_[j].y - end_points_[i].y);
            }

            // Solve the linear equations A * x = b
            Eigen::VectorXd x = A.colPivHouseholderQr().solve(b);

            // Store the result to D matrix
            d_matrix[i](0, 0) = x(0);
            d_matrix[i](0, 1) = x(1);
            d_matrix[i](1, 0) = x(2);
            d_matrix[i](1, 1) = x(3);
        }

        // Got D matrix, calculate f(p) for each pixel
        for (int old_x = 0; old_x < image->width(); old_x++)
        {
            for (int old_y = 0; old_y < image->height(); old_y++)
            {
                int new_x = 0;
                int new_y = 0;

                // Answer shows as sum of w_i(p) * f_i(p)
                for (int i = 0; i < n; i++)
                {
                    // Calculate w_i(p) = sigma_i(p) / (Sum_j sigma_j(p)), where
                    // p = (old_x, old_y)
                    double sigmasum = 0;
                    for (int j = 0; j < n; j++)
                    {
                        if (old_x != start_points_[j].x ||
                            old_y != start_points_[j].y)
                        {
                            sigmasum +=
                                1 /
                                std::pow(
                                    std::sqrt(
                                        std::pow(old_x - start_points_[j].x, 2) +
                                        std::pow(old_y - start_points_[j].y, 2)),
                                    mu);
                        }
                    }
                    double w;
                    if (old_x == start_points_[i].x &&
                        old_y == start_points_[i].y)
                    {
                        w = 1;
                    }
                    else
                    {
                        w = 1 /
                            std::pow(
                                std::sqrt(
                                    std::pow(old_x - start_points_[i].x, 2) +
                                    std::pow(old_y - start_points_[i].y, 2)),
                                mu) /
                            sigmasum;
                    }

                    // Calculate f_i(p) = q_i + D_i(p - p_i) and sum them up
                    new_x +=
                        (int) (w * (end_points_[i].x +
                             d_matrix[i](0, 0) * (old_x - start_points_[i].x) +
                             d_matrix[i](0, 1) * (old_y - start_points_[i].y)));
                    new_y +=
                        (int) (w * (end_points_[i].y +
                             d_matrix[i](1, 0) * (old_x - start_points_[i].x) +
                             d_matrix[i](1, 1) * (old_y - start_points_[i].y)));
                }
                // Set the color of the new pixel
                if (new_x >= 0 && new_x < image->width() && new_y >= 0 &&
                    new_y < image->height())
                {
                    // warped_image.set_pixel(new_x, new_y,
                    //     image->get_pixel(old_x, old_y));

                    // The difference of inverse: calculate what pixel is the
                    // new one originated from.
                    warped_image.set_pixel(
                        old_x, old_y, image->get_pixel(new_x, new_y));
                }
            }
        }

        return warped_image;
    }
};
}  // namespace USTC_CG