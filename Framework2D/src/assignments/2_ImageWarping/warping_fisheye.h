#pragma once

#include <cmath>

#include "annoylib.h"
#include "kissrandom.h"
#include "warping.h"

namespace USTC_CG
{
class WarpingFishEye : public Warping
{
   public:
    WarpingFishEye() = default;
    virtual ~WarpingFishEye() noexcept = default;

    /**
     * @brief Warp the image using the "fish-eye" warping function.
     * Inverse warping calculates where the pixel in result image comes from in
     * the original image. It is used to fill in the "gaps" in the result image.
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
        // Example: (simplified) "fish-eye" warping
        // For each (x, y) from the input image, the "fish-eye" warping transfer
        // it to (x', y') in the new image: Note: For this transformation
        // ("fish-eye" warping), one can also calculate the inverse (x', y') ->
        // (x, y) to fill in the "gaps".

        // Fix the gap using ann method, proves to be very slow
        Annoy::AnnoyIndex<
            int,
            float,
            Annoy::Euclidean,
            Annoy::Kiss32Random,
            Annoy::AnnoyIndexSingleThreadedBuildPolicy>
            index(2);
        int indexcnt = 0;

        // Detect which new pixel is painted
        std::vector<bool> painted(data_->width() * data_->height(), false);

        for (int x = 0; x < data_->width(); ++x)
        {
            for (int y = 0; y < data_->height(); ++y)
            {
                // Apply warping function to (x, y), and we can get (x',
                // y')
                int width = data_->width();
                int height = data_->height();
                float center_x = width / 2.0f;
                float center_y = height / 2.0f;
                float dx = x - center_x;
                float dy = y - center_y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (Inverse_Flag == false)
                {
                    // Simple non-linear transformation r -> r' = f(r)
                    float new_distance = std::sqrt(distance) * 10;
                    int new_x = 0;
                    int new_y = 0;

                    if (distance == 0)
                    {
                        new_x = static_cast<int>(center_x);
                        new_y = static_cast<int>(center_y);
                    }
                    else
                    {
                        // (x', y')
                        float ratio = new_distance / distance;
                        new_x = static_cast<int>(center_x + dx * ratio);
                        new_y = static_cast<int>(center_y + dy * ratio);
                    }

                    // Copy the color from the original image to the result
                    // image
                    if (new_x >= 0 && new_x < width && new_y >= 0 &&
                        new_y < height)
                    {
                        std::vector<unsigned char> pixel =
                            data_->get_pixel(x, y);
                        warped_image.set_pixel(new_x, new_y, pixel);
                        painted[new_y * width + new_x] = true;
                        // Store given pixels for ann method
                        if (Fixgap_Flag_ANN == true)
                        {
                            float p[2] = { (float)new_x, (float)new_y };
                            index.add_item(new_y * width + new_x, p);
                            indexcnt++;
                            // printf("add item index %d, place %f, %f\n", new_y
                            // * width + new_x, p[0], p[1]);
                        }
                    }
                }
                else
                {
                    // Another way: Inverse (x', y') -> (x, y)
                    float old_distance = distance * distance / 100;
                    int old_x;
                    int old_y;

                    if (distance == 0)
                    {
                        old_x = static_cast<int>(center_x);
                        old_y = static_cast<int>(center_y);
                    }
                    else
                    {
                        // (x, y)
                        float ratio = old_distance / distance;
                        old_x = static_cast<int>(center_x + dx * ratio);
                        old_y = static_cast<int>(center_y + dy * ratio);
                    }

                    // Copy the color from the original image to the result
                    // image
                    if (old_x >= 0 && old_x < width && old_y >= 0 &&
                        old_y < height)
                    {
                        std::vector<unsigned char> pixel =
                            data_->get_pixel(old_x, old_y);
                        warped_image.set_pixel(x, y, pixel);
                    }
                }
            }
        }

        // Fix the gap using ann method
        if (Fixgap_Flag_ANN == true && indexcnt)
        {
            index.build((int)log2(indexcnt));
            int k = 3;  // search k nearest points
            float max_distance =
                2.0f;  // max distance to search, avoid paint at non-sight area
            std::vector<int> closest_points;
            std::vector<float> distances;
            for (int i = 0; i < data_->width(); i++)
            {
                for (int j = 0; j < data_->height(); j++)
                {
                    if (painted[j * data_->width() + i])
                    {
                        continue;
                    }
                    float p[2] = { (float)i, (float)j };
                    index.get_nns_by_vector(
                        p, k, -1, &closest_points, &distances);

                    // search the k nearest points for each new pixel
                    std::vector<unsigned char> red(0), green(0), blue(0);
                    std::vector<int> cnt(0);
                    for (int l = 0; l < distances.size(); l++)
                    {
                        if (distances[l] > max_distance)
                        {
                            continue;
                        }
                        // Within the max distance, count the color
                        std::vector<unsigned char> pixel =
                            warped_image.get_pixel(
                                closest_points[l] % data_->width(),
                                closest_points[l] / data_->width());
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
                    closest_points.clear();
                    distances.clear();
                }
            }
        }
        index.unbuild();
        index.reinitialize();

        if (Fixgap_Flag_Neighbour == true)
        {
            int max_distance = 2;
            for (int i = 0; i < data_->width(); i++)
            {
                for (int j = 0; j < data_->height(); j++)
                {
                    if (painted[j * data_->width() + i])
                    {
                        continue;
                    }
                    std::vector<unsigned char> red(0), green(0), blue(0);
                    std::vector<int> cnt(0);
                    for (int x = i - max_distance / 2;
                         x <= i + max_distance / 2;
                         x++)
                    {
                        for (int y = j - max_distance / 2;
                             y <= j + max_distance / 2;
                             y++)
                        {
                            if (x < 0 || x >= data_->width() || y < 0 ||
                                y >= data_->height())
                            {
                                continue;
                            }
                            if (painted[y * data_->width() + x])
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
    }
};
}  // namespace USTC_CG