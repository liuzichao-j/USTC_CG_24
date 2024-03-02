#include "view/shapes/ellipse.h"

#include <imgui.h>
#include <math.h>

namespace USTC_CG
{
/**
 * @brief 绘制椭圆形状的方法。
 *
 * @param config 绘制配置参数，包含偏移量、线条颜色、线条粗细和是否填充等。
 */
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
                (unsigned char)((0.5f +
                                 0.5f * cos(conf.time) * cos(conf.time)) *
                                conf.line_color[3])),
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
                (unsigned char)((0.5f +
                                 0.5f * cos(conf.time) * cos(conf.time)) *
                                conf.line_color[3])),
            0.0f,  // rot 旋转角度
            0,     // 线段个数，<=0自动计算
            conf.line_thickness);
    }
}

/**
 * @brief 设置新坐标点更新椭圆形状
 *
 * 如果按下左shift或右shift键，函数将根据起点和新坐标之间的差异调整椭圆的终点，使其为包含在框选区域内最大的、顶点偏向起点的椭圆。
 *
 * @param x X坐标
 * @param y Y坐标
 */
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

/**
 * @brief 判断点是否在椭圆上，包含椭圆有无填充两种情况。
 *
 * @param x X坐标
 * @param y Y坐标
 * @return true 点在椭圆上
 * @return false 点不在椭圆上
 */
bool Ellipse::is_select_on(float x, float y) const
{
    // 椭圆方程：(x-h)^2/a^2 + (y-k)^2/b^2 = 1
    float a = (float)fabs((start_point_x_ - end_point_x_) / 2);
    float b = (float)fabs((start_point_y_ - end_point_y_) / 2);
    float h = (start_point_x_ + end_point_x_) / 2;
    float k = (start_point_y_ + end_point_y_) / 2;

    float dx = x - h;
    float dy = y - k;

    // 计算result，如果result<=1，则点在椭圆内
    float result = (dx * dx) / (a * a) + (dy * dy) / (b * b);

    if (conf.filled)
    {
        if (result <= 1)
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
        if (fabs(result - 1) <= 0.1)
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