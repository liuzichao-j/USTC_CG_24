#pragma once
#include <Eigen/Dense>

#include "particle_system.h"
#include "sph_base.h"
#include "utils.h"

namespace USTC_CG::node_sph_fluid {

using namespace Eigen;

class IISPH : public SPHBase {
   public:
    IISPH() = default;
    IISPH(const MatrixXd& X, const Vector3d& box_min, const Vector3d& box_max);
    ~IISPH() = default;

    void step() override;
    void compute_pressure() override;

    double pressure_solve_iteration();
    void predict_advection();

    void reset() override;

    int& max_iter()
    {
        return max_iter_;
    }
    double& omega()
    {
        return omega_;
    }

   protected:
    int max_iter_ = 20;
    double omega_ = 0.1;

    // (HW TODO) Feel free to modify this part to remove or add necessary member variables
    VectorXd predict_density_;
    VectorXd aii_;
    VectorXd Api_;
    VectorXd last_pressure_;

    MatrixXd dii_;
};
}  // namespace USTC_CG::node_sph_fluid