#include "MassSpring.h"

#include <cmath>
#include <iostream>

namespace USTC_CG::node_mass_spring {
MassSpring::MassSpring(const Eigen::MatrixXd& X, const EdgeSet& E)
{
    this->X = this->init_X = X;
    this->vel = Eigen::MatrixXd::Zero(X.rows(), X.cols());
    this->E = E;

    std::cout << "number of edges: " << E.size() << std::endl;
    std::cout << "init mass spring" << std::endl;

    // Compute the rest pose edge length
    for (const auto& e : E) {
        Eigen::Vector3d x0 = X.row(e.first);
        Eigen::Vector3d x1 = X.row(e.second);
        this->E_rest_length.push_back((x0 - x1).norm());
    }

    // Initialize the mask for Dirichlet boundary condition
    dirichlet_bc_mask.resize(X.rows(), false);

    // (HW_TODO) Fix two vertices, feel free to modify this
    unsigned n_fix = sqrt(X.rows());  // Here we assume the cloth is square
    dirichlet_bc_mask[0] = true;
    dirichlet_bc_mask[n_fix - 1] = true;
}

void MassSpring::step()
{
    Eigen::Vector3d acceleration_ext = gravity + wind_ext_acc;

    unsigned n_vertices = X.rows();

    // The reason to not use 1.0 as mass per vertex: the cloth gets heavier as we increase the
    // resolution
    double mass_per_vertex = mass / n_vertices;

    //----------------------------------------------------
    // (HW Optional) Bonus part: Sphere collision
    Eigen::MatrixXd acceleration_collision =
        getSphereCollisionForce(sphere_center.cast<double>(), sphere_radius);
    //----------------------------------------------------

    if (time_integrator == IMPLICIT_EULER) {
        // Implicit Euler
        TIC(step)

        // (HW TODO)
        auto H_elastic = computeHessianSparse(stiffness);  // size = [nx3, nx3]
        Eigen::SparseMatrix<double> H(n_vertices * 3, n_vertices * 3);
        H.setIdentity();
        H = H * mass_per_vertex / h / h + H_elastic;
        // if (!checkSPD(H)) {
        //     std::cerr << "Hessian is not SPD!" << std::endl;
        //     return;
        // }
        toSPD(H);

        Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
        solver.compute(H);
        if (solver.info() != Eigen::Success) {
            std::cerr << "Decomposition failed!" << std::endl;
            return;
        }

        // compute Y
        Eigen::MatrixXd Y = X + h * vel;
        for (int i = 0; i < n_vertices; i++) {
            if (!dirichlet_bc_mask[i]) {
                Y.row(i) += h * h * acceleration_ext.transpose();
                if (enable_sphere_collision) {
                    Y.row(i) += h * h * acceleration_collision.row(i);
                }
            }
        }

        auto grad_g = computeGrad(stiffness);
        for (int i = 0; i < n_vertices; i++) {
            if (!dirichlet_bc_mask[i]) {
                grad_g.row(i) += mass_per_vertex * (X.row(i) - Y.row(i)) / h / h;
            }
        }
        auto grad_g_flatten = flatten(grad_g);

        // Solve Newton's search direction with linear solver
        // Delta x = H^(-1) * grad, H * Delta x = grad
        auto delta_X_flatten = solver.solve(grad_g_flatten);
        if (solver.info() != Eigen::Success) {
            std::cerr << "Solving failed!" << std::endl;
            return;
        }
        auto delta_X = unflatten(delta_X_flatten);

        // update X and vel
        for (int i = 0; i < n_vertices; i++) {
            if (!dirichlet_bc_mask[i]) {
                X.row(i) -= delta_X.row(i);
                vel.row(i) = -delta_X.row(i) / h;
            }
        }

        TOC(step)
    }
    else if (time_integrator == SEMI_IMPLICIT_EULER) {
        // Semi-implicit Euler
        Eigen::MatrixXd acceleration = -computeGrad(stiffness) / mass_per_vertex;
        // acceleration.rowwise() += acceleration_ext.transpose();
        for (int i = 0; i < n_vertices; i++) {
            if (!dirichlet_bc_mask[i]) {
                acceleration.row(i) += acceleration_ext.transpose();
            }
        }
        // acceleration (n x 3) stores the acceleration for each vertex

        // -----------------------------------------------
        // (HW Optional)
        if (enable_sphere_collision) {
            acceleration += acceleration_collision;
        }
        // -----------------------------------------------

        // (HW TODO): Implement semi-implicit Euler time integration
        // Update X and vel
        for (int i = 0; i < n_vertices; i++) {
            vel.row(i) += h * acceleration.row(i);
            X.row(i) += h * vel.row(i);
        }
        vel *= std::pow(damping, h);
    }
    else {
        std::cerr << "Unknown time integrator!" << std::endl;
        return;
    }
}

// There are different types of mass spring energy:
// For this homework we will adopt Prof. Huamin Wang's energy definition introduced in GAMES103
// course Lecture 2 E = 0.5 * stiffness * sum_{i=1}^{n} (||x_i - x_j|| - l)^2 There exist other
// types of energy definition, e.g., Prof. Minchen Li's energy definition
// https://www.cs.cmu.edu/~15769-f23/lec/3_Mass_Spring_Systems.pdf
double MassSpring::computeEnergy(double stiffness)
{
    double sum = 0.;
    unsigned i = 0;
    for (const auto& e : E) {
        // For each edge
        auto diff = X.row(e.first) - X.row(e.second);
        // X (n x 3) stores the vertex positions
        // E (m x 2) stores the edge indices
        auto l = E_rest_length[i];
        // E_rest_length stores the rest length of each edge, num. i
        sum += 0.5 * stiffness * std::pow((diff.norm() - l), 2);
        i++;
    }
    return sum;
}

Eigen::MatrixXd MassSpring::computeGrad(double stiffness)
{
    Eigen::MatrixXd g = Eigen::MatrixXd::Zero(X.rows(), X.cols());
    // g (n x 3) stores the gradient of the energy. The gradient is about the vertex i.
    unsigned i = 0;
    for (const auto& e : E) {
        // --------------------------------------------------
        // (HW TODO): Implement the gradient computation
        const Eigen::Vector3d x = X.row(e.first) - X.row(e.second);
        g.row(e.first) += stiffness * (x.norm() - E_rest_length[i]) * x.normalized();
        g.row(e.second) += -stiffness * (x.norm() - E_rest_length[i]) * x.normalized();
        // --------------------------------------------------
        i++;
    }
    for (int j = 0; j < X.rows(); j++) {
        if (dirichlet_bc_mask[j]) {
            g.row(j).setZero();
        }
    }
    return g;
}

Eigen::SparseMatrix<double> MassSpring::computeHessianSparse(double stiffness)
{
    unsigned n_vertices = X.rows();
    Eigen::SparseMatrix<double> H(n_vertices * 3, n_vertices * 3);

    unsigned i = 0;
    auto k = stiffness;
    const auto I = Eigen::MatrixXd::Identity(3, 3);

    std::vector<Eigen::Triplet<double>> triplets;
    for (const auto& e : E) {
        // --------------------------------------------------
        // (HW TODO): Implement the sparse version Hessian computation
        // Remember to consider fixed points
        // You can also consider positive definiteness here
        const Eigen::Vector3d x = X.row(e.first) - X.row(e.second);
        const Eigen::MatrixXd He =
            k * (x * x.transpose() / x.squaredNorm() +
                 (1 - E_rest_length[i] / x.norm()) * (I - x * x.transpose() / x.squaredNorm()));
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (!dirichlet_bc_mask[e.first]) {
                    if (!dirichlet_bc_mask[e.second]) {
                        triplets.push_back(
                            Eigen::Triplet<double>(e.first * 3 + i, e.first * 3 + j, He(i, j)));
                        triplets.push_back(
                            Eigen::Triplet<double>(e.first * 3 + i, e.second * 3 + j, -He(i, j)));
                        triplets.push_back(
                            Eigen::Triplet<double>(e.second * 3 + i, e.first * 3 + j, -He(i, j)));
                        triplets.push_back(
                            Eigen::Triplet<double>(e.second * 3 + i, e.second * 3 + j, He(i, j)));
                    }
                    else {
                        triplets.push_back(
                            Eigen::Triplet<double>(e.first * 3 + i, e.first * 3 + j, He(i, j)));
                    }
                }
                else {
                    if (!dirichlet_bc_mask[e.second]) {
                        triplets.push_back(
                            Eigen::Triplet<double>(e.second * 3 + i, e.second * 3 + j, He(i, j)));
                    }
                }
            }
        }
        // --------------------------------------------------
        i++;
    }
    H.setFromTriplets(triplets.begin(), triplets.end());
    H.makeCompressed();
    return H;
}

