#include "view/shapes/ellipse.h"

#include <imgui.h>
#include <math.h>

namespace USTC_CG
{
// Draw the line using ImGui
void Ellipse::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (conf.filled)
    {
        draw_list->AddEllipseFilled(
            ImVec2(
                config.bias[0] + (start_point_x_ + end_point_x_) / 2,
                config.bias[1] + (start_point_y_ + end_point_y_) / 2),
            (float)fabs((start_point_x_ - end_point_x_) / 2),
            (float)fabs((start_point_y_ - end_point_y_) / 2),
            IM_COL32(
                conf.line_color[0],
                conf.line_color[1],
                conf.line_color[2],
                conf.line_color[3]),
            0.0f,  // rot 旋转角度
            0);    // 线段个数，<=0自动计算
    }
    else
    {
        draw_list->AddEllipse(
            ImVec2(
                config.bias[0] + (start_point_x_ + end_point_x_) / 2,
                config.bias[1] + (start_point_y_ + end_point_y_) / 2),
            (float)fabs((start_point_x_ - end_point_x_) / 2),
            (float)fabs((start_point_y_ - end_point_y_) / 2),
            IM_COL32(
                conf.line_color[0],
                conf.line_color[1],
                conf.line_color[2],
                conf.line_color[3]),
            0.0f,  // rot 旋转角度
            0,     // 线段个数，<=0自动计算
            conf.line_thickness);
    }
}

void Ellipse::update(float x, float y)
{
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
        ImGui::IsKeyDown(ImGuiKey_RightShift))
    {
        if (fabs(start_point_x_ - x) > fabs(start_point_y_ - y))
        {
            end_point_x_ =
                start_point_x_ +
                (x - start_point_x_) *
                    (float)fabs((start_point_y_ - y) / (start_point_x_ - x));
            end_point_y_ = y;
        }
        else
        {
            end_point_x_ = x;
            end_point_y_ =
                start_point_y_ +
                (y - start_point_y_) *
                    (float)fabs((start_point_x_ - x) / (start_point_y_ - y));
        }
    }
    else
    {
        end_point_x_ = x;
        end_point_y_ = y;
    }
}
}  // namespace USTC_CG