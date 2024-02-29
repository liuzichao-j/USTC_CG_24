#include "view/shapes/images.h"
#define STB_IMAGE_IMPLEMENTATION
#include "iostream"
#include "stb_image.h"

namespace USTC_CG
{
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

Images::~Images()
{
    if (image_data_)
        stbi_image_free(image_data_);
    glDeleteTextures(1, &tex_id_);
}

void Images::draw(const Config& config) const
{
    auto draw_list = ImGui::GetWindowDrawList();
    if (image_data_)
    {
        // Center the image in the window
        ImVec2 p_min = ImVec2(
            config.bias[0] + canvas_size_x_ / 2 - (float)image_width_ / 2,
            config.bias[1] + canvas_size_y_ / 2 - (float)image_height_ / 2);
        ImVec2 p_max = ImVec2(p_min.x + image_width_, p_min.y + image_height_);
        draw_list->AddImage((void*)(intptr_t)tex_id_, p_min, p_max);
    }
}

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
}  // namespace USTC_CG