#include "view/shapes/polygon.h"

#include <imgui.h>

namespace USTC_CG
{
/**
 * @brief 绘制多边形形状的函数
 *
 * 绘制的多边形由一系列的顶点坐标组成，通过连接这些顶点形成闭合的多边形。
 * 最后一条边的粗细为前面边的一半。
 *
 * @param config 绘制配置参数，包括偏移量、线条颜色和线条粗细等。
 */
void Polygon::draw(const Config& config) const
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
                (unsigned char)((0.5f+0.5f*cos(conf.time) * cos(conf.time)) *
                                conf.line_color[3])),
            conf.line_thickness);
    }
    draw_list->AddLine(
        ImVec2(
            config.bias[0] + points_x_.at(points_x_.size() - 1),
            config.bias[1] + points_y_.at(points_x_.size() - 1)),
        ImVec2(config.bias[0] + end_point_x_, config.bias[1] + end_point_y_),
        IM_COL32(
            conf.line_color[0],
            conf.line_color[1],
            conf.line_color[2],
            conf.line_color[3]),
        conf.line_thickness);
    draw_list->AddLine(
        ImVec2(config.bias[0] + end_point_x_, config.bias[1] + end_point_y_),
        ImVec2(
            config.bias[0] + points_x_.at(0), config.bias[1] + points_y_.at(0)),
        IM_COL32(
            conf.line_color[0],
            conf.line_color[1],
            conf.line_color[2],
            conf.line_color[3]),
        0.5f * conf.line_thickness);
}

/**
 * @brief 设置新坐标点更新多边形形状
 *
 * 如果鼠标悬停在多边形内部，且按下鼠标左键或右键，函数将添加当前点作为新的顶点。
 * 否则，函数将更新多边形的最后一个顶点坐标。
 *
 * @param x X坐标
 * @param y Y坐标
 */
void Polygon::update(float x, float y)
{
    if (ImGui::IsItemHovered() &&
        (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
         ImGui::IsMouseClicked(ImGuiMouseButton_Right)))
    {
        points_x_.push_back(x);
        points_y_.push_back(y);
        end_point_x_ = points_x_.at(0);
        end_point_y_ = points_y_.at(0);
    }
    else
    {
        end_point_x_ = x;
        end_point_y_ = y;
    }
}

/**
 * @brief 判断点是否在多边形上。
 *
 * @param x X坐标
 * @param y Y坐标
 * @return true 点在多边形上
 * @return false 点不在多边形上
 */
bool Polygon::is_select_on(float x, float y) const
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
        if (fabs(dis1 + dis2 - dis) < 5)
        {
            return true;
        }
    }
    double dis1 = sqrt(
        (points_x_.at(points_x_.size() - 1) - x) *
            (points_x_.at(points_x_.size() - 1) - x) +
        (points_y_.at(points_x_.size() - 1) - y) *
            (points_y_.at(points_x_.size() - 1) - y));
    double dis2 = sqrt(
        (end_point_x_ - x) * (end_point_x_ - x) +
        (end_point_y_ - y) * (end_point_y_ - y));
    double dis = sqrt(
        (points_x_.at(points_x_.size() - 1) - end_point_x_) *
            (points_x_.at(points_x_.size() - 1) - end_point_x_) +
        (points_y_.at(points_x_.size() - 1) - end_point_y_) *
            (points_y_.at(points_x_.size() - 1) - end_point_y_));
    if (fabs(dis1 + dis2 - dis) < 5)
    {
        return true;
    }
    dis1 = sqrt(
        (end_point_x_ - x) * (end_point_x_ - x) +
        (end_point_y_ - y) * (end_point_y_ - y));
    dis2 = sqrt(
        (points_x_.at(0) - x) * (points_x_.at(0) - x) +
        (points_y_.at(0) - y) * (points_y_.at(0) - y));
    dis = sqrt(
        (end_point_x_ - points_x_.at(0)) * (end_point_x_ - points_x_.at(0)) +
        (end_point_y_ - points_y_.at(0)) * (end_point_y_ - points_y_.at(0)));
    if (fabs(dis1 + dis2 - dis) < 5)
    {
        return true;
    }
    return false;
}
}  // namespace USTC_CG