bool MassSpring::checkSPD(const Eigen::SparseMatrix<double>& A)
{
    // Eigen::SimplicialLDLT<SparseMatrix_d> ldlt(A);
    // return ldlt.info() == Eigen::Success;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(A);
    auto eigen_values = es.eigenvalues();
    return eigen_values.minCoeff() >= 1e-10;
}

#include "Spectra/SymEigsSolver.h"
#include "Spectra/MatOp/SparseSymMatProd.h"

void MassSpring::toSPD(Eigen::SparseMatrix<double> &A)
{
    // Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(A);
    // auto eigen_values = es.eigenvalues();
    // if (eigen_values.minCoeff() < 0) {
    //     printf("Eigenvalue < 0, make SPD\n");
    //     Eigen::SparseMatrix<double> B(A.rows(), A.cols());
    //     B.setIdentity();
    //     A += B * (1 - eigen_values.minCoeff());
    // }

    // Spectra::SparseSymShiftSolve<double> op(A);
    // Spectra::SymEigsShiftSolver<double, Eigen::Spectra::LARGEST_MAGN, Eigen::Spectra::SparseSymShiftSolve<double>> eigs(&op, 1, 1e-6, 10);
    Spectra::SparseSymMatProd<double> op(A);
    Spectra::SymEigsSolver<Spectra::SparseSymMatProd<double>> eigs(op, 1, 10);
    eigs.init();
    eigs.compute(Spectra::SortRule::LargestMagn);
    auto minimal = eigs.eigenvalues()[0];
    if (minimal < 0) {
        printf("Eigenvalue < 0, make SPD\n");
        Eigen::SparseMatrix<double> B(A.rows(), A.cols());
        B.setIdentity();
        A += B * (1e-6 - minimal);
    }
    return;
}

void MassSpring::reset()
{
    std::cout << "reset" << std::endl;
    this->X = this->init_X;
    this->vel.setZero();
}

// ----------------------------------------------------------------------------------
// (HW Optional) Bonus part
Eigen::MatrixXd MassSpring::getSphereCollisionForce(Eigen::Vector3d center, double radius)
{
    Eigen::MatrixXd force = Eigen::MatrixXd::Zero(X.rows(), X.cols());
    for (int i = 0; i < X.rows(); i++) {
        // (HW Optional) Implement penalty-based force here
        auto delta_x = X.row(i) - center.transpose();
        force.row(i) += collision_penalty_k *
                        std::max(0.0, collision_scale_factor * radius - delta_x.norm()) *
                        delta_x.normalized();
    }
    return force;
}
// ----------------------------------------------------------------------------------

}  // namespace USTC_CG::node_mass_spring
