#include "view/shapes/rect.h"

#include <imgui.h>
#include <math.h>

namespace USTC_CG
{

/**
 * @brief 绘制矩形形状的方法。
 *
 * @param config 绘制配置参数，包含偏移量、线条颜色、线条粗细和是否填充等。
 */
void Rect::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (conf.filled)
    {
        draw_list->AddRectFilled(
            ImVec2(
                config.bias[0] + start_point_x_,
                config.bias[1] + start_point_y_),
            ImVec2(
                config.bias[0] + end_point_x_, config.bias[1] + end_point_y_),
            IM_COL32(
                conf.line_color[0],
                conf.line_color[1],
                conf.line_color[2],
                (unsigned char)((0.5f+0.5f*cos(conf.time) * cos(conf.time)) *
                                conf.line_color[3])),
            0.f,  // No rounding of corners
            ImDrawFlags_None);
    }
    else
    {
        draw_list->AddRect(
            ImVec2(
                config.bias[0] + start_point_x_,
                config.bias[1] + start_point_y_),
            ImVec2(
                config.bias[0] + end_point_x_, config.bias[1] + end_point_y_),
            IM_COL32(
                conf.line_color[0],
                conf.line_color[1],
                conf.line_color[2],
                (unsigned char)((0.5f+0.5f*cos(conf.time) * cos(conf.time)) *
                                conf.line_color[3])),
            0.f,  // No rounding of corners
            ImDrawFlags_None,
            conf.line_thickness);
    }
}

/**
 * @brief 设置新坐标点更新矩形形状
 *
 * 如果按下左shift或右shift键，函数将根据起点和新坐标之间的差异调整矩形的终点，使其为包含在框选区域内最大的、顶点偏向起点的正方形。
 *
 * @param x X坐标
 * @param y Y坐标
 */
void Rect::update(float x, float y)
{
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
        ImGui::IsKeyDown(ImGuiKey_RightShift))
    {
        if (fabs(x - start_point_x_) < fabs(y - start_point_y_))
        {
            end_point_x_ = x;
            end_point_y_ =
                start_point_y_ +
                (y - start_point_y_) *
                    (float)fabs((start_point_x_ - x) / (start_point_y_ - y));
        }
        else
        {
            end_point_y_ = y;
            end_point_x_ =
                start_point_x_ +
                (x - start_point_x_) *
                    (float)fabs((start_point_y_ - y) / (start_point_x_ - x));
        }
    }
    else
    {
        end_point_x_ = x;
        end_point_y_ = y;
    }
}

/**
 * @brief 判断点是否在矩形上。
 *
 * @param x X坐标
 * @param y Y坐标
 * @return true 点在矩形上
 * @return false 点不在矩形上
 */
bool Rect::is_select_on(float x, float y) const
{
    if(conf.filled)
    {
        if(x > start_point_x_ && x < end_point_x_ && y > start_point_y_ && y < end_point_y_)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if(fabs(x - start_point_x_) < 5 && y > start_point_y_ && y < end_point_y_)
        {
            return true;
        }
        else if(fabs(x - end_point_x_) < 5 && y > start_point_y_ && y < end_point_y_)
        {
            return true;
        }
        else if(fabs(y - start_point_y_) < 5 && x > start_point_x_ && x < end_point_x_)
        {
            return true;
        }
        else if(fabs(y - end_point_y_) < 5 && x > start_point_x_ && x < end_point_x_)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}
}  // namespace USTC_CG
