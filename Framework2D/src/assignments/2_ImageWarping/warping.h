#pragma once

#include <memory>
#include <vector>

#include "imgui.h"
#include "view/image.h"

namespace USTC_CG
{

// Class for image warping operations
class Warping
{
   public:
    Warping() = default;
    virtual ~Warping() noexcept = default;

    virtual void warping(
        std::shared_ptr<Image> &data_,
        Image &warped_image,
        std::vector<ImVec2> &start_points,
        std::vector<ImVec2> &end_points,
        bool Inverse_Flag = false,
        bool Fixgap_Flag = false) = 0;
};
}  // namespace USTC_CG