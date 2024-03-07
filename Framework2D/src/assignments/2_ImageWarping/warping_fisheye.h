#pragma once

#include <cmath>

#include "comp_warping.h"

namespace USTC_CG
{
class WarpingFishEye : public CompWarping
{
   public:
    explicit WarpingFishEye(
        const std::string& label,
        const std::string& filename)
        : CompWarping(label, filename)
    {
    }
    virtual ~WarpingFishEye() noexcept = default;

    /**
     * @brief Warp the image.
     * Inverse warping calculates where the pixel in result image comes from in
     * the original image. It is used to fill in the "gaps" in the result image.
     * @param Inverse_Flag Whether to use the inverse warping function.
     */
    void warping(bool Inverse_Flag = false) override
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
        warping_fisheye(warped_image, Inverse_Flag);
        *data_ = std::move(warped_image);
        update();
    }

   private:
    /**
     * @brief Apply "fish-eye" warping to the image.
     * Use resources from outside the function. 
     * @param warped_image The result image.
     * @param Inverse_Flag Whether to use the inverse warping function.
     */
    Image warping_fisheye(Image& warped_image, bool Inverse_Flag = false)
    {
        // Example: (simplified) "fish-eye" warping
        // For each (x, y) from the input image, the "fish-eye" warping transfer
        // it to (x', y') in the new image: Note: For this transformation
        // ("fish-eye" warping), one can also calculate the inverse (x', y') ->
        // (x, y) to fill in the "gaps".
        for (int y = 0; y < data_->height(); ++y)
        {
            for (int x = 0; x < data_->width(); ++x)
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
    }
};
}  // namespace USTC_CG