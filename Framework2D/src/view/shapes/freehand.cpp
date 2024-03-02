#include "view/shapes/freehand.h"

#include <imgui.h>

namespace USTC_CG
{
/**
 * @brief 绘制自由手绘图形，图形由一系列线段组成。
 *
 * @param config 绘制配置，包含偏移量、线条颜色和线条粗细等。
 */
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
                conf.line_color[0],
                conf.line_color[1],
                conf.line_color[2],
                (unsigned char)((0.5f +
                                 0.5f * cos(conf.time) * cos(conf.time)) *
                                conf.line_color[3])),
            conf.line_thickness);
    }
}

/**
 * @brief 更新自由手绘图形的点集。
 *
 * 这个函数会在新点与上一个点的差异大于5时调用，将新点添加到自由手绘图形的点集中。
 *
 * @param x X坐标
 * @param y Y坐标
 */
void Freehand::update(float x, float y)
{
    if (abs(x - points_x_.at(points_x_.size() - 1)) > 5 ||
        abs(y - points_y_.at(points_y_.size() - 1)) > 5)
    {
        points_x_.push_back(x);
        points_y_.push_back(y);
    }
}

/**
 * @brief 判断点是否在自由手绘图形上。
 *
 * @param x X坐标
 * @param y Y坐标
 * @return true 点在自由手绘图形上
 * @return false 点不在自由手绘图形上
 */
bool Freehand::is_select_on(float x, float y) const
{
    // 线段的处理方式：用未知点到线段两端点的距离之和与线段的长度的差值判断是否在线段上
    for (int i = 0; i < points_x_.size() - 1; i++)
    {
        double dis1 = sqrt(
            (points_x_.at(i) - x) * (points_x_.at(i) - x) +
            (points_y_.at(i) - y) * (points_y_.at(i) - y));
        double dis2 = sqrt(
            (points_x_.at(i + 1) - x) * (points_x_.at(i + 1) - x) +
            (points_y_.at(i + 1) - y) * (points_y_.at(i + 1) - y));
        double dis = sqrt(
            (points_x_.at(i) - points_x_.at(i + 1)) *
                (points_x_.at(i) - points_x_.at(i + 1)) +
            (points_y_.at(i) - points_y_.at(i + 1)) *
                (points_y_.at(i) - points_y_.at(i + 1)));
        if (dis1 + dis2 - dis < 5)
        {
            return true;
        }
    }
    return false;
}
}  // namespace USTC_CG