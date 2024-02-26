#pragma once

#include <memory>

#include "view/window.h"
#include "view/cmpt_canvas.h"

namespace USTC_CG
{
class MiniDraw : public Window
{
   public:
    explicit MiniDraw(const std::string& window_name);
    ~MiniDraw();

    void draw();

   private:
    void draw_canvas();

    std::shared_ptr<Canvas> p_canvas = nullptr;

    bool flag_show_canvas_view = true;
};
}  // namespace USTC_CG