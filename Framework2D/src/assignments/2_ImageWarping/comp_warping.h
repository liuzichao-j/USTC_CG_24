#pragma once

#include "view/comp_image.h"

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
    void gray_scale();

    // HW2_TODO: You should implement your own warping function that interpolate
    // the selected points.
    // You can design a class for such warping operations, utilizing the
    // encapsulation, inheritance, and polymorphism features of C++. More files
    // like "*.h", "*.cpp" can be added to this directory or anywhere you like.
    virtual void warping(bool Inverse_Flag = false) = 0;
    void restore();

    // Point selecting interaction
    void enable_selecting(bool flag);
    void select_points();
    void init_selections();

   protected:
    // Set as protected to let Child Class have access to start_points_ and
    // end_points_.

    // The selected point couples for image warping
    std::vector<ImVec2> start_points_, end_points_;

   private:
    // Store the original image data
    std::shared_ptr<Image> back_up_;

    ImVec2 start_, end_;
    bool flag_enable_selecting_points_ = false;
    bool draw_status_ = false;
};

}  // namespace USTC_CG