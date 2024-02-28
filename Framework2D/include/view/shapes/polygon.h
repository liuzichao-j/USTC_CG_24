#pragma once

#include <vector>

#include "shape.h"

namespace USTC_CG
{
class Polygon : public Shape
{
   public:
    Polygon() = default;

    // Constructor to initialize a Polygon with start coordinates
    Polygon(float start_point_x, float start_point_y,float end_point_x,float end_point_y)
        : end_point_x_(end_point_x),end_point_y_(end_point_y)
    {
        points_x_.clear();
        points_y_.clear();
        points_x_.push_back(start_point_x);
        points_y_.push_back(start_point_y);
    }

    virtual ~Polygon() = default;

    // Overrides draw function to implement polygon-specific drawing logic
    void draw(const Config& config) const override;

    // Overrides Shape's update function to adjust the end point during interaction
    void update(float x, float y) override;

    void addpoint(float x,float y) override;

   private:
    std::vector<float> points_x_, points_y_;
    float end_point_x_, end_point_y_;
};
}  // namespace USTC_CG
