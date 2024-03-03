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
            (unsigned char)((0.5f + 0.5f * cos(conf.time) * cos(conf.time)) *
                            conf.line_color
                                [3])),  // 实现A通道正弦函数变化，范围为0.5倍-1倍
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
    // 线段的处理方式：绘制垂直线，若距离小于线宽且交点在线段上（与两端点差值之积小于等于0），则认为点在图形上
    // 引入正态分布函数，使得线宽越小判断越宽松，线宽越大判断越严格，便于用户操作。
    double k1 =
        (end_point_y_ - start_point_y_) / (end_point_x_ - start_point_x_);
    double b1 = start_point_y_ - k1 * start_point_x_;
    double k2 = -1 / k1;
    double b2 = y - k2 * x;
    double cross_x = (b2 - b1) / (k1 - k2);
    double dis = sqrt(
        (cross_x - x) * (cross_x - x) +
        (k1 * cross_x + b1 - y) * (k1 * cross_x + b1 - y));
    if (dis < conf.line_thickness * 0.5f *
                  (1.0f + 9 * exp(-(conf.line_thickness - 1.0f) *
                                  (conf.line_thickness - 1.0f) / 4)) &&
        ((cross_x - start_point_x_) * (cross_x - end_point_x_)) <= 0)
    {
        return true;
    }
    return false;
}
}  // namespace USTC_CG