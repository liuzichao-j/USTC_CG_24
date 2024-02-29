#pragma once

#include <glad/glad.h>  // Include GLAD before GLFW.
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <memory>
#include <string>
#include <vector>

#include "shape.h"

namespace USTC_CG
{
class Images : public Shape
{
   public:
    Images() = default;
    // Constructs an Image component with a given label and image file.
    explicit Images(const std::string& filename, ImVec2 canvas_size);
    ~Images();  // Destructor to manage resources.

    // Renders the image component.
    void draw(const Config& config) const override;

    // Overrides Shape's update function to adjust the end point during
    // interaction
    void update(float x, float y) override
    {
        canvas_size_x_ = x;
        canvas_size_y_ = y;
    }

    void addpoint(float x, float y) override
    {
    }

    // Loads the image file into OpenGL texture memory.
    void loadgltexture();

   protected:
    std::string filename_;                 // Path to the image file.
    unsigned char* image_data_ = nullptr;  // Raw pixel data of the image.
    GLuint tex_id_ = 0;                    // OpenGL texture identifier.

    float canvas_size_x_ = 0, canvas_size_y_ = 0;

    int image_width_ = 0, image_height_ = 0;  // Dimensions of the loaded image.
};
}  // namespace USTC_CG
