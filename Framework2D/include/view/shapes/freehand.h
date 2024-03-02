#pragma once

#include <vector>

#include "shape.h"

namespace USTC_CG
{
class Freehand : public Shape
{
   public:
    Freehand() = default;

    // Initializing with start coordinates
    Freehand(float point_x, float point_y)
    {
        points_x_.clear();
        points_y_.clear();
        points_x_.push_back(point_x);
        points_y_.push_back(point_y);
    }

    virtual ~Freehand() = default;

    // Overrides draw function to implement freehand drawing logic
    void draw(const Config& config) const override;

    // Overrides Shape's update function to adjust the end point during
    // interaction
    void update(float x, float y) override;

    int get_shape_type() const override
    {
        return 5;
    }

    bool is_select_on(float x, float y) const override;

   private:
    // A series of points that make up the freehand shape
    std::vector<float> points_x_, points_y_;
};
}  // namespace USTC_CG
