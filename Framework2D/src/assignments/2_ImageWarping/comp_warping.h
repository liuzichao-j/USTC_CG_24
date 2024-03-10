#pragma once

#include "view/comp_image.h"
#include "warping.h"

namespace USTC_CG
{
// Image component for warping and other functions
class CompWarping : public ImageEditor
{
   public:
    explicit CompWarping(const std::string& label, const std::string& filename);
    virtual ~CompWarping() noexcept = default;

    void draw() override;

    // Simple edit functions
    void invert();
    void mirror(bool is_horizontal, bool is_vertical);
    void gray_scale(int method = 0);

    void warping();
    void restore();

    // Point selecting interaction
    void enable_selecting(bool flag);
    void select_points();
    void init_selections();

    // Set warping method
    void set_warping_method(int method);

    // Whether to use the inverse warping function, default is false.
    bool inverse_flag = false;
    // Whether to fix the gaps in normal mode, default is false.
    bool fixgap_flag_ann = false;
    bool fixgap_flag_neighbour = false;

    // Whether to show the points
    bool flag_enable_selecting_points_ = false;

   private:
    // Store the original image data
    std::shared_ptr<Image> back_up_;
    // The selected point couples for image warping
    std::vector<ImVec2> start_points_, end_points_;
    // Warping element for implementation of warping functions
    std::shared_ptr<Warping> warping_;

    ImVec2 start_, end_;
    bool draw_status_ = false;
};
}  // namespace USTC_CG