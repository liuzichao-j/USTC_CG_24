#include "FastMassSpring.h"

#include <iostream>
#include <vector>

namespace USTC_CG::node_mass_spring {
FastMassSpring::FastMassSpring(
    const Eigen::MatrixXd& X,
    const EdgeSet& E,
    const float stiffness,
    const float h)
    : MassSpring(X, E)
{
    // construct L and J at initialization
    std::cout << "init fast mass spring" << std::endl;

    unsigned n_vertices = X.rows();
    this->stiffness = stiffness;
    this->h = h;

    A.setZero();
    A.resize(3 * n_vertices, 3 * n_vertices);
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.clear();
    for (auto e : E) {
        for (int i = 0; i < 3; i++) {
            triplets.push_back(
                Eigen::Triplet<double>(e.first * 3 + i, e.first * 3 + i, h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(e.second * 3 + i, e.second * 3 + i, h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(e.first * 3 + i, e.second * 3 + i, -h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(e.second * 3 + i, e.first * 3 + i, -h * h * stiffness));
        }
    }
    for (int i = 0; i < 3 * n_vertices; i++) {
        triplets.push_back(Eigen::Triplet<double>(i, i, mass / n_vertices));
    }
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();

    // (HW Optional) precompute A and prefactorize
    // Note: one thing to take care of: A is related with stiffness, if stiffness changes, A need to
    // be recomputed

    solver.compute(A);
    if (solver.info() != Eigen::Success) {
        std::cerr << "Decomposition failed" << std::endl;
    }

    // print A for debugging
    printf("A: \n");
    for (int i = 0; i < 3 * n_vertices; i++) {
        for (int j = 0; j < 3 * n_vertices; j++) {
            printf("%6.3f ", A.coeff(i, j));
        }
        printf("\n");
    }
}

void FastMassSpring::step()
{
    // (HW Optional) Necessary preparation
    unsigned n_vertices = X.rows();
    unsigned n_edges = E.size();
    double mass_per_vertex = mass / n_vertices;

    Eigen::Vector3d acceleration_ext = gravity + wind_ext_acc;
    Eigen::MatrixXd acceleration_collision =
        getSphereCollisionForce(sphere_center.cast<double>(), sphere_radius);
    
    TIC(step)

    auto new_X = X;

    // Calculate y
    Eigen::MatrixXd y = Eigen::MatrixXd::Zero(n_vertices, 3);
    for (int i = 0; i < n_vertices; i++) {
        // if (!dirichlet_bc_mask[i]) {
            y.row(i) = X.row(i) + h * vel.row(i) + h * h * acceleration_ext.transpose();
            if (enable_sphere_collision)
            {
                y.row(i) += h * h * acceleration_collision.row(i);
            }
        // }
    }

    // // print y for debugging, in dense format
    // for (int i = 0; i < n_vertices; i++) {
    //     std::cout << "y[" << i << "]: " << y.row(i) << std::endl;
    // }

    for (unsigned iter = 0; iter < max_iter; iter++) {
        // (HW Optional)
        // local_step and global_step alternating solving

        // // print X on iter
        // std::cout << "                                           iter: " << iter << std::endl;
        // for (int i = 0; i < n_vertices; i++) {
        //     std::cout << "X[" << i << "]: " << X.row(i) << std::endl;
        // }

        // Local step
        // Eigen::MatrixXd d = Eigen::MatrixXd::Zero(n_edges, 3);
        Eigen::VectorXd d = Eigen::VectorXd::Zero(n_edges * 3);
        int cnt = 0;
        for (auto e : E) {
            // d.row(cnt) = E_rest_length[cnt] * (new_X.row(e.first) - new_X.row(e.second)).normalized();
            d(cnt * 3) = E_rest_length[cnt] * (new_X(e.first, 0) - new_X(e.second, 0)) / (new_X.row(e.first) - new_X.row(e.second)).norm();
            d(cnt * 3 + 1) = E_rest_length[cnt] * (new_X(e.first, 1) - new_X(e.second, 1)) / (new_X.row(e.first) - new_X.row(e.second)).norm();
            d(cnt * 3 + 2) = E_rest_length[cnt] * (new_X(e.first, 2) - new_X(e.second, 2)) / (new_X.row(e.first) - new_X.row(e.second)).norm();
            cnt++;
        }

        // // print d
        // for (int i = 0; i < n_edges * 3; i++) {
        //     std::cout << "d[" << i << "]: " << d(i) << std::endl;
        // }

        // Global step

        Eigen::MatrixXd J = Eigen::MatrixXd::Zero(n_vertices * 3, n_edges * 3);
        cnt = 0;
        for (auto e : E) {
            for (int i = 0; i < 3; i++) {
                J(e.first * 3 + i, cnt * 3 + i) = stiffness;
                J(e.second * 3 + i, cnt * 3 + i) = -stiffness;
            }
            cnt++;
        }

        // Eigen::MatrixXd b = Eigen::MatrixXd::Zero(n_vertices, 3);
        Eigen::VectorXd b = Eigen::VectorXd::Zero(n_vertices * 3);
        cnt = 0;
        for (auto e : E) {
            for (int i = 0; i < 3; i++) {
                // b.row(e.first) += h * h * stiffness * d.row(cnt);
                // b.row(e.second) -= h * h * stiffness * d.row(cnt);
            }
            cnt++;
        }
        b = h * h * J * d;
        b += mass_per_vertex * flatten(y);

        // // print b
        // for (int i = 0; i < n_vertices * 3; i++) {
        //     std::cout << "b[" << i << "]: " << b(i) << std::endl;
        // }

        Eigen::VectorXd x = Eigen::VectorXd::Zero(n_vertices * 3);
        x = solver.solve(b);
        if (solver.info() != Eigen::Success) {
            std::cerr << "Solving failed" << std::endl;
        }

        // // print x
        // for (int i = 0; i < n_vertices; i++) {
        //     std::cout << "x[" << i << "]: " << x.segment<3>(i * 3) << std::endl;
        // }

        for (int i = 0; i < n_vertices; i++) {
            // if (!dirichlet_bc_mask[i]) {
                new_X(i, 0) = x(i * 3);
                new_X(i, 1) = x(i * 3 + 1);
                new_X(i, 2) = x(i * 3 + 2);
            // }
        }

        // print answer
        std::cout << "Answer X: " << std::endl;
        for (int i = 0; i < n_vertices; i++) {
            std::cout << "X[" << i << "]: " << new_X.row(i) << std::endl;
        }
    }

    // Update velocity
    for (int i = 0; i < n_vertices; i++) {
        // if (!dirichlet_bc_mask[i]) {
            vel.row(i) = (new_X.row(i) - X.row(i)) / h;
        // }
    }
    X = new_X;
    
        // print answer
        std::cout << "Total X: " << std::endl;
        for (int i = 0; i < n_vertices; i++) {
            std::cout << "X[" << i << "]: " << new_X.row(i) << std::endl;
        }
        std::cout << "Total vel: " << std::endl;
        for (int i = 0; i < n_vertices; i++) {
            std::cout << "vel[" << i << "]: " << vel.row(i) << std::endl;
        }
        std::cout << std::endl;
        std::cout << std::endl;

    TOC(step)
}

}  // namespace USTC_CG::node_mass_spring
