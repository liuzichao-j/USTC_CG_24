#include "iisph.h"

#include <iostream>

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

using namespace Eigen;

IISPH::IISPH(const MatrixXd& X, const Vector3d& box_min, const Vector3d& box_max)
    : SPHBase(X, box_min, box_max)
{
    // (HW TODO) Feel free to modify this part to remove or add necessary member variables
    predict_density_ = VectorXd::Zero(ps_.particles().size());
    aii_ = VectorXd::Zero(ps_.particles().size());
    Api_ = VectorXd::Zero(ps_.particles().size());
    last_pressure_ = VectorXd::Zero(ps_.particles().size());

    dii_ = MatrixXd::Zero(3, ps_.particles().size());
}

void IISPH::step()
{
    // (HW Optional)
    TIC(step)

    // 1. assign particle to cells & search neighbors
    ps_.assign_particles_to_cells();
    ps_.search_neighbors();

    // 2. compute density (actually, you can direct compute pressure here)
    compute_density();

    // 3. compute non-pressure accelerations, e.g. viscosity force, gravity
    compute_non_pressure_acceleration();

    // 4. predict advection
    predict_advection();

    // 5. solve pressure poisson equation
    compute_pressure();

    // 6. update acceleration
    compute_pressure_gradient_acceleration();

    // 7. update velocity and position
    advect();

    TOC(step)
}

void IISPH::compute_pressure()
{
    // (HW Optional) solve pressure using relaxed Jacobi iteration
    // Something like this:

    double threshold = 0.001;
    for (unsigned iter = 0; iter < max_iter_; iter++) {
        double avg_density_error = pressure_solve_iteration();
        if (avg_density_error < threshold) {
            break;
        }
    }
    auto& particles = ps_.particles();
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        p->pressure_ = last_pressure_[i];
        p->density_ = ps_.density0();
    }
}

void IISPH::predict_advection()
{
    // (HW Optional)
    // predict new density based on non-pressure forces,
    // compute necessary variables for pressure solve, etc.

    // Note: feel free to remove or add functions based on your need,
    // you can also rename this function.

    auto& particles = ps_.particles();
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        p->vel_ += dt() * p->acceleration_;
    }
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        predict_density_[i] = p->density();
        for (auto& q : p->neighbors()) {
            predict_density_[i] -= p->density() * dt() * ps_.mass() / q->density() *
                                   (p->vel() - q->vel()).dot(grad_W(p->x() - q->x(), ps_.h()));
        }
    }
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        VectorXd sum = VectorXd::Zero(3);
        for (auto& q : p->neighbors()) {
            VectorXd grad = grad_W(p->x() - q->x(), ps_.h());
            sum += ps_.mass() / (p->density() * p->density()) * grad;
        }
        dii_.col(i) = sum;
    }
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        double sum = 0.0;
        for (auto& q : p->neighbors()) {
            VectorXd grad = grad_W(p->x() - q->x(), ps_.h());
            sum -= ps_.mass() *
                   (ps_.mass() / (p->density() * p->density()) * grad + dii_.col(i)).dot(grad);
        }
        aii_[i] = sum;
    }

    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        last_pressure_[i] = p->pressure();
    }
}

double IISPH::pressure_solve_iteration()
{
    // (HW Optional)
    // One step iteration to solve the pressure poisson equation of IISPH
    double density_error = 0.0;
    auto& particles = ps_.particles();
    MatrixXd api = MatrixXd::Zero(3, particles.size());
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        VectorXd sum = VectorXd::Zero(3);
        for (auto& q : p->neighbors()) {
            VectorXd grad = grad_W(p->x() - q->x(), ps_.h());
            sum -= ps_.mass() *
                   (last_pressure_[i] / (p->density() * p->density()) +
                    last_pressure_[q->idx()] / (q->density() * q->density())) *
                   grad;
        }
        api.col(i) = sum;
    }
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        double sum = 0.0;
        for (auto& q : p->neighbors()) {
            VectorXd grad = grad_W(p->x() - q->x(), ps_.h());
            sum += ps_.mass() * (api.col(i) - api.col(q->idx())).dot(grad);
        }
        Api_[i] = sum;
    }
    OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        if (aii_[i] != 0) {
            last_pressure_[i] += omega_ / aii_[i] *
                                 ((ps_.density0() - predict_density_[i]) / (dt() * dt()) - Api_[i]);
        }
        else {
            last_pressure_[i] +=
                omega_ / 1e-6 * ((ps_.density0() - predict_density_[i]) / (dt() * dt()) - Api_[i]);
        }
        if (last_pressure_[i] < 0.0f) {
            last_pressure_[i] = 0.0f;
        }
        if (last_pressure_[i] > 1e5) {
            last_pressure_[i] = 1e5;
        }
    }
    // OMP_PARALLEL_FOR
    // for (int i = 0; i < particles.size(); i++) {
    //     auto& p = particles[i];
    //     predict_density_[i] = ps_.density0() - Api_[i] * dt() * dt();
    // }
    // OMP_PARALLEL_FOR
    for (int i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        density_error += std::abs(Api_[i] * dt() * dt() + predict_density_[i] - ps_.density0());
    }
    return density_error / particles.size();
}

// ------------------ helper function, no need to modify ---------------------
void IISPH::reset()
{
    SPHBase::reset();

    predict_density_ = VectorXd::Zero(ps_.particles().size());
    aii_ = VectorXd::Zero(ps_.particles().size());
    Api_ = VectorXd::Zero(ps_.particles().size());
    last_pressure_ = VectorXd::Zero(ps_.particles().size());
}
}  // namespace USTC_CG::node_sph_fluid