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
     * @param Fixgap_Flag_ANN Whether to fix the gaps using ann method.
     * @param Fixgap_Flag_Neighbour Whether to fix the gaps using neighbour
     */
    void warping(
        std::shared_ptr<Image> &data_,
        Image &warped_image,
        std::vector<ImVec2> &start_points,
        std::vector<ImVec2> &end_points,
        bool Inverse_Flag = false,
        bool Fixgap_Flag_ANN = false,
        bool Fixgap_Flag_Neighbour = false) override
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
        int width = data_->width();
        int height = data_->height();
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
        // Build the index, the height of the tree is log2(n)
        index.build((int)log2(n));
        std::vector<float> r_min(n);
        std::vector<int> closest_items;
        std::vector<float> distances;
        for (int i = 0; i < n; i++)
        {
            float p[2] = { start_points[i].x, start_points[i].y };
            index.get_nns_by_vector(p, 2, -1, &closest_items, &distances);
            r_min[i] = distances[1];

            closest_items.clear();
            distances.clear();
        }
        index.unbuild();
        index.reinitialize();

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
                            pow(r_min[j], 2),
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

        // Solve the linear equation
        Eigen::MatrixXd x = T.colPivHouseholderQr().solve(y);

        // Now use index to fix the gap
        int indexcnt = 0;

        // Detect which new pixel is painted
        std::vector<bool> painted(width * height, false);

        // Calculate the warped image. x(i, 0) and x(i, 1) are the coefficients
        // a_i, x(n, 0), x(n, 1), x(n+1, 0), x(n+1, 1) are the coefficients A^T,
        // x(n+2, 0) and x(n+2, 1) are the coefficients b
        for (int old_x = 0; old_x < width; old_x++)
        {
            for (int old_y = 0; old_y < height; old_y++)
            {
                double new_x = 0;
                double new_y = 0;

                // Calculate the new position of the pixel
                for (int i = 0; i < n; i++)
                {
                    new_x +=
                        x(i, 0) * pow(pow(old_x - start_points[i].x, 2) +
                                          pow(old_y - start_points[i].y, 2) +
                                          pow(r_min[i], 2),
                                      mu / 2);
                    new_y +=
                        x(i, 1) * pow(pow(old_x - start_points[i].x, 2) +
                                          pow(old_y - start_points[i].y, 2) +
                                          pow(r_min[i], 2),
                                      mu / 2);
                }
                new_x += old_x * x(n, 0) + old_y * x(n + 1, 0) + x(n + 2, 0);
                new_y += old_x * x(n, 1) + old_y * x(n + 1, 1) + x(n + 2, 1);
                // Set the color of the new pixel
                if (Inverse_Flag == false)
                {
                    if (new_x >= 0 && new_x < width && new_y >= 0 &&
                        new_y < height)
                    {
                        warped_image.set_pixel(
                            (int)new_x,
                            (int)new_y,
                            data_->get_pixel(old_x, old_y));
                        painted[(int)new_y * width + (int)new_x] = true;
                        if (Fixgap_Flag_ANN == true)
                        {
                            float p[2] = { (float)(int)new_x,
                                           (float)(int)new_y };
                            // Add the painted pixel to the index. Choose id as
                            // y * width + x, to easily find out the pixel by id
                            index.add_item((int)new_y * width + (int)new_x, p);
                            indexcnt++;
                        }
                    }
                }
                else
                {
                    if (new_x >= 0 && new_x < width && new_y >= 0 &&
                        new_y < height)
                    {
                        // The difference of inverse: calculate what pixel is
                        // the new one originated from.
                        warped_image.set_pixel(
                            old_x,
                            old_y,
                            data_->get_pixel((int)new_x, (int)new_y));
                    }
                }
            }
        }

        // Calculate the max distance to search, avoid paint at non-sight area.
        // Choose the maximum distance as the zoom ratio.
        double oldmindis = (width > height ? width : height), newmaxdis = 0;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                double dis = std::sqrt(
                    std::pow(start_points[i].x - start_points[j].x, 2) +
                    std::pow(start_points[i].y - start_points[j].y, 2));
                if (i != j && dis < oldmindis)
                {
                    oldmindis = dis;
                }
            }
        }
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                double dis = std::sqrt(
                    std::pow(end_points[i].x - end_points[j].x, 2) +
                    std::pow(end_points[i].y - end_points[j].y, 2));
                if (dis > newmaxdis)
                {
                    newmaxdis = dis;
                }
            }
        }
        double max_distance = newmaxdis / oldmindis;

        // Fix the gap by ann method
        if (Fixgap_Flag_ANN == true && indexcnt)
        {
            // Build the index, the height of the tree is log2(indexcnt)
            index.build((int)log2(indexcnt));

            int k = 3;  // search k nearest points
            std::vector<int> closest_points;
            std::vector<float> distances;
            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    if (painted[j * width + i])
                    {
                        // Painted, continue
                        continue;
                    }

                    // search the k nearest points for each new pixel
                    float p[2] = { (float)i, (float)j };
                    index.get_nns_by_vector(
                        p, k, -1, &closest_points, &distances);

                    std::vector<unsigned char> red(0), green(0), blue(0);
                    std::vector<int> cnt(0);
                    for (int l = 0; l < distances.size(); l++)
                    {
                        if (distances[l] > max_distance)
                        {
                            break;
                        }
                        // Within the max distance, count the color
                        std::vector<unsigned char> pixel =
                            warped_image.get_pixel(
                                closest_points[l] % width,
                                closest_points[l] / width);
                        bool flag = false;
                        for (int m = 0; m < cnt.size(); m++)
                        {
                            if (pixel[0] == red[m] && pixel[1] == green[m] &&
                                pixel[2] == blue[m])
                            {
                                flag = true;
                                cnt[m]++;
                                break;
                            }
                        }
                        if (flag == false)
                        {
                            red.push_back(pixel[0]);
                            green.push_back(pixel[1]);
                            blue.push_back(pixel[2]);
                            cnt.push_back(1);
                        }
                    }

                    // find the most frequent color and give it to the gap
                    // pixel
                    int maxcnt = 0, maxindex = -1;
                    for (int l = 0; l < cnt.size(); l++)
                    {
                        if (cnt[l] > maxcnt)
                        {
                            maxcnt = cnt[l];
                            maxindex = l;
                        }
                    }
                    if (maxindex != -1)
                    {
                        std::vector<unsigned char> pixel = { red[maxindex],
                                                             green[maxindex],
                                                             blue[maxindex] };
                        warped_image.set_pixel(i, j, pixel);
                    }
                    closest_points.clear();
                    distances.clear();
                }
            }
        }
        index.unbuild();
        index.reinitialize();

        if (Fixgap_Flag_Neighbour == true)
        {
            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    if (painted[j * width + i])
                    {
                        continue;
                    }
                    std::vector<unsigned char> red(0), green(0), blue(0);
                    std::vector<int> cnt(0);

                    // Search the painted pixels within the max_distance
                    for (int x = i - (int)(max_distance / 2);
                         x <= i + (int)(max_distance / 2);
                         x++)
                    {
                        for (int y = j - (int)(max_distance / 2);
                             y <= j + (int)(max_distance / 2);
                             y++)
                        {
                            if (x < 0 || x >= width || y < 0 || y >= height)
                            {
                                continue;
                            }
                            if (painted[y * width + x])
                            {
                                std::vector<unsigned char> pixel =
                                    warped_image.get_pixel(x, y);
                                bool flag = false;
                                for (int m = 0; m < cnt.size(); m++)
                                {
                                    if (pixel[0] == red[m] &&
                                        pixel[1] == green[m] &&
                                        pixel[2] == blue[m])
                                    {
                                        flag = true;
                                        cnt[m]++;
                                        break;
                                    }
                                }
                                if (flag == false)
                                {
                                    red.push_back(pixel[0]);
                                    green.push_back(pixel[1]);
                                    blue.push_back(pixel[2]);
                                    cnt.push_back(1);
                                }
                            }
                        }
                    }

                    // find the most frequent color and give it to the gap pixel
                    int maxcnt = 0, maxindex = -1;
                    for (int l = 0; l < cnt.size(); l++)
                    {
                        if (cnt[l] > maxcnt)
                        {
                            maxcnt = cnt[l];
                            maxindex = l;
                        }
                    }
                    if (maxindex != -1)
                    {
                        std::vector<unsigned char> pixel = { red[maxindex],
                                                             green[maxindex],
                                                             blue[maxindex] };
                        warped_image.set_pixel(i, j, pixel);
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