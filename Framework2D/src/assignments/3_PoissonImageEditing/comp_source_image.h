#pragma once

#include <Eigen/Sparse>

#include "view/comp_image.h"

namespace USTC_CG
{
class CompSourceImage : public ImageEditor
{
   public:
    // HW3_TODO(optional): Add more region shapes like polygon and freehand.
    enum RegionType
    {
        kDefault = 0,
        kRect = 1,
        kPolygon = 2,
        kFreehand = 3
    };

    explicit CompSourceImage(
        const std::string& label,
        const std::string& filename);
    virtual ~CompSourceImage() noexcept = default;

    void draw() override;

    // Point selecting interaction
    void enable_selecting(bool flag);
    void set_region_type(RegionType type);
    void select_region();
    // Get the selected region in the source image, this would be a binary mask
    std::shared_ptr<Image> get_region();
    // Get the image data
    std::shared_ptr<Image> get_data();
    // Get the position to locate the region in the target image
    ImVec2 get_position() const;

    // Scanning line algorithm to fill the selected region
    void init_selections();

    // Initialize the selected region by giving every point an id
    void init_id();
    // Get the id of a point in the selected region
    int get_id(ImVec2 point);
    // Get the point of an id in the selected region
    ImVec2 get_point(int id);
    // Get the number of points in the selected region
    int get_point_num();

    // Initialize matrix A in advance
    void init_matrix();
    // The solver of A
    void solver(Eigen::VectorXf& b, Eigen::VectorXf& x);
    bool is_solver_ready();

   private:
    RegionType region_type_ = kDefault;
    std::shared_ptr<Image> selected_region_;
    // Now position when drawing the selected region
    ImVec2 now_;
    // The left-top position of the selected region
    ImVec2 start_, end_;
    // Store points on the edge
    std::vector<ImVec2> edge_points_;
    bool flag_enable_selecting_region_ = false;
    bool draw_status_ = false;

    // Calculate the id of a point in the selected region
    std::vector<std::vector<int>> point_to_id_;
    std::vector<ImVec2> id_to_point_;

    // Store matrix A and solver
    Eigen::SparseMatrix<float> A_;
    Eigen::SimplicialLLT<Eigen::SparseMatrix<float>> solver_;

    bool flag_solver_ready_ = false;
};
}  // namespace USTC_CG