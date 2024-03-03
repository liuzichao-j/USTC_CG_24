#include "view/shapes/images.h"

#include <math.h>

#include <iostream>

#include "stb_image.h"

namespace USTC_CG
{
/**
 * @brief
 * 创建一个Images对象，指定图像文件名。传入画布的大小以便绘制在画布的中心。
 *
 * @param filename 图像文件的路径。
 * @param canvas_size 画布的大小。
 */
Images::Images(const std::string& filename, ImVec2 canvas_size)
    : filename_(filename),
      canvas_size_x_(canvas_size.x),
      canvas_size_y_(canvas_size.y)
{
    glGenTextures(1, &tex_id_);
    image_data_ =
        stbi_load(filename.c_str(), &image_width_, &image_height_, NULL, 4);
    if (image_data_ == nullptr)
        std::cout << "Failed to load image from file " << filename << std::endl;
    else
        loadgltexture();
}

/**
 * @brief 析构函数
 *
 * 释放图像数据的内存并删除OpenGL纹理。
 */
Images::~Images()
{
    if (image_data_)
        stbi_image_free(image_data_);
    glDeleteTextures(1, &tex_id_);
}

/**
 * @brief 在画布上绘制图像。
 *
 * 它将图像居中显示在窗口中，并根据提供的大小进行缩放。
 *
 * @param config 绘制图像的配置，包括偏移量、线条颜色、线条粗细、缩放比例等。
 */
void Images::draw(const Config& config) const
{
    auto draw_list = ImGui::GetWindowDrawList();
    if (image_data_)
    {
        // Center the image in the window
        ImVec2 p_min = ImVec2(
            config.bias[0] + conf.image_bia[0] * canvas_size_x_ -
                conf.image_size * image_width_ / 2,
            config.bias[1] + conf.image_bia[1] * canvas_size_y_ -
                conf.image_size * image_height_ / 2);
        ImVec2 p_max = ImVec2(
            p_min.x + conf.image_size * image_width_,
            p_min.y + conf.image_size * image_height_);
        draw_list->AddImage((void*)(intptr_t)tex_id_, p_min, p_max);
        if (conf.time)
        {
            draw_list->AddRect(
                p_min,
                p_max,
                IM_COL32(
                    255,
                    255,
                    255,
                    (unsigned char)((0.5f +
                                     0.5f * cos(conf.time) * cos(conf.time)) *
                                    255)),  // 实现A通道正弦函数变化，范围为0.5倍-1倍
                0.0f,
                0,
                3.0f);
        }
    }
}

/**
 * @brief 使用update函数来更新画布尺寸。
 *
 * @param x 画布的宽度。
 * @param y 画布的高度。
 */
void Images::update(float x, float y)
{
    canvas_size_x_ = x;
    canvas_size_y_ = y;
}

/**
 * 绑定OpenGL纹理，设置显示的过滤参数，并将图像数据上传到纹理中。
 */
void Images::loadgltexture()
{
    glBindTexture(GL_TEXTURE_2D, tex_id_);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        image_width_,
        image_height_,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image_data_);
}

/**
 * @brief 判断点是否在图像上。
 *
 * @param x X坐标
 * @param y Y坐标
 * @return true 点在图像上
 * @return false 点不在图像上
 */
bool Images::is_select_on(float x, float y) const
{
    // 确定图像的位置范围
    ImVec2 p_min = ImVec2(
        conf.image_bia[0] * canvas_size_x_ - conf.image_size * image_width_ / 2,
        conf.image_bia[1] * canvas_size_y_ -
            conf.image_size * image_height_ / 2);
    ImVec2 p_max = ImVec2(
        p_min.x + conf.image_size * image_width_,
        p_min.y + conf.image_size * image_height_);
    if (x >= p_min.x && x <= p_max.x && y >= p_min.y && y <= p_max.y)
    {
        return true;
    }
    else
    {
        return false;
    }
}
}  // namespace USTC_CG