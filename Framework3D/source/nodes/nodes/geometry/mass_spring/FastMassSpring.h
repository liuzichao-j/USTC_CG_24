#pragma once 
#include "MassSpring.h"
#include <memory>

namespace USTC_CG::node_mass_spring {
// Impliment the Liu13's paper: https://tiantianliu.cn/papers/liu13fast/liu13fast.pdf
class FastMassSpring : public MassSpring {
   public:
    FastMassSpring() = default;
    ~FastMassSpring() = default; 

    FastMassSpring(const Eigen::MatrixXd& X, const EdgeSet& E, const float stiffness, const float h, const unsigned iter);
    void step() override;
    unsigned max_iter = 100; // (HW Optional) add UI for this parameter

   protected:
    // Custom variables, like prefactorized A 
    Eigen::SparseMatrix<double> A;
    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;

    // Renumbering vectors
    std::vector<int> point_to_id;
    std::vector<int> id_to_point;

    // Another A used for fixed points
    Eigen::SparseMatrix<double> A1;
};
}  // namespace USTC_CG::node_mass_spring