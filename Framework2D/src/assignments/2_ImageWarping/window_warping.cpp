#include "window_warping.h"

#include <ImGuiFileDialog.h>

#include <iostream>

namespace USTC_CG
{

ImageWarping::ImageWarping(const std::string& window_name) : Window(window_name)
{
}

ImageWarping::~ImageWarping()
{
}

void ImageWarping::draw()
{
    draw_toolbar();
    if (flag_open_file_dialog_)
    {
        draw_open_image_file_dialog();
    }
    if (flag_save_file_dialog_ &&
        p_image_)  // Only save the image when it is not empty
    {
        draw_save_image_file_dialog();
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    if (ImGui::Begin(
            "ImageEditor",
            &flag_show_main_view_,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoBringToFrontOnFocus))
    // Important Flag of NoBringToFrontOnFocus to avoid the popup window to be
    // behind the main window
    {
        if (p_image_)
        {
            draw_image();
        }
        ImGui::End();
    }
}

void ImageWarping::draw_toolbar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Image File.."))
            {
                flag_open_file_dialog_ = true;
            }
            if (ImGui::MenuItem("Save As.."))
            {
                flag_save_file_dialog_ = true;
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Invert") && p_image_)
        {
            p_image_->invert();
        }
        if (ImGui::MenuItem("Mirror") && p_image_)
        {
            p_image_->mirror(true, false);
        }
        if (ImGui::MenuItem("GrayScale") && p_image_)
        {
            p_image_->gray_scale();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Select Points") && p_image_)
        {
            // p_image_->init_selections();
            p_image_->enable_selecting(true);
        }
        if (ImGui::MenuItem("Reset Points") && p_image_)
        {
            p_image_->init_selections();
        }
        if (ImGui::BeginMenu("Warping"))
        {
            // HW2_TODO: You can add more interactions for IDW, RBF, etc.
            if (ImGui::MenuItem("Warping-FishEye") && p_image_)
            {
                p_image_->set_warping_method(0);
                p_image_->enable_selecting(false);
                p_image_->warping();
                p_image_->init_selections();
            }
            if (ImGui::MenuItem("Warping-IDW") && p_image_)
            {
                p_image_->set_warping_method(1);
                p_image_->enable_selecting(false);
                p_image_->warping();
                // p_image_->init_selections();
            }
            if (ImGui::MenuItem("Warping-RBF") && p_image_)
            {
                p_image_->set_warping_method(2);
                p_image_->enable_selecting(false);
                p_image_->warping();
                p_image_->init_selections();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Restore") && p_image_)
        {
            p_image_->restore();
        }
        ImGui::Separator();
        if (p_image_)
        {
            if (p_image_->inverse_flag)
            {
                if (ImGui::MenuItem("Normal Warping"))
                {
                    p_image_->inverse_flag = false;
                }
            }
            else
            {
                if (ImGui::MenuItem("Inverse Warping"))
                {
                    p_image_->inverse_flag = true;
                }
            }
        }
        ImGui::EndMainMenuBar();
    }
}

void ImageWarping::draw_image()
{
    const auto& canvas_min = ImGui::GetCursorScreenPos();
    const auto& canvas_size = ImGui::GetContentRegionAvail();
    const auto& image_size = p_image_->get_image_size();
    // Center the image in the window
    ImVec2 pos = ImVec2(
        canvas_min.x + canvas_size.x / 2 - image_size.x / 2,
        canvas_min.y + canvas_size.y / 2 - image_size.y / 2);
    p_image_->set_position(pos);
    p_image_->draw();
}

void ImageWarping::draw_open_image_file_dialog()
{
    IGFD::FileDialogConfig config;
    config.path = DATA_PATH;
    config.flags = ImGuiFileDialogFlags_Modal;
    // Use Flag ImGuiFileDialogFlags_Modal to show popup window
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseImageOpenFileDlg", "Choose Image File", ".png,.jpg", config);
    ImVec2 main_size = ImGui::GetMainViewport()->WorkSize;  // Without bars
    ImVec2 dlg_size(main_size.x / 2, main_size.y / 2);
    if (ImGuiFileDialog::Instance()->Display(
            "ChooseImageOpenFileDlg", ImGuiWindowFlags_NoCollapse, dlg_size))
    // Avoid collapse, Set size
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePathName =
                ImGuiFileDialog::Instance()->GetFilePathName();
            std::string label = filePathName;
            p_image_ = std::make_shared<CompWarping>(label, filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
        flag_open_file_dialog_ = false;
    }
}

void ImageWarping::draw_save_image_file_dialog()
{
    IGFD::FileDialogConfig config;
    config.path = DATA_PATH;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseImageSaveFileDlg", "Save Image As...", ".png", config);
    ImVec2 main_size = ImGui::GetMainViewport()->WorkSize;
    ImVec2 dlg_size(main_size.x / 2, main_size.y / 2);
    if (ImGuiFileDialog::Instance()->Display(
            "ChooseImageSaveFileDlg", ImGuiWindowFlags_NoCollapse, dlg_size))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePathName =
                ImGuiFileDialog::Instance()->GetFilePathName();
            std::string label = filePathName;
            if (p_image_)
                p_image_->save_to_disk(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
        flag_save_file_dialog_ = false;
    }
}
}  // namespace USTC_CG