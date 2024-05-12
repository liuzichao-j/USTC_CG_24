#include "wcsph.h"

#include <omp.h>

#include <iostream>
using namespace Eigen;

// per OpenMP standard, _OPENMP defined when compiling with OpenMP
#ifdef _OPENMP
#ifdef _MSC_VER
// must use MSVC __pragma here instead of _Pragma otherwise you get an internal
// compiler error. still an issue in Visual Studio 2022
#define OMP_PARALLEL_FOR __pragma(omp parallel for)
// any other standards-compliant C99/C++11 compiler
#else
#define OMP_PARALLEL_FOR _Pragma("omp parallel for")
#endif  // _MSC_VER
// no OpenMP support
#else
#define OMP_PARALLEL_FOR
#endif  // _OPENMP

namespace USTC_CG::node_sph_fluid {

WCSPH::WCSPH(const MatrixXd& X, const Vector3d& box_min, const Vector3d& box_max)
    : SPHBase(X, box_min, box_max)
{
}

void WCSPH::compute_density()
{
    // -------------------------------------------------------------
    // (HW TODO) Implement the density computation
    // You can also compute pressure in this function
    // -------------------------------------------------------------
    auto& particles = ps_.particles();
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        p->density_ = ps_.mass() * W_zero(ps_.h());
        for (auto& q : p->neighbors()) {
            p->density_ += ps_.mass() * W(p->x() - q->x(), ps_.h());
        }
        p->pressure_ =
            std::max(0.0, stiffness_ * (pow(p->density_ / ps_.density0(), exponent_) - 1));
    }

    // for (auto& p : ps_.particles()) {
    //     p->density_ = ps_.mass() * W_zero(ps_.h());
    //     for (auto& q : p->neighbors()) {
    //         p->density_ += ps_.mass() * W(p->x() - q->x(), ps_.h());
    //     }
    //     p->pressure_ =
    //         std::max(0.0, stiffness_ * (pow(p->density_ / ps_.density0(), exponent_) - 1));
    // }
}

void WCSPH::step()
{
    TIC(step)
    // -------------------------------------------------------------
    // (HW TODO) Follow the instruction in documents and PPT,
    // implement the pipeline of fluid simulation
    // -------------------------------------------------------------

    // Search neighbors, compute density, advect, solve pressure acceleration, etc.

    // 1. assign particle to cells & search neighbors
    ps_.assign_particles_to_cells();
    ps_.search_neighbors();

    // 2. compute density (actually, you can direct compute pressure here)
    compute_density();

    // 3. compute non-pressure accelerations, e.g. viscosity force, gravity
    compute_non_pressure_acceleration();
    auto& particles = ps_.particles();
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        p->vel_ += p->acceleration_ * this->dt();
    }
    // for (auto p : ps_.particles()) {
    //     p->vel_ += p->acceleration_ * this->dt();
    // }

    // 4. compute pressure gradient acceleration
    compute_pressure_gradient_acceleration();

    // 5. update velocity and positions, call advect()
    advect();

    TOC(step)
}
}  // namespace USTC_CG::node_sph_fluid