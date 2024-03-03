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
                (unsigned char)((0.5f +
                                 0.5f * cos(conf.time) * cos(conf.time)) *
                                conf.line_color
                                    [3])),  // 实现A通道正弦函数变化，范围为0.5倍-1倍
            conf.line_thickness);
    }

    // 下面无需使用闪烁，因为绘制时必然不在Select模式下
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
    // 线段的处理方式：绘制垂直线，若距离小于线宽且交点在线段上（与两端点差值之积小于等于0），则认为点在图形上
    // 引入正态分布函数，使得线宽越小判断越宽松，线宽越大判断越严格，便于用户操作。
    for (int i = 0; i < points_x_.size() - 1; i++)
    {
        double k1 = (points_y_.at(i + 1) - points_y_.at(i)) /
                    (points_x_.at(i + 1) - points_x_.at(i));
        double b1 = points_y_.at(i) - k1 * points_x_.at(i);
        double k2 = -1 / k1;
        double b2 = y - k2 * x;
        double cross_x = (b2 - b1) / (k1 - k2);
        double dis = sqrt(
            (cross_x - x) * (cross_x - x) +
            (k1 * cross_x + b1 - y) * (k1 * cross_x + b1 - y));
        if (dis < conf.line_thickness * 0.5f *
                      (1.0f + 9 * exp(-(conf.line_thickness - 1.0f) *
                                   (conf.line_thickness - 1.0f) / 4)) &&
            ((cross_x - points_x_.at(i)) * (cross_x - points_x_.at(i + 1))) <=
                0)
        {
            return true;
        }
    }
    double k1 = (end_point_y_ - points_y_.at(points_x_.size() - 1)) /
                (end_point_x_ - points_x_.at(points_x_.size() - 1));
    double b1 = points_y_.at(points_x_.size() - 1) -
                k1 * points_x_.at(points_x_.size() - 1);
    double k2 = -1 / k1;
    double b2 = y - k2 * x;
    double cross_x = (b2 - b1) / (k1 - k2);
    double dis = sqrt(
        (cross_x - x) * (cross_x - x) +
        (k1 * cross_x + b1 - y) * (k1 * cross_x + b1 - y));
    if (dis < conf.line_thickness * 0.5f * 
                  (1.0f + 9 * exp(-(conf.line_thickness - 1.0f) *
                               (conf.line_thickness - 1.0f) / 4)) &&
        ((cross_x - points_x_.at(points_x_.size() - 1)) *
         (cross_x - end_point_x_)) <= 0)
    {
        return true;
    }
    k1 = (points_y_.at(0) - end_point_y_) / (points_x_.at(0) - end_point_x_);
    b1 = end_point_y_ - k1 * end_point_x_;
    k2 = -1 / k1;
    b2 = y - k2 * x;
    cross_x = (b2 - b1) / (k1 - k2);
    dis = sqrt(
        (cross_x - x) * (cross_x - x) +
        (k1 * cross_x + b1 - y) * (k1 * cross_x + b1 - y));
    if (dis < conf.line_thickness * 0.5f * 
                  (1.0f + 9 * exp(-(conf.line_thickness - 1.0f) *
                               (conf.line_thickness - 1.0f) / 4)) &&
        ((cross_x - end_point_x_) * (cross_x - points_x_.at(0))) <= 0)
    {
        return true;
    }
    return false;
}
}  // namespace USTC_CG