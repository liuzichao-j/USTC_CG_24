#include "FastMassSpring.h"

#include <iostream>
#include <vector>

namespace USTC_CG::node_mass_spring {
FastMassSpring::FastMassSpring(
    const Eigen::MatrixXd& X,
    const EdgeSet& E,
    const float stiffness,
    const float h,
    const unsigned iter = 100)
    : MassSpring(X, E)
{
    // construct L and J at initialization
    std::cout << "init fast mass spring" << std::endl;

    unsigned n_vertices = X.rows();
    this->stiffness = stiffness;
    this->h = h;
    this->max_iter = iter;

    std::cout << "max iteration times: " << max_iter << std::endl;

    // Renumbering nodes
    point_to_id.assign(n_vertices, -1);
    n_vertex = 0;
    for (int i = 0; i < n_vertices; i++) {
        if (!dirichlet_bc_mask[i]) {
            point_to_id[i] = n_vertex;
            n_vertex++;
        }
    }

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

    fixed.resize(n_vertex, 3);
    fixed.setZero();
    for (auto e : E) {
        int n1 = point_to_id[e.first];
        int n2 = point_to_id[e.second];
        if (n1 != -1) {
            // We have e.first and e.second. Check if they are in dirichlet_bc_mask
            if (dirichlet_bc_mask[e.first]) {
                fixed.row(n1) += X.row(e.first) * h * h * stiffness;
            }
            if (dirichlet_bc_mask[e.second]) {
                fixed.row(n1) -= X.row(e.second) * h * h * stiffness;
            }
        }
        if (n2 != -1) {
            if (dirichlet_bc_mask[e.first]) {
                fixed.row(n2) -= X.row(e.first) * h * h * stiffness;
            }
            if (dirichlet_bc_mask[e.second]) {
                fixed.row(n2) += X.row(e.second) * h * h * stiffness;
            }
        }
    }
}
FastMassSpring::FastMassSpring(
    const Eigen::MatrixXd& X,
    const EdgeSet& E,
    const float stiffness,
    const float h)
    : MassSpring(X, E)
{
    FastMassSpring(X, E, stiffness, h, 100);
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
    Eigen::MatrixXd y = Eigen::MatrixXd::Zero(n_vertex, 3);
    for (int i = 0; i < n_vertices; i++) {
        if (!dirichlet_bc_mask[i]) {
            int id = point_to_id[i];
            y.row(id) = X.row(i) + h * vel.row(i) + h * h * acceleration_ext.transpose();
            if (enable_sphere_collision) {
                y.row(id) += h * h * acceleration_collision.row(i);
            }
        }
    }

    for (unsigned iter = 0; iter < max_iter; iter++) {
        // (HW Optional)
        // local_step and global_step alternating solving

        // Local step
        // Mixed in the global step.

        // Global step
        int cnt = 0;
        Eigen::MatrixXd b = Eigen::MatrixXd::Zero(n_vertex, 3);
        for (auto e : E) {
            int n1 = point_to_id[e.first];
            int n2 = point_to_id[e.second];
            if (n1 != -1) {
                b.row(n1) += h * h * stiffness * E_rest_length[cnt] *
                             (new_X.row(e.first) - new_X.row(e.second)).normalized();
            }
            if (n2 != -1) {
                b.row(n2) -= h * h * stiffness * E_rest_length[cnt] *
                             (new_X.row(e.first) - new_X.row(e.second)).normalized();
            }
            cnt++;
        }
        b += mass_per_vertex * y;

        Eigen::MatrixXd x = Eigen::MatrixXd::Zero(n_vertex, 3);
        x = unflatten(solver.solve(flatten(b - fixed)));
        if (solver.info() != Eigen::Success) {
            std::cerr << "Solving failed" << std::endl;
        }

        for (int i = 0; i < n_vertices; i++) {
            if (!dirichlet_bc_mask[i]) {
                int id = point_to_id[i];
                new_X.row(i) = x.row(id);
            }
        }
    }

    // Update velocity
    for (int i = 0; i < n_vertices; i++) {
        vel.row(i) = (new_X.row(i) - X.row(i)) / h;
    }
    X = new_X;
    collision_correction(X, vel, sphere_center.cast<double>(), sphere_radius);

    TOC(step)
}

void FastMassSpring::init()
{
    // construct L and J at initialization
    std::cout << "init fast mass spring" << std::endl;

    unsigned n_vertices = X.rows();

    // Renumbering nodes
    point_to_id.assign(n_vertices, -1);
    n_vertex = 0;
    for (int i = 0; i < n_vertices; i++) {
        if (!dirichlet_bc_mask[i]) {
            point_to_id[i] = n_vertex;
            n_vertex++;
        }
    }

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

    fixed.resize(n_vertex, 3);
    fixed.setZero();
    for (auto e : E) {
        int n1 = point_to_id[e.first];
        int n2 = point_to_id[e.second];
        if (n1 != -1) {
            // We have e.first and e.second. Check if they are in dirichlet_bc_mask
            if (dirichlet_bc_mask[e.first]) {
                fixed.row(n1) += X.row(e.first) * h * h * stiffness;
            }
            if (dirichlet_bc_mask[e.second]) {
                fixed.row(n1) -= X.row(e.second) * h * h * stiffness;
            }
        }
        if (n2 != -1) {
            if (dirichlet_bc_mask[e.first]) {
                fixed.row(n2) -= X.row(e.first) * h * h * stiffness;
            }
            if (dirichlet_bc_mask[e.second]) {
                fixed.row(n2) += X.row(e.second) * h * h * stiffness;
            }
        }
    }
}

bool FastMassSpring::set_dirichlet_bc_mask(const std::vector<bool>& mask)
{
	if (mask.size() == X.rows())
	{
		dirichlet_bc_mask = mask;
        init();
		return true;
	}
	else
		return false;
}

bool FastMassSpring::update_dirichlet_bc_vertices(const MatrixXd &control_vertices)
{
   for (int i = 0; i < dirichlet_bc_control_pair.size(); i++)
   {
       int idx = dirichlet_bc_control_pair[i].first;
	   int control_idx = dirichlet_bc_control_pair[i].second;
	   X.row(idx) = control_vertices.row(control_idx);
   }
    init();
   return true; 
}

bool FastMassSpring::init_dirichlet_bc_vertices_control_pair(const MatrixXd &control_vertices,
    const std::vector<bool>& control_mask)
{
	if (control_mask.size() != control_vertices.rows())
			return false; 

   // TODO: optimize this part from O(n) to O(1)
   // First, get selected_control_vertices
   std::vector<VectorXd> selected_control_vertices; 
   std::vector<int> selected_control_idx; 
   for (int i = 0; i < control_mask.size(); i++)
   {
       if (control_mask[i])
       {
			selected_control_vertices.push_back(control_vertices.row(i));
            selected_control_idx.push_back(i);
		}
   }

   // Then update mass spring fixed vertices 
   for (int i = 0; i < dirichlet_bc_mask.size(); i++)
   {
       if (dirichlet_bc_mask[i])
       {
           // O(n^2) nearest point search, can be optimized
           // -----------------------------------------
           int nearest_idx = 0;
           double nearst_dist = 1e6; 
           VectorXd X_i = X.row(i);
           for (int j = 0; j < selected_control_vertices.size(); j++)
           {
               double dist = (X_i - selected_control_vertices[j]).norm();
               if (dist < nearst_dist)
               {
				   nearst_dist = dist;
				   nearest_idx = j;
			   }
           }
           //-----------------------------------------
           
		   X.row(i) = selected_control_vertices[nearest_idx];
           dirichlet_bc_control_pair.push_back(std::make_pair(i, selected_control_idx[nearest_idx]));
	   }
   }
   init();
   return true; 
}

}  // namespace USTC_CG::node_mass_spring
