#pragma once

#include <Eigen/Dense>
#include <cmath>

#include "warping.h"

namespace USTC_CG
{
class WarpingIDW : public Warping
{
   public:
    WarpingIDW() = default;
    virtual ~WarpingIDW() noexcept = default;

    /**
     * @brief Inverse Distance Weighting (IDW) warping function.
     * @param data_ The original image data.
     * @param warped_image The result image.
     * @param start_points The start points for warping.
     * @param end_points The end points for warping.
     * @param Inverse_Flag Whether to use the inverse warping function.
     * @param Fixgap_Flag Whether to fix the gap in normal warping function.
     */
    void warping(
        std::shared_ptr<Image> &data_,
        Image &warped_image,
        std::vector<ImVec2> &start_points,
        std::vector<ImVec2> &end_points,
        bool Inverse_Flag = false,
        bool Fixgap_Flag = false) override
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
         * D_i_12(p_j_2 - p_i_2)) * (p_j_1 - p_i_1)) = 0
         *
         * Sum_j (sigma_i(p_j) (q_i_1 - q_j_1 + D_i_11(p_j_1 - p_i_1) +
         * D_i_12(p_j_2 - p_i_2)) * (p_j_2 - p_i_2)) = 0
         *
         * Sum_j (sigma_i(p_j) (q_i_2 - q_j_2 +
         * D_i_21(p_j_1 - p_i_1) + D_i_22(p_j_2 - p_i_2)) * (p_j_1 - p_i_1)) = 0
         *
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

        // Calculate a function that maps the end points to the start points,
        // for Inverse_Flag
        if (Inverse_Flag)
        {
            std::swap(start_points, end_points);
        }

        int n = (int)start_points.size();
        double mu = 2;

        // If no start points, return the original image
        if (n == 0)
        {
            warped_image = *data_;
        }

        // If there is only one start point: w_1(p) = 1, f_1(p) = q_1 + D_1(p -
        // p_1), f(p) = f_1(p), nothing to minimize. In this situation, we can
        // set D_1 = I, where I is the identity matrix. So f(p) = q_1 + (p -
        // p_1), which is the translation of the image.
        // If following steps below, we can get D_1 = 0, not suitable.
        // For it is an identity transformation, there is no need to fix the
        // gap.
        else if (n == 1)
        {
            for (int old_x = 0; old_x < data_->width(); old_x++)
            {
                for (int old_y = 0; old_y < data_->height(); old_y++)
                {
                    int new_x =
                        (int)(old_x + end_points[0].x - start_points[0].x);
                    int new_y =
                        (int)(old_y + end_points[0].y - start_points[0].y);
                    if (Inverse_Flag == false)
                    {
                        if (new_x >= 0 && new_x < data_->width() &&
                            new_y >= 0 && new_y < data_->height())
                        {
                            warped_image.set_pixel(
                                new_x, new_y, data_->get_pixel(old_x, old_y));
                        }
                    }
                    else
                    {
                        if (new_x >= 0 && new_x < data_->width() &&
                            new_y >= 0 && new_y < data_->height())
                        {
                            warped_image.set_pixel(
                                old_x, old_y, data_->get_pixel(new_x, new_y));
                        }
                    }
                }
            }
        }

        // If there are only two start points, D_1 = D_2 are not unique, leading
        // to a line shape. So we can set D_1 = D_2 = diag(d_1, d_2), where d_1
        // and d_2 are calculated by minimizing the square error. Minimize
        // (q_1_1 - q_2_1 + d_1(p_2_1 - p_1_1))^2 + (q_1_2 - q_2_2 + d_2(p_2_2 -
        // p_1_2))^2
        //
        // Differentiate the above equation, we get:
        //
        // d_1 = (q_2_1 - q_1_1)/(p_2_1 - p_1_1)
        //
        // d_2 = (q_2_2 - q_1_2)/(p_2_2 - p_1_2)
        //
        // Then we can calculate f_1(p) = q_1 + D_1(p - p_1) and f_2(p) = q_2 +
        // D_2(p - p_2). f(p) = w_1(p)f_1(p) + w_2(p)f_2(p), where w_1(p) =
        // sigma_1(p)/(sigma_1(p) + sigma_2(p)), w_2(p) = sigma_2(p)/(sigma_1(p)
        // + sigma_2(p)).
        // For there are maybe some expanded pixels, we need to fix the gap.
        // Inverse way would be effective.
        else if (n == 2)
        {
            // Fix the gap by inverse method
            if (Fixgap_Flag == true && Inverse_Flag == false)
            {
                std::swap(start_points, end_points);
            }
            double d_1 = (end_points[1].x - end_points[0].x) /
                         (start_points[1].x - start_points[0].x);
            double d_2 = (end_points[1].y - end_points[0].y) /
                         (start_points[1].y - start_points[0].y);
            for (int old_x = 0; old_x < data_->width(); old_x++)
            {
                for (int old_y = 0; old_y < data_->height(); old_y++)
                {
                    double sigma_1 =
                        1 / std::pow(
                                std::sqrt(
                                    std::pow(old_x - start_points[0].x, 2) +
                                    std::pow(old_y - start_points[0].y, 2)),
                                mu);
                    double sigma_2 =
                        1 / std::pow(
                                std::sqrt(
                                    std::pow(old_x - start_points[1].x, 2) +
                                    std::pow(old_y - start_points[1].y, 2)),
                                mu);
                    int new_x = (int)(sigma_1 / (sigma_1 + sigma_2) *
                                          (end_points[0].x +
                                           d_1 * (old_x - start_points[0].x)) +
                                      sigma_2 / (sigma_1 + sigma_2) *
                                          (end_points[1].x +
                                           d_1 * (old_x - start_points[1].x)));
                    int new_y = (int)(sigma_1 / (sigma_1 + sigma_2) *
                                          (end_points[0].y +
                                           d_2 * (old_y - start_points[0].y)) +
                                      sigma_2 / (sigma_1 + sigma_2) *
                                          (end_points[1].y +
                                           d_2 * (old_y - start_points[1].y)));
                    if (Inverse_Flag == true || Fixgap_Flag == true)
                    {
                        if (new_x >= 0 && new_x < data_->width() &&
                            new_y >= 0 && new_y < data_->height())
                        {
                            warped_image.set_pixel(
                                old_x, old_y, data_->get_pixel(new_x, new_y));
                        }
                    }
                    else
                    {
                        if (new_x >= 0 && new_x < data_->width() &&
                            new_y >= 0 && new_y < data_->height())
                        {
                            warped_image.set_pixel(
                                new_x, new_y, data_->get_pixel(old_x, old_y));
                        }
                    }
                }
            }
            if (Fixgap_Flag == true && Inverse_Flag == false)
            {
                std::swap(start_points, end_points);
            }
        }

        // If there are more than two points, there is no need to specially deal
        // with it. If D degenrates to a line: in normal way, it is a line; in
        // inverse way, the new picture degenrates to a line in the old picture,
        // so the picture will be a line that expands vertically to full
        // picture. This shows the difference between normal and inverse way.
        else
        {
            // Calculate the distance between start points (p_i, p_j)
            Eigen::MatrixXd distance_matrix(n, n);
            for (int i = 0; i < n; ++i)
            {
                for (int j = 0; j < n; ++j)
                {
                    distance_matrix(i, j) = std::sqrt(
                        std::pow(start_points[i].x - start_points[j].x, 2) +
                        std::pow(start_points[i].y - start_points[j].y, 2));
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
            std::vector<Eigen::MatrixXd> d_matrix(
                n, Eigen::MatrixXd::Zero(2, 2));
            for (int i = 0; i < n; i++)
            {
                // Calculate D_i matrix by solving linear equations of four
                // variables, D_i_11, D_i_12, D_i_21, D_i_22

                // Calculate A matrix
                Eigen::MatrixXd A = Eigen::MatrixXd::Zero(4, 4);
                for (int j = 0; j < n; j++)
                {
                    A(0, 0) +=
                        sigma_matrix(i, j) *
                        std::pow(start_points[j].x - start_points[i].x, 2);
                    A(0, 1) += sigma_matrix(i, j) *
                               (start_points[j].x - start_points[i].x) *
                               (start_points[j].y - start_points[i].y);
                    A(1, 0) += sigma_matrix(i, j) *
                               (start_points[j].x - start_points[i].x) *
                               (start_points[j].y - start_points[i].y);
                    A(1, 1) +=
                        sigma_matrix(i, j) *
                        std::pow(start_points[j].y - start_points[i].y, 2);
                    A(2, 2) +=
                        sigma_matrix(i, j) *
                        std::pow(start_points[j].x - start_points[i].x, 2);
                    A(2, 3) += sigma_matrix(i, j) *
                               (start_points[j].x - start_points[i].x) *
                               (start_points[j].y - start_points[i].y);
                    A(3, 2) += sigma_matrix(i, j) *
                               (start_points[j].x - start_points[i].x) *
                               (start_points[j].y - start_points[i].y);
                    A(3, 3) +=
                        sigma_matrix(i, j) *
                        std::pow(start_points[j].y - start_points[i].y, 2);
                }

                // Calculate b matrix
                Eigen::VectorXd b = Eigen::VectorXd::Zero(4);
                for (int j = 0; j < n; j++)
                {
                    b(0) += sigma_matrix(i, j) *
                            (start_points[j].x - start_points[i].x) *
                            (end_points[j].x - end_points[i].x);
                    b(1) += sigma_matrix(i, j) *
                            (start_points[j].y - start_points[i].y) *
                            (end_points[j].x - end_points[i].x);
                    b(2) += sigma_matrix(i, j) *
                            (start_points[j].x - start_points[i].x) *
                            (end_points[j].y - end_points[i].y);
                    b(3) += sigma_matrix(i, j) *
                            (start_points[j].y - start_points[i].y) *
                            (end_points[j].y - end_points[i].y);
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
                        // Calculate w_i(p) = sigma_i(p) / (Sum_j sigma_j(p)),
                        // where p = (old_x, old_y)
                        double sigmasum = 0;
                        for (int j = 0; j < n; j++)
                        {
                            if (old_x != start_points[j].x ||
                                old_y != start_points[j].y)
                            {
                                sigmasum +=
                                    1 /
                                    std::pow(
                                        std::sqrt(
                                            std::pow(
                                                old_x - start_points[j].x, 2) +
                                            std::pow(
                                                old_y - start_points[j].y, 2)),
                                        mu);
                            }
                        }
                        double w;
                        if (old_x == start_points[i].x &&
                            old_y == start_points[i].y)
                        {
                            w = 1;
                        }
                        else
                        {
                            w = 1 /
                                std::pow(
                                    std::sqrt(
                                        std::pow(old_x - start_points[i].x, 2) +
                                        std::pow(old_y - start_points[i].y, 2)),
                                    mu) /
                                sigmasum;
                        }

                        // Calculate f_i(p) = q_i + D_i(p - p_i) and sum them up
                        new_x += (int)(w * (end_points[i].x +
                                            d_matrix[i](0, 0) *
                                                (old_x - start_points[i].x) +
                                            d_matrix[i](0, 1) *
                                                (old_y - start_points[i].y)));
                        new_y += (int)(w * (end_points[i].y +
                                            d_matrix[i](1, 0) *
                                                (old_x - start_points[i].x) +
                                            d_matrix[i](1, 1) *
                                                (old_y - start_points[i].y)));
                    }
                    // Set the color of the new pixel
                    if (Inverse_Flag == false)
                    {
                        if (new_x >= 0 && new_x < data_->width() &&
                            new_y >= 0 && new_y < data_->height())
                        {
                            warped_image.set_pixel(
                                new_x, new_y, data_->get_pixel(old_x, old_y));
                        }
                    }
                    else
                    {
                        if (new_x >= 0 && new_x < data_->width() &&
                            new_y >= 0 && new_y < data_->height())
                        {
                            // The difference of inverse: calculate what pixel
                            // is the new one originated from.
                            warped_image.set_pixel(
                                old_x, old_y, data_->get_pixel(new_x, new_y));
                        }
                    }
                }
            }
        }

        // Get it back for further use
        if (Inverse_Flag)
        {
            std::swap(start_points, end_points);
        }
    }
};
}  // namespace USTC_CG