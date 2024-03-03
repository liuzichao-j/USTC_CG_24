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
                                conf.line_color
                                    [3])),  // 实现A通道正弦函数变化，范围为0.5倍-1倍
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
        // 两点之间的距离大于5时，添加新点，避免过于密集
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
    // 线段的处理方式：绘制垂直线，若距离小于线宽且交点在线段上（与两端点差值之积小于等于0），则认为点在图形上。
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
    return false;
}
}  // namespace USTC_CG