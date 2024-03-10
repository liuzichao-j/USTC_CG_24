#pragma once

#include <Eigen/Dense>
#include <cmath>

#include "annoylib.h"
#include "kissrandom.h"
#include "warping.h"

namespace USTC_CG
{
class WarpingRBF : public Warping
{
   public:
    WarpingRBF() = default;
    virtual ~WarpingRBF() noexcept = default;

    /**
     * @brief Radial Basis Function (RBF) warping function.
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
         * We follow the formulas in homework guide.
         *
         * f(p) = Sum_i a_i*R(||p-p_i||) + A*p + b, where R(d) = (d^2 +
         * r_min^2)^(mu/2) is the RBF function, r_min = min_(j!=i) ||p_j -
         * p_i||, A*p + b is the affine transformation.
         *
         * Follow the guidance, we implement a restriction that: (p_1 ... p_n) *
         * (a_i^T) = 0, (1 ... 1) * (a_i^T) = 0
         *
         * Thus, we can get a matrix formula T * x = y. T(n+3)*(n+3), x(n+3)*2,
         * y(n+3)*2
         *
         * T = {{R(0,0), R(0,1), ..., R(0,n-1), p_0_1, p_0_2, 1}, {R(1,0), ...,
         * R(1,n-1), p_1_1, p_1_2, 1}, ..., {p_0_1, ..., p_(n-1)_1, 0, 0, 0},
         * {p_0_2, ..., p_(n-1)_2, 0, 0, 0}, {1, ..., 1, 0, 0, 0}}
         *
         * x = {{a_0_1, a_0_2}, ..., {a_(n-1)_1, a_(n-1)_2}, {A_00, A_10},
         * {A_01, A_11}, {b_1, b_2}}
         *
         * y = {{q_0_1, q_0_2}, ..., {q_(n-1)_1, q_(n-1)_2}, {0, 0}, {0, 0}, {0,
         * 0}}
         *
         * Solve the linear equation, we can get a_i, A, b, and f(p).
         */

        // Calculate a function that maps the end points to the start points,
        // for Inverse_Flag
        if (Inverse_Flag)
        {
            std::swap(start_points, end_points);
        }

        int n = (int)start_points.size();
        double mu = 1.0f;

        // Calculate r_min_i = min_(j!=i) ||p_j - p_i||, use ann method
        Annoy::AnnoyIndex<
            int,
            float,
            Annoy::Euclidean,
            Annoy::Kiss32Random,
            Annoy::AnnoyIndexSingleThreadedBuildPolicy>
            index(2);
        for (int i = 0; i < n; i++)
        {
            float p[2] = { (float)start_points[i].x, (float)start_points[i].y };
            index.add_item(i, p);
        }
        index.build((int)log2(n));
        std::vector<float> r_min(n);
        std::vector<int> closest_items;
        std::vector<float> distances;
        for (int i = 0; i < n; i++)
        {
            float p[2] = { start_points[i].x, start_points[i].y };
            index.get_nns_by_vector(p, 2, -1, &closest_items, &distances);
            r_min[i] = distances[1];
            printf(
                "get nns for %d: %d, %f; %d, %f, %f\n",
                i,
                closest_items[0],
                distances[0],
                closest_items[1],
                r_min[i],
                sqrt(
                    pow(start_points[i].x - start_points[closest_items[1]].x,
                        2) +
                    pow(start_points[i].y - start_points[closest_items[1]].y,
                        2)));

            closest_items.clear();
            distances.clear();
        }

