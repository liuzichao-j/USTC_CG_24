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
                (unsigned char)((0.5f +
                                 0.5f * cos(conf.time) * cos(conf.time)) *
                                conf.line_color
                                    [3])),  // 实现A通道正弦函数变化，范围为0.5倍-1倍
            0.f,                            // No rounding of corners
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
                (unsigned char)((0.5f +
                                 0.5f * cos(conf.time) * cos(conf.time)) *
                                conf.line_color
                                    [3])),  // 实现A通道正弦函数变化，范围为0.5倍-1倍
            0.f,                            // No rounding of corners
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
    if (conf.filled)
    {
        if ((x - start_point_x_) * (x - end_point_x_) <= 0 &&
            (y - start_point_y_) * (y - end_point_y_) <= 0)
        {
            return true;
        }
        return false;
    }
    else
    {
        // 分别判断四条边上的点，若距离小于线宽且另外一个坐标在两端点之间（差值乘积小于等于0），则认为点在图形上。
        // 引入正态分布函数，使得线宽越小判断越宽松，线宽越大判断越严格，便于用户操作。
        if (fabs(x - start_point_x_) <
                conf.line_thickness * 0.5f *
                    (1.0f + 9 * exp(-(conf.line_thickness - 1.0f) *
                                 (conf.line_thickness - 1.0f) / 4)) &&
            (y - start_point_y_) * (y - end_point_y_) <= 0)
        {
            return true;
        }
        if (fabs(x - end_point_x_) <
                conf.line_thickness * 0.5f *
                    (1.0f + 9 * exp(-(conf.line_thickness - 1.0f) *
                                 (conf.line_thickness - 1.0f) / 4)) &&
            (y - start_point_y_) * (y - end_point_y_) <= 0)
        {
            return true;
        }
        if (fabs(y - start_point_y_) <
                conf.line_thickness * 0.5f *
                    (1.0f + 9 * exp(-(conf.line_thickness - 1.0f) *
                                 (conf.line_thickness - 1.0f) / 4)) &&
            (x - start_point_x_) * (x - end_point_x_) <= 0)
        {
            return true;
        }
        if (fabs(y - end_point_y_) <
                conf.line_thickness * 0.5f *
                    (1.0f + 9 * exp(-(conf.line_thickness - 1.0f) *
                                 (conf.line_thickness - 1.0f) / 4)) &&
            (x - start_point_x_) * (x - end_point_x_) <= 0)
        {
            return true;
        }
    }
    return false;
}
}  // namespace USTC_CG
