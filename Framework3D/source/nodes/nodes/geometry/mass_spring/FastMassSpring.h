#pragma once
#include <memory>

#include "MassSpring.h"

namespace USTC_CG::node_mass_spring {
// Impliment the Liu13's paper: https://tiantianliu.cn/papers/liu13fast/liu13fast.pdf
class FastMassSpring : public MassSpring {
   public:
    FastMassSpring() = default;
    ~FastMassSpring() = default;

    FastMassSpring(
        const Eigen::MatrixXd& X,
        const EdgeSet& E,
        const float stiffness,
        const float h,
        const unsigned iter);
    FastMassSpring(
        const Eigen::MatrixXd& X,
        const EdgeSet& E,
        const float stiffness,
        const float h);
    void step() override;
    unsigned max_iter = 100;  // (HW Optional) add UI for this parameter

    bool set_dirichlet_bc_mask(const std::vector<bool>& mask) override;
    bool update_dirichlet_bc_vertices(const MatrixXd &control_vertices) override; 
    bool init_dirichlet_bc_vertices_control_pair(const MatrixXd &control_vertices,
                                      const std::vector<bool>& control_mask) override;
                                      
    void init();

   protected:
    // Custom variables, like prefactorized A
    Eigen::SparseMatrix<double> A;
    // Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    Eigen::SimplicialLLT<Eigen::SparseMatrix<double>> solver;

    // Renumbering vectors
    std::vector<int> point_to_id;
    int n_vertex;

    // Fixed item on the right of the equation
    Eigen::MatrixXd fixed;
};
}  // namespace USTC_CG::node_mass_spring