#include "FastMassSpring.h"

#include <iostream>
#include <vector>

namespace USTC_CG::node_mass_spring {
FastMassSpring::FastMassSpring(
    const Eigen::MatrixXd& X,
    const EdgeSet& E,
    const float stiffness,
    const float h, const unsigned iter = 100)
    : MassSpring(X, E)
{
    // construct L and J at initialization
    std::cout << "init fast mass spring" << std::endl;

    this->stiffness = stiffness;
    this->h = h;
    this->max_iter = iter;

    std::cout << "max iteration times: " << max_iter << std::endl;

    // Renumbering nodes
    point_to_id.assign(X.rows(), -1);
    id_to_point.clear();
    int n_vertex = 0;
    for (int i = 0; i < X.rows(); i++) {
        if (!dirichlet_bc_mask[i]) {
            point_to_id[i] = n_vertex;
            id_to_point.push_back(i);
            n_vertex++;
        }
    }
    // printf("point and id: \n");
    // for (int i = 0; i < X.rows(); i++) {
    //     printf("point %d -> id %d\n", i, point_to_id[i]);
    // }
    // for (int i = 0; i < n_vertex; i++) {
    //     printf("id %d -> point %d\n", i, id_to_point[i]);
    // }

    A.setZero();
    A.resize(3 * n_vertex, 3 * n_vertex);
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.clear();
    for (auto e : E) {
        int n1 = point_to_id[e.first];
        int n2 = point_to_id[e.second];
        if (n1 != -1) {
            triplets.push_back(Eigen::Triplet<double>(n1 * 3, n1 * 3, h * h * stiffness));
            triplets.push_back(Eigen::Triplet<double>(n1 * 3 + 1, n1 * 3 + 1, h * h * stiffness));
            triplets.push_back(Eigen::Triplet<double>(n1 * 3 + 2, n1 * 3 + 2, h * h * stiffness));
            if (n2 != -1) {
                triplets.push_back(Eigen::Triplet<double>(n1 * 3, n2 * 3, -h * h * stiffness));
                triplets.push_back(
                    Eigen::Triplet<double>(n1 * 3 + 1, n2 * 3 + 1, -h * h * stiffness));
                triplets.push_back(
                    Eigen::Triplet<double>(n1 * 3 + 2, n2 * 3 + 2, -h * h * stiffness));
                triplets.push_back(Eigen::Triplet<double>(n2 * 3, n1 * 3, -h * h * stiffness));
                triplets.push_back(
                    Eigen::Triplet<double>(n2 * 3 + 1, n1 * 3 + 1, -h * h * stiffness));
                triplets.push_back(
                    Eigen::Triplet<double>(n2 * 3 + 2, n1 * 3 + 2, -h * h * stiffness));
            }
        }
        if (n2 != -1) {
            triplets.push_back(Eigen::Triplet<double>(n2 * 3, n2 * 3, h * h * stiffness));
            triplets.push_back(Eigen::Triplet<double>(n2 * 3 + 1, n2 * 3 + 1, h * h * stiffness));
            triplets.push_back(Eigen::Triplet<double>(n2 * 3 + 2, n2 * 3 + 2, h * h * stiffness));
        }
    }
    for (int i = 0; i < 3 * n_vertex; i++) {
        triplets.push_back(Eigen::Triplet<double>(i, i, mass / X.rows()));
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

    // // print A for debugging
    // printf("A: \n");
    // for (int i = 0; i < 3 * n_vertex; i++) {
    //     for (int j = 0; j < 3 * n_vertex; j++) {
    //         printf("%6.3f ", A.coeff(i, j));
    //     }
    //     printf("\n");
    // }

    A1.setZero();
    A1.resize(3 * n_vertex, 3 * X.rows());
    triplets.clear();
    for (auto e : E) {
        int n1 = point_to_id[e.first];
        int n2 = point_to_id[e.second];
        if (n1 != -1) {
            triplets.push_back(Eigen::Triplet<double>(n1 * 3, e.first * 3, h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(n1 * 3 + 1, e.first * 3 + 1, h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(n1 * 3 + 2, e.first * 3 + 2, h * h * stiffness));
            triplets.push_back(Eigen::Triplet<double>(n1 * 3, e.second * 3, -h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(n1 * 3 + 1, e.second * 3 + 1, -h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(n1 * 3 + 2, e.second * 3 + 2, -h * h * stiffness));
        }
        if (n2 != -1)
        {
            triplets.push_back(Eigen::Triplet<double>(n2 * 3, e.first * 3, -h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(n2 * 3 + 1, e.first * 3 + 1, -h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(n2 * 3 + 2, e.first * 3 + 2, -h * h * stiffness));
            triplets.push_back(Eigen::Triplet<double>(n2 * 3, e.second * 3, h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(n2 * 3 + 1, e.second * 3 + 1, h * h * stiffness));
            triplets.push_back(
                Eigen::Triplet<double>(n2 * 3 + 2, e.second * 3 + 2, h * h * stiffness));
        }
    }
    for (int i = 0; i < n_vertex; i++) {
        triplets.push_back(Eigen::Triplet<double>(i * 3, id_to_point[i] * 3, mass / X.rows()));
        triplets.push_back(Eigen::Triplet<double>(i * 3 + 1, id_to_point[i] * 3 + 1, mass / X.rows()));
        triplets.push_back(Eigen::Triplet<double>(i * 3 + 2, id_to_point[i] * 3 + 2, mass / X.rows()));
    }
    A1.setFromTriplets(triplets.begin(), triplets.end());
    A1.makeCompressed();

    // // print A1 for debugging
    // printf("A1: \n");
    // for (int i = 0; i < 3 * n_vertex; i++) {
    //     for (int j = 0; j < 3 * X.rows(); j++) {
    //         printf("%6.3f ", A1.coeff(i, j));
    //     }
    //     printf("\n");
    // }

    // printf("Original X: \n");
    // for (int i = 0; i < X.rows(); i++) {
    //     printf("X[%d] = (%6.3f, %6.3f, %6.3f)\n", i, X(i, 0), X(i, 1), X(i, 2));
    // }
}

void FastMassSpring::step()
{
    // (HW Optional) Necessary preparation
    unsigned n_vertex = id_to_point.size();
    unsigned n_edges = E.size();
    double mass_per_vertex = mass / X.rows();

    Eigen::Vector3d acceleration_ext = gravity + wind_ext_acc;
    Eigen::MatrixXd acceleration_collision =
        getSphereCollisionForce(sphere_center.cast<double>(), sphere_radius);

    TIC(step)

    auto new_X = X;

    // Calculate y
    Eigen::MatrixXd y = Eigen::MatrixXd::Zero(n_vertex, 3);
    for (int i = 0; i < n_vertex; i++) {
        int point = id_to_point[i];
        y.row(i) = X.row(point) + h * vel.row(point) + h * h * acceleration_ext.transpose();
        if (enable_sphere_collision) {
            y.row(i) += h * h * acceleration_collision.row(point);
        }
    }

    // // print y for debugging, in dense format
    // for (int i = 0; i < n_vertex; i++) {
    //     std::cout << "y[" << i << "]: " << y.row(i) << std::endl;
    // }

    for (unsigned iter = 0; iter < max_iter; iter++) {
        // (HW Optional)
        // local_step and global_step alternating solving

        // // print X on iter
        // std::cout << "                                           iter: " << iter << std::endl;
        // for (int i = 0; i < n_vertex; i++) {
        //     std::cout << "X[" << i << "]: " << X.row(i) << std::endl;
        // }

        // Local step
        // Eigen::MatrixXd d = Eigen::MatrixXd::Zero(n_edges, 3);
        Eigen::VectorXd d = Eigen::VectorXd::Zero(n_edges * 3);
        int cnt = 0;
        for (auto e : E) {
            // d.row(cnt) = E_rest_length[cnt] * (new_X.row(e.first) -
            // new_X.row(e.second)).normalized();
            d(cnt * 3) = E_rest_length[cnt] * (new_X(e.first, 0) - new_X(e.second, 0)) /
                         (new_X.row(e.first) - new_X.row(e.second)).norm();
            d(cnt * 3 + 1) = E_rest_length[cnt] * (new_X(e.first, 1) - new_X(e.second, 1)) /
                             (new_X.row(e.first) - new_X.row(e.second)).norm();
            d(cnt * 3 + 2) = E_rest_length[cnt] * (new_X(e.first, 2) - new_X(e.second, 2)) /
                             (new_X.row(e.first) - new_X.row(e.second)).norm();
            cnt++;
        }

        // // print d
        // for (int i = 0; i < n_edges * 3; i++) {
        //     std::cout << "d[" << i << "]: " << d(i) << std::endl;
        // }

        // Global step

        // Eigen::MatrixXd J = Eigen::MatrixXd::Zero(n_vertex * 3, n_edges * 3);
        // cnt = 0;
        // for (auto e : E) {
        //     int n1 = point_to_id[e.first];
        //     int n2 = point_to_id[e.second];
        //     if (n1 != -1) {
        //         J(n1 * 3, cnt * 3) = stiffness;
        //         J(n1 * 3 + 1, cnt * 3 + 1) = stiffness;
        //         J(n1 * 3 + 2, cnt * 3 + 2) = stiffness;
        //     }
        //     if (n2 != -1) {
        //         J(n2 * 3, cnt * 3) = -stiffness;
        //         J(n2 * 3 + 1, cnt * 3 + 1) = -stiffness;
        //         J(n2 * 3 + 2, cnt * 3 + 2) = -stiffness;
        //     }
        //     cnt++;
        // }

        // Eigen::MatrixXd b = Eigen::MatrixXd::Zero(n_vertices, 3);
        Eigen::VectorXd b = Eigen::VectorXd::Zero(n_vertex * 3);
        cnt = 0;
        for (auto e : E) {
            int n1 = point_to_id[e.first];
            int n2 = point_to_id[e.second];
            if (n1 != -1) {
                b(n1 * 3) += h * h * stiffness * d(cnt * 3);
                b(n1 * 3 + 1) += h * h * stiffness * d(cnt * 3 + 1);
                b(n1 * 3 + 2) += h * h * stiffness * d(cnt * 3 + 2);
            }
            if (n2 != -1) {
                b(n2 * 3) -= h * h * stiffness * d(cnt * 3);
                b(n2 * 3 + 1) -= h * h * stiffness * d(cnt * 3 + 1);
                b(n2 * 3 + 2) -= h * h * stiffness * d(cnt * 3 + 2);
            }
            cnt++;
        }
        b += mass_per_vertex * flatten(y);

        // Add fixed points
        Eigen::VectorXd b_fixed = Eigen::VectorXd::Zero(X.rows() * 3);
        for (int i = 0; i < X.rows(); i++) {
            if (dirichlet_bc_mask[i]) {
                b_fixed(i * 3) = X(i, 0);
                b_fixed(i * 3 + 1) = X(i, 1);
                b_fixed(i * 3 + 2) = X(i, 2);
            }
        }
        b -= A1 * b_fixed;

        // // print b
        // for (int i = 0; i < n_vertex * 3; i++) {
        //     std::cout << "b[" << i << "]: " << b(i) << std::endl;
        // }

        Eigen::VectorXd x = Eigen::VectorXd::Zero(n_vertex * 3);
        x = solver.solve(b);
        if (solver.info() != Eigen::Success) {
            std::cerr << "Solving failed" << std::endl;
        }

        // // print x
        // for (int i = 0; i < n_vertex; i++) {
        //     std::cout << "x[" << i << "]: " << x.segment<3>(i * 3) << std::endl;
        // }

        for (int i = 0; i < n_vertex; i++) {
            int point = id_to_point[i];
            new_X(point, 0) = x(i * 3);
            new_X(point, 1) = x(i * 3 + 1);
            new_X(point, 2) = x(i * 3 + 2);
        }

        // // print answer
        // std::cout << "Answer X: " << std::endl;
        // for (int i = 0; i < X.rows(); i++) {
        //     std::cout << "X[" << i << "]: " << new_X.row(i) << std::endl;
        // }
    }

    // Update velocity
    for (int i = 0; i < X.rows(); i++) {
        // if (!dirichlet_bc_mask[i]) {
        vel.row(i) = (new_X.row(i) - X.row(i)) / h;
        // }
    }
    X = new_X;

    // // print answer
    // std::cout << "Total X: " << std::endl;
    // for (int i = 0; i < X.rows(); i++) {
    //     std::cout << "X[" << i << "]: " << new_X.row(i) << std::endl;
    // }
    // std::cout << "Total vel: " << std::endl;
    // for (int i = 0; i < X.rows(); i++) {
    //     std::cout << "vel[" << i << "]: " << vel.row(i) << std::endl;
    // }
    // std::cout << std::endl;
    // std::cout << std::endl;

    TOC(step)
}

}  // namespace USTC_CG::node_mass_spring
