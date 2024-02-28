#include "view/shapes/freehand.h"

#include <imgui.h>

namespace USTC_CG
{
// Draw the line using ImGui
void Freehand::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int i = 0; i < points_x_.size() - 1; i++)
    {
        draw_list->AddLine(
            ImVec2(
                config.bias[0] + points_x_.at(i),
                config.bias[1] + points_y_.at(i)),
            ImVec2(
                config.bias[0] + points_x_.at(i + 1),
                config.bias[1] + points_y_.at(i + 1)),
            IM_COL32(
                config.line_color[0],
                config.line_color[1],
                config.line_color[2],
                config.line_color[3]),
            config.line_thickness);
    }
}

void Freehand::update(float x, float y)
{
    if (x != points_x_.at(points_x_.size() - 1) ||
        y != points_y_.at(points_y_.size() - 1))
    {
        points_x_.push_back(x);
        points_y_.push_back(y);
    }
}
}  // namespace USTC_CG