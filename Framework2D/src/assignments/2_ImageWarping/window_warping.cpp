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

/**
 * @brief Draw the main window
 * Draw the toolbar and the open/save file dialog. Draw the image by calling the
 * draw function of the CompWarping class.
 */
void ImageWarping::draw()
{
    draw_toolbar();
    if (flag_open_file_dialog_)
    {
        draw_open_image_file_dialog();
    }
    if (flag_save_file_dialog_ && p_image_)
    {
        // Only save the image when it is not empty
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

/**
 * @brief Draw the toolbar
 * Draw the main menu bar and the submenus for file operations, image editing,
 * and warping.
 */
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
        if (p_image_)
        {
            if (ImGui::MenuItem("Invert"))
            {
                p_image_->invert();
            }
            if (ImGui::BeginMenu("Mirror"))
            {
                if (ImGui::MenuItem("Horizontal"))
                {
                    p_image_->mirror(true, false);
                }
                if (ImGui::MenuItem("Vertical"))
                {
                    p_image_->mirror(false, true);
                }
                if (ImGui::MenuItem("Both"))
                {
                    p_image_->mirror(true, true);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("GrayScale"))
            {
                if (ImGui::MenuItem("Average"))
                {
                    p_image_->gray_scale(0);
                }
                if (ImGui::MenuItem("Weighted"))
                {
                    p_image_->gray_scale(1);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Points"))
            {
                if (ImGui::MenuItem("Select") && p_image_)
                {
                    p_image_->enable_selecting(true);
                }
                if (ImGui::MenuItem("Hide") && p_image_)
                {
                    p_image_->enable_selecting(false);
                }
                if (ImGui::MenuItem("Reset") && p_image_)
                {
                    p_image_->init_selections();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Warping"))
            {
                if (ImGui::MenuItem("FishEye"))
                {
                    p_image_->set_warping_method(0);
                    p_image_->enable_selecting(false);
                    p_image_->warping();
                }
                ImGui::Separator();
                // HW2_TODO: You can add more interactions for IDW, RBF, etc.
                if (ImGui::MenuItem("IDW"))
                {
                    p_image_->set_warping_method(1);
                    p_image_->enable_selecting(false);
                    p_image_->warping();
                }
                if (ImGui::MenuItem("RBF"))
                {
                    p_image_->set_warping_method(2);
                    p_image_->enable_selecting(false);
                    p_image_->warping();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Fix gaps"))
            {
                ImGui::Checkbox("Inverse Warping", &p_image_->inverse_flag);
                if (p_image_->inverse_flag == false)
                {
                    if (!p_image_->fixgap_flag_ann)
                    {
                        ImGui::Checkbox(
                            "Neighbour", &p_image_->fixgap_flag_neighbour);
                    }
                    if (!p_image_->fixgap_flag_neighbour)
                    {
                        ImGui::Checkbox(
                            "ANN (very slow)", &p_image_->fixgap_flag_ann);
                    }
                }
                else
                {
                    p_image_->fixgap_flag_ann = false;
                    p_image_->fixgap_flag_neighbour = false;
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Restore") && p_image_)
            {
                p_image_->restore();
            }
        }
        ImGui::EndMainMenuBar();
    }
}

/**
 * @brief Draw the image on center of the window with the original size of the
 * image
 */
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

/**
 * @brief Draw the open file dialog
 * Use the ImGuiFileDialog library to open a file dialog for choosing an image
 * file. The file dialog is modal and will be displayed when the flag
 * flag_open_file_dialog_ is true.
 */
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

/**
 * @brief Draw the save file dialog
 * Use the ImGuiFileDialog library to open a file dialog for choosing a file to
 * save the image. The file dialog is modal and will be displayed when the flag
 * flag_save_file_dialog_ is true.
 */
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