#pragma once

#include "shape.h"

namespace USTC_CG
{
class Ellipse : public Shape
{
   public:
    Ellipse() = default;

    // Constructor to initialize a Ellipse with start and end coordinates
    Ellipse(
        float start_point_x,
        float start_point_y,
        float end_point_x,
        float end_point_y)
        : start_point_x_(start_point_x),
          start_point_y_(start_point_y),
          end_point_x_(end_point_x),
          end_point_y_(end_point_y)
    {
    }

    virtual ~Ellipse() = default;

    // Overrides draw function to implement polygon-specific drawing logic
    void draw(const Config& config) const override;

    // Overrides Shape's update function to adjust the end point during
    // interaction
    void update(float x, float y) override;

    int get_shape_type() const override
    {
        return 3;
    }

    bool is_select_on(float x, float y) const override;

   private:
    // Start and end coordinates of the Ellipse
    float start_point_x_, start_point_y_, end_point_x_, end_point_y_;
};
}  // namespace USTC_CG
