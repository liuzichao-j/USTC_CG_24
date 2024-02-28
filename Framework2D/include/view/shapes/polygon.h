#pragma once

#include "shape.h"
#include<vector>

namespace USTC_CG
{
class Polygon : public Shape
{
   public:
    Polygon() = default;

    virtual ~Polygon() = default;

    // Overrides draw function to implement line-specific drawing logic
    void draw(const Config& config) const override;

    // Overrides Shape's update function to adjust the end point during
    // interaction
    void update(float x, float y) override;

   private:
    std::vector<std::shared_ptr<Line>> shape_list_;
};
}  // namespace USTC_CG