        // Calculate T and y
        Eigen::MatrixXd T = Eigen::MatrixXd::Zero(n + 3, n + 3);
        Eigen::MatrixXd y = Eigen::MatrixXd::Zero(n + 3, 2);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                T(i, j) =
                    pow(pow(start_points[i].x - start_points[j].x, 2) +
                            pow(start_points[i].y - start_points[j].y, 2) +
                            pow(r_min[i], 2),
                        mu / 2);
            }
            T(i, n) = start_points[i].x;
            T(i, n + 1) = start_points[i].y;
            T(i, n + 2) = 1;
            y(i, 0) = end_points[i].x;
            y(i, 1) = end_points[i].y;
        }
        for (int j = 0; j < n; j++)
        {
            T(n, j) = start_points[j].x;
        }
        for (int j = 0; j < n; j++)
        {
            T(n + 1, j) = start_points[j].y;
        }
        for (int j = 0; j < n; j++)
        {
            T(n + 2, j) = 1;
        }

        printf("T matrix: \n");
        for (int i = 0; i < n + 3; i++)
        {
            for (int j = 0; j < n + 3; j++)
            {
                printf("%f ", T(i, j));
            }
            printf("\n");
        }
        printf("y matrix: \n");
        for (int i = 0; i < n + 3; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                printf("%f ", y(i, j));
            }
            printf("\n");
        }

        // Solve the linear equation
        Eigen::MatrixXd x = T.colPivHouseholderQr().solve(y);

        auto t1=T.operator*(x).operator-(y);
        printf("error matrix: \n");
        for(int i=0;i<t1.rows();i++)
        {
            for(int j=0;j<t1.cols();j++)
            {
                printf("%f ",t1(i,j));
            }
            printf("\n");
        }

        printf("Solved\n");
        for (int i = 0; i < n; i++)
        {
            printf("a_%d = (%f, %f)\n", i, x(i, 0), x(i, 1));
        }
        printf(
            "A = ((%f, %f), (%f, %f))\n",
            x(n, 0),
            x(n + 1, 0),
            x(n, 1),
            x(n + 1, 1));
        printf("b = (%f, %f)\n", x(n + 2, 0), x(n + 2, 1));

        // Calculate the warped image. x(i, 0) and x(i, 1) are the coefficients
        // a_i, x(n, 0), x(n, 1), x(n+1, 0), x(n+1, 1) are the coefficients A^T,
        // x(n+2, 0) and x(n+2, 1) are the coefficients b
        for (int old_x = 0; old_x < data_->width(); old_x++)
        {
            for (int old_y = 0; old_y < data_->height(); old_y++)
            {
                int new_x = 0;
                int new_y = 0;

                // Calculate the new position of the pixel
                for (int i = 0; i < n; i++)
                {
                    new_x += (int)(x(i, 0) *
                                   pow(pow(old_x - start_points[i].x, 2) +
                                           pow(old_y - start_points[i].y, 2) +
                                           pow(r_min[i], 2),
                                       mu / 2));
                    new_y += (int)(x(i, 1) *
                                   pow(pow(old_x - start_points[i].x, 2) +
                                           pow(old_y - start_points[i].y, 2) +
                                           pow(r_min[i], 2),
                                       mu / 2));
                }
                new_x +=
                    (int)(old_x * x(n, 0) + old_y * x(n + 1, 0) + x(n + 2, 0));
                new_y +=
                    (int)(old_x * x(n, 1) + old_y * x(n + 1, 1) + x(n + 2, 1));
                // Set the color of the new pixel
                if (Inverse_Flag == false)
                {
                    if (new_x >= 0 && new_x < data_->width() && new_y >= 0 &&
                        new_y < data_->height())
                    {
                        warped_image.set_pixel(
                            new_x, new_y, data_->get_pixel(old_x, old_y));
                    }
                }
                else
                {
                    if (new_x >= 0 && new_x < data_->width() && new_y >= 0 &&
                        new_y < data_->height())
                    {
                        // The difference of inverse: calculate what pixel is
                        // the new one originated from.
                        warped_image.set_pixel(
                            old_x, old_y, data_->get_pixel(new_x, new_y));
                    }
                }
            }
        }

        // TODO: Implement the RBF warping function

        // Get it back for further use
        if (Inverse_Flag)
        {
            std::swap(start_points, end_points);
        }
    }
};
}  // namespace USTC_CG