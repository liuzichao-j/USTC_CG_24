#pragma once

#include <Eigen/Dense>
#include <cmath>

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
     */
    void warping(
        std::shared_ptr<Image> &data_,
        Image &warped_image,
        std::vector<ImVec2> &start_points,
        std::vector<ImVec2> &end_points,
        bool Inverse_Flag = false) override
    {
        /**
         * TODO: Math formula for RBF warping
         */

        // Calculate a function that maps the end points to the start points,
        // for Inverse_Flag
        if (Inverse_Flag)
        {
            std::swap(start_points, end_points);
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