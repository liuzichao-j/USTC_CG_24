#include "view/shapes/line.h"

#include <imgui.h>
#include <math.h>

namespace USTC_CG
{
/**
 * @brief 绘制直线形状的方法。
 *
 * @param config 绘制配置参数，包含偏移量、线条颜色和线条粗细等。
 */
void Line::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddLine(
        ImVec2(
            config.bias[0] + start_point_x_, config.bias[1] + start_point_y_),
        ImVec2(config.bias[0] + end_point_x_, config.bias[1] + end_point_y_),
        IM_COL32(
            conf.line_color[0],
            conf.line_color[1],
            conf.line_color[2],
            (unsigned char)((0.5f+0.5f*cos(conf.time) * cos(conf.time)) *
                                conf.line_color[3])),
        conf.line_thickness);
}

/**
 * @brief 设置新坐标点更新直线形状
 *
 * @param x X坐标
 * @param y Y坐标
 */
void Line::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

/**
 * @brief 判断点是否在直线上。
 *
 * @param x X坐标
 * @param y Y坐标
 * @return true 点在直线上
 * @return false 点不在直线上
 */
bool Line::is_select_on(float x, float y) const
{
    // 线段的处理方式：用未知点到线段两端点的距离之和与线段的长度的差值判断是否在线段上
    double dis1 = sqrt(
        (x - start_point_x_) * (x - start_point_x_) +
        (y - start_point_y_) * (y - start_point_y_));
    double dis2 = sqrt(
        (x - end_point_x_) * (x - end_point_x_) +
        (y - end_point_y_) * (y - end_point_y_));
    double dis = sqrt(
        (start_point_x_ - end_point_x_) * (start_point_x_ - end_point_x_) +
        (start_point_y_ - end_point_y_) * (start_point_y_ - end_point_y_));
    if (fabs(dis - (dis1 + dis2)) < 5)
    {
        return true;
    }
    return false;
}
}  // namespace USTC_CG