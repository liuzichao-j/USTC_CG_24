#include <Eigen/Dense>
#include <Eigen/SVD>
#include <Eigen/Sparse>

#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"
#include "utils/util_openmesh_bind.h"

/*
** @brief HW5_ARAP_Parameterization
**
** This file presents the basic framework of a "node", which processes inputs
** received from the left and outputs specific variables for downstream nodes to
** use.
**
** - In the first function, node_declare, you can set up the node's input and
** output variables.
**
** - The second function, node_exec is the execution part of the node, where we
** need to implement the node's functionality.
**
** - The third function generates the node's registration information, which
** eventually allows placing this node in the GUI interface.
**
** Your task is to fill in the required logic at the specified locations
** within this template, especially in node_exec.
*/

namespace USTC_CG::node_arap {

inline void flatten_triangles(
    std::shared_ptr<USTC_CG::PolyMesh>& halfedge_mesh,
    std::vector<std::vector<std::pair<int, Eigen::Vector2f>>>& x,
    std::vector<float>& cot_theta)
{
    // Prestep: Implement a piecewise linear mapping from 3D mesh to 2D plane. It may be
    // seperated pieces. It serves as a way to know the relative position of the vertices in the
    // 2D plane. In ARAP and ASAP, rotation is allowed, so we can rotate the pieces.
    for (auto face_handle : halfedge_mesh->faces()) {
        const auto halfedge_handle = face_handle.halfedges().begin().handle();
        const auto ver1 = halfedge_handle.to();
        const auto ver2 = halfedge_handle.next().to();
        const auto ver3 = halfedge_handle.next().next().to();
        const auto point1 = halfedge_mesh->point(ver1);
        const auto point2 = halfedge_mesh->point(ver2);
        const auto point3 = halfedge_mesh->point(ver3);
        x[face_handle.idx()].push_back(std::make_pair(ver1.idx(), Eigen::Vector2f(0, 0)));
        x[face_handle.idx()].push_back(
            std::make_pair(ver2.idx(), Eigen::Vector2f((point2 - point1).norm(), 0)));
        const float theta = acos(
            (point2 - point1).dot(point3 - point1) /
            ((point2 - point1).norm() * (point3 - point1).norm()));
        x[face_handle.idx()].push_back(std::make_pair(
            ver3.idx(),
            Eigen::Vector2f(
                (point3 - point1).norm() * cos(theta), (point3 - point1).norm() * sin(theta))));
        cot_theta[halfedge_handle.idx()] =
            1 / tan(acos((point1 - point2).normalized().dot((point3 - point2).normalized())));
        cot_theta[halfedge_handle.next().idx()] =
            1 / tan(acos((point2 - point3).normalized().dot((point1 - point3).normalized())));
        cot_theta[halfedge_handle.next().next().idx()] =
            1 / tan(acos((point3 - point1).normalized().dot((point2 - point1).normalized())));
    }
}

inline void fix_points(
    std::shared_ptr<USTC_CG::PolyMesh>& halfedge_mesh,
    int& fixed1,
    int& fixed2,
    int option = 0)
{
    // Fix two points. option = 0 for the two points with the largest distance. option = 1 for the
    // two points on the first edge.
    if (option == 0) {
        float maxdist = 0;
        for (auto vertex_handle1 : halfedge_mesh->vertices()) {
            for (auto vertex_handle2 : halfedge_mesh->vertices()) {
                float dist =
                    (halfedge_mesh->point(vertex_handle1) - halfedge_mesh->point(vertex_handle2))
                        .norm();
                if (dist > maxdist) {
                    maxdist = dist;
                    fixed1 = vertex_handle1.idx();
                    fixed2 = vertex_handle2.idx();
                }
            }
        }
    }
    else if (option == 1) {
        auto fixed_edge = halfedge_mesh->edges_begin()->halfedge();
        fixed1 = fixed_edge.from().idx();
        fixed2 = fixed_edge.to().idx();
    }
    return;
}

inline void set_output(
    int& n,
    std::vector<Eigen::Vector2f>& u,
    std::shared_ptr<USTC_CG::PolyMesh>& halfedge_mesh,
    ExeParams& params)
{
    // Get all u within [0, 1] x [0, 1].
    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::min();
    float min_y = std::numeric_limits<float>::max();
    float max_y = std::numeric_limits<float>::min();
    for (int i = 0; i < n; i++) {
        min_x = std::min(min_x, u[i].x());
        max_x = std::max(max_x, u[i].x());
        min_y = std::min(min_y, u[i].y());
        max_y = std::max(max_y, u[i].y());
    }
    for (int i = 0; i < n; i++) {
        u[i].x() = (u[i].x() - min_x) / (max_x - min_x);
        u[i].y() = (u[i].y() - min_y) / (max_y - min_y);
    }
    // Now we have the final u, we can output the result.
    // The result UV coordinates
    pxr::VtArray<pxr::GfVec2f> uv_result(n);
    // uv_result.assign(u.begin(), u.end());
    for (int i = 0; i < n; i++) {
        uv_result.push_back(pxr::GfVec2f(u[i].x(), u[i].y()));
    }
    // The result mesh geometry
    auto operand_base = openmesh_to_operand(halfedge_mesh.get());
    auto& texcoords = operand_base->get_component<MeshComponent>()->texcoordsArray;
    texcoords.clear();
    for (int i = 0; i < n; i++) {
        texcoords.push_back(pxr::GfVec2f(u[i].x(), u[i].y()));
    }
    // The flattened mesh geometry
    auto flattened_mesh = openmesh_to_operand(halfedge_mesh.get());
    for (int i = 0; i < n; i++) {
        flattened_mesh->get_component<MeshComponent>()->vertices[i] =
            pxr::GfVec3f(u[i].x(), u[i].y(), 0);
    }
    // Set the output of the node
    params.set_output("OutputMesh", std::move(*operand_base));
    params.set_output("TexCoords", uv_result);
    params.set_output("FlattenedMesh", std::move(*flattened_mesh));
}

static void node_arap_declare(NodeDeclarationBuilder& b)
{
    // Input-1: Original 3D mesh with boundary
    // Maybe you need to add another input for initialization?
    b.add_input<decl::Geometry>("Input");
    // Initialized input that has UV coordinates generated by min surface process.
    b.add_input<decl::Int>("Iteration Times").min(0).max(10).default_val(1);
    /*
    ** NOTE: You can add more inputs or outputs if necessary. For example, in
    ** some cases, additional information (e.g. other mesh geometry, other
    ** parameters) is required to perform the computation.
    **
    ** Be sure that the input/outputs do not share the same name. You can add
    ** one geometry as
    **
    **                b.add_input<decl::Geometry>("Input");
    **
    ** Or maybe you need a value buffer like:
    **
    **                b.add_input<decl::Float1Buffer>("Weights");
    */
    // Output-1: The UV coordinate of the mesh, provided by ARAP algorithm
    b.add_output<decl::Float2Buffer>("TexCoords");
    // Output-2: The flattened image of the mesh, provided by ARAP algorithm
    b.add_output<decl::Geometry>("OutputMesh");
    // Output-3: The flattened image of the mesh, provided by ARAP algorithm
    b.add_output<decl::Geometry>("FlattenedMesh");
}

static void node_arap_exec(ExeParams params)
{
    // Get the input from params
    auto input = params.get_input<GOperandBase>("Input");
    int iteration_times = params.get_input<int>("Iteration Times");

    // Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Need Geometry Input.");
    }
    // throw std::runtime_error("Not implemented");
    /* ----------------------------- Preprocess -------------------------------
    ** Create a halfedge structure (using OpenMesh) for the input mesh. The
    ** half-edge data structure is a widely used data structure in geometric
    ** processing, offering convenient operations for traversing and modifying
    ** mesh elements.
    */
    auto halfedge_mesh = operand_to_openmesh(&input);
    int n = halfedge_mesh->n_vertices();
    int h = halfedge_mesh->n_halfedges();
    int t = halfedge_mesh->n_faces();

    /* ------------- [HW5_TODO] ARAP Parameterization Implementation -----------
    ** Implement ARAP mesh parameterization to minimize local distortion.
    **
    ** Steps:
    ** 1. Initial Setup: Use a HW4 parameterization result as initial setup.
    **
    ** 2. Local Phase: For each triangle, compute local orthogonal approximation
    **    (Lt) by computing SVD of Jacobian(Jt) with fixed u.
    **
    ** 3. Global Phase: With Lt fixed, update parameter coordinates(u) by solving
    **    a pre-factored global sparse linear system.
    **
    ** 4. Iteration: Repeat Steps 2 and 3 to refine parameterization.
    **
    ** Note:
    **  - Fixed points' selection is crucial for ARAP and ASAP.
    **  - Encapsulate algorithms into classes for modularity.
    */

    // We have variables: u for each vertex, L for each triangle.
    std::vector<Eigen::Vector2f> u(n);
    std::vector<Eigen::Matrix2f> L(t);
    // Initialize u with the UV coordinates from Input.
    for (int i = 0; i < n; i++) {
        u[i] = Eigen::Vector2f(input.get_component<MeshComponent>()->texcoordsArray[i].data());
    }

    // Step 1-Initial Setup: Use a HW4 parameterization result as initial setup.
    // This step is implemented by giving initialized input.

    // Prestep 1: Implement a piecewise linear mapping from 3D mesh to 2D plane. It may be
    // seperated pieces. It serves as a way to know the relative position of the vertices in the
    // 2D plane. In ARAP and ASAP, rotation is allowed, so we can rotate the pieces.
    std::vector<std::vector<std::pair<int, Eigen::Vector2f>>> x(t);
    std::vector<float> cot_theta(h, 0.0f);
    flatten_triangles(halfedge_mesh, x, cot_theta);

    // Prestep 2: Have the global sparse linear system pre-factored.
    // Solve the equations: Sum_{j \in N(i)} (cot(\theta_ij) + cot(\theta_ji)) * (u_i - u_j) =
    // Sum_{j \in N(i)} (cot(\theta_ij) L_t(i, j) + cot(\theta_ji) L_t(j, i)) * (x_i - x_j), where
    // N(i) is the set of neighbors of i, \theta_ij is the angle that is opposite to the edge (i, j)
    // in surface t, L_t(i, j) is L in the surface t that contains the halfedge (i, j).
    // This equation depends only on x and structure of the mesh, so we can pre-factored it.
    Eigen::SparseMatrix<float> A(n, n);
    std::vector<Eigen::Triplet<float>> triplets;
    Eigen::MatrixXf b = Eigen::MatrixXf::Zero(n, 2);
    Eigen::SparseLU<Eigen::SparseMatrix<float>> solver;
    // Calculate A and b.
    for (auto vertex_handle : halfedge_mesh->vertices()) {
        for (auto halfedge_handle : vertex_handle.outgoing_halfedges()) {
            const int from = vertex_handle.idx();
            const int to = halfedge_handle.to().idx();
            const float cot =
                cot_theta[halfedge_handle.idx()] + cot_theta[halfedge_handle.opp().idx()];
            triplets.push_back(Eigen::Triplet<float>(from, from, cot));
            triplets.push_back(Eigen::Triplet<float>(from, to, -cot));
        }
    }
    A.setFromTriplets(triplets.begin(), triplets.end());
    // Fix two points.
    // int fixed1 = -1, fixed2 = -1;
    // fix_points(halfedge_mesh, fixed1, fixed2, 0);
    // for (int i = 0; i < n; i++) {
    //     if (i != fixed1) {
    //         A.coeffRef(fixed1, i) = 0;
    //     }
    //     else {
    //         A.coeffRef(fixed1, i) = 1;
    //     }
    // }
    // for (int i = 0; i < n; i++) {
    //     if (i != fixed2) {
    //         A.coeffRef(fixed2, i) = 0;
    //     }
    //     else {
    //         A.coeffRef(fixed2, i) = 1;
    //     }
    // }
    solver.compute(A);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Decomposition failed.");
    }

    // We will iterate the following steps.
    while (iteration_times-- > 0) {
        // Step 2-Local Phase: For each triangle, compute local orthogonal approximation (Lt) by
        // computing SVD of Jacobian(Jt) with fixed u.

        // Equivalently, use "cross-covariance matrix" S_t(u) instead of J_t(u).
        // S_t(u) = Sum_{i=0}^{2} cot(\theta_t^i) (u_t^i - u_t^{i+1}) (x_t^i - x_t^{i+1})^T
        // SVD decompose S_t(u) = U_t * Sigma_t * V_t^T. The best Lt = U_t * V_t^T.
        for (auto face_handle : halfedge_mesh->faces()) {
            const auto halfedge_handle = face_handle.halfedges().begin().handle();
            const auto x1 = x[face_handle.idx()][0].second;
            const auto x2 = x[face_handle.idx()][1].second;
            const auto x3 = x[face_handle.idx()][2].second;
            const auto u1 = u[halfedge_handle.to().idx()];
            const auto u2 = u[halfedge_handle.next().to().idx()];
            const auto u3 = u[halfedge_handle.next().next().to().idx()];
            Eigen::Matrix2f S = Eigen::Matrix2f::Zero();
            S += cot_theta[halfedge_handle.next().idx()] * (u1 - u2) * (x1 - x2).transpose();
            S += cot_theta[halfedge_handle.next().next().idx()] * (u2 - u3) * (x2 - x3).transpose();
            S += cot_theta[halfedge_handle.idx()] * (u3 - u1) * (x3 - x1).transpose();
            Eigen::JacobiSVD<Eigen::Matrix2f> svd(S, Eigen::ComputeFullU | Eigen::ComputeFullV);
            L[face_handle.idx()] = svd.matrixU() * svd.matrixV().transpose();
            // auto I = L[face_handle.idx()] * L[face_handle.idx()].transpose() -
            // Eigen::Matrix2f::Identity(); if (I.norm() > 1e-6) {
            //     printf("We have U = {%.6f, %.6f, %.6f, %.6f}, V = {%.6f, %.6f, %.6f, %.6f}\n",
            //            svd.matrixU()(0, 0), svd.matrixU()(0, 1), svd.matrixU()(1, 0),
            //            svd.matrixU()(1, 1), svd.matrixV()(0, 0), svd.matrixV()(0, 1),
            //            svd.matrixV()(1, 0), svd.matrixV()(1, 1));
            //     printf("We have L = {%.6f, %.6f, %.6f, %.6f}\n", L[face_handle.idx()](0, 0),
            //            L[face_handle.idx()](0, 1), L[face_handle.idx()](1, 0),
            //            L[face_handle.idx()](1, 1));
            //     printf("We have sigma = {%.6f, %.6f}\n", svd.singularValues()(0),
            //            svd.singularValues()(1));
            //     printf("We have I = {%.6f, %.6f, %.6f, %.6f}\n", I(0, 0), I(0, 1), I(1, 0),
            //            I(1, 1));
            //     throw std::runtime_error("SVD failed.");
            // }
            // printf(
            //     "arap: L_%d = {%.6f, %.6f, %.6f, %.6f}\n",
            //     face_handle.idx(),
            //     L[face_handle.idx()](0, 0),
            //     L[face_handle.idx()](0, 1),
            //     L[face_handle.idx()](1, 0),
            //     L[face_handle.idx()](1, 1));
        }

        // Step 3-Global Phase: With Lt fixed, update parameter coordinates(u) by solving a
        // pre-factored global sparse linear system.

        // First set b.
        b = Eigen::MatrixXf::Zero(n, 2);
        for (auto face_handle : halfedge_mesh->faces()) {
            const auto halfedge_handle = face_handle.halfedges().begin().handle();
            const auto x1 = x[face_handle.idx()][0].second;
            const auto x2 = x[face_handle.idx()][1].second;
            const auto x3 = x[face_handle.idx()][2].second;
            const auto u1 = u[halfedge_handle.to().idx()];
            const auto u2 = u[halfedge_handle.next().to().idx()];
            const auto u3 = u[halfedge_handle.next().next().to().idx()];
            b.row(halfedge_handle.to().idx()) +=
                cot_theta[halfedge_handle.next().idx()] * L[face_handle.idx()] * (x1 - x2);
            b.row(halfedge_handle.next().to().idx()) +=
                cot_theta[halfedge_handle.next().next().idx()] * L[face_handle.idx()] * (x2 - x3);
            b.row(halfedge_handle.next().next().to().idx()) +=
                cot_theta[halfedge_handle.idx()] * L[face_handle.idx()] * (x3 - x1);
            b.row(halfedge_handle.next().to().idx()) +=
                cot_theta[halfedge_handle.next().idx()] * L[face_handle.idx()] * (x2 - x1);
            b.row(halfedge_handle.next().next().to().idx()) +=
                cot_theta[halfedge_handle.next().next().idx()] * L[face_handle.idx()] * (x3 - x2);
            b.row(halfedge_handle.to().idx()) +=
                cot_theta[halfedge_handle.idx()] * L[face_handle.idx()] * (x1 - x3);
        }
        // for (auto vertex_handle : halfedge_mesh->vertices()) {
        //     for (auto halfedge_handle : vertex_handle.outgoing_halfedges()) {
        //         const int from = vertex_handle.idx();
        //         const int to = halfedge_handle.to().idx();
        //         const OpenMesh::SmartFaceHandle face[2] = { halfedge_handle.face(),
        //                                                     halfedge_handle.opp().face() };
        //         float cot[2];
        //         for (int i = 0; i < 2; i++) {
        //             if (face[i].is_valid()) {
        //                 for (int j = 0; j < 3; j++) {
        //                     int k = (j + 1) % 3;
        //                     if ((x[face[i].idx()][j].first == from &&
        //                          x[face[i].idx()][k].first == to) ||
        //                         (x[face[i].idx()][j].first == to &&
        //                          x[face[i].idx()][k].first == from)) {
        //                         int l = (j + 2) % 3;
        //                         cot[i] = 1 / tan(acos(((x[face[i].idx()][j].second -
        //                                                 x[face[i].idx()][l].second)
        //                                                    .normalized())
        //                                                   .dot(((x[face[i].idx()][k].second -
        //                                                          x[face[i].idx()][l].second)
        //                                                             .normalized()))));
        //                         if (x[face[i].idx()][j].first == from) {
        //                             b.row(from) +=
        //                                 cot[i] * L[face[i].idx()] *
        //                                 (x[face[i].idx()][j].second -
        //                                 x[face[i].idx()][k].second);
        //                         }
        //                         else {
        //                             b.row(from) +=
        //                                 cot[i] * L[face[i].idx()] *
        //                                 (x[face[i].idx()][k].second -
        //                                 x[face[i].idx()][j].second);
        //                         }
        //                         break;
        //                     }
        //                 }
        //             }
        //         }
        //     }
        // }
        // b.row(fixed1) = Eigen::Vector2f(0, 0);
        // b.row(fixed2) = Eigen::Vector2f(u[fixed2].x(), u[fixed2].y());
        // for (int i = 0; i < 3; i++) {
        //     if (x[fixed_face_idx][i].first == fixed1) {
        //         b.row(fixed1) = Eigen::Vector2f(
        //             x[fixed_face_idx][i].second.x(), x[fixed_face_idx][i].second.y());
        //     }
        //     if (x[fixed_face_idx][i].first == fixed2) {
        //         b.row(fixed2) = Eigen::Vector2f(
        //             x[fixed_face_idx][i].second.x(), x[fixed_face_idx][i].second.y());
        //     }
        // }
        // Solve the linear system.
        Eigen::MatrixXf u_new = solver.solve(b);
        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Solve failed.");
        }
        // Update u.
        for (auto vertex_handle : halfedge_mesh->vertices()) {
            u[vertex_handle.idx()] = u_new.row(vertex_handle.idx());
        }
    }

    // Step 4-Iteration: Repeat Steps 2 and 3 to refine parameterization.
    // Implemented by the while loop.
    set_output(n, u, halfedge_mesh, params);
}

static void node_asap_declare(NodeDeclarationBuilder& b)
{
    b.add_input<decl::Geometry>("Input");

    b.add_output<decl::Float2Buffer>("TexCoords");
    b.add_output<decl::Geometry>("OutputMesh");
    b.add_output<decl::Geometry>("FlattenedMesh");
}

static void node_asap_exec(ExeParams params)
{
    auto input = params.get_input<GOperandBase>("Input");

    // Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Need Geometry Input.");
    }

    /* ----------------------------- Preprocess -------------------------------
    ** Create a halfedge structure (using OpenMesh) for the input mesh. The
    ** half-edge data structure is a widely used data structure in geometric
    ** processing, offering convenient operations for traversing and modifying
    ** mesh elements.
    */
    auto halfedge_mesh = operand_to_openmesh(&input);
    int n = halfedge_mesh->n_vertices();
    int h = halfedge_mesh->n_halfedges();
    int t = halfedge_mesh->n_faces();

    // Prestep: Implement a piecewise linear mapping from 3D mesh to 2D plane. It may be
    // seperated pieces. It serves as a way to know the relative position of the vertices in the
    // 2D plane. In ARAP and ASAP, rotation is allowed, so we can rotate the pieces.
    std::vector<std::vector<std::pair<int, Eigen::Vector2f>>> x(t);
    std::vector<float> cot_theta(h);
    flatten_triangles(halfedge_mesh, x, cot_theta);

    // We have energy E = 1/2 * Sum_{t=1}^T (Sum_{i=0}^2 cot(theta_t^i) ||(u_t^i - u_t^(i+1)) -
    // {(a_t b_t), (-b_t, a_t)} * (x_t^i - x_t^(i+1))||^2)

    // 2n + 2t variables: u(ux, uy) for each vertex and (a_t, b_t) for each triangle, where L = {(a,
    // b), (-b, a)}
    Eigen::SparseMatrix<float> A(2 * (n + t), 2 * (n + t));
    std::vector<Eigen::Triplet<float>> triplets;
    Eigen::VectorXf b = Eigen::VectorXf::Zero(2 * (n + t));
    Eigen::SparseLU<Eigen::SparseMatrix<float>> solver;
    // Minimize E with respect to u, a_t and b_t.
    // Calculate A and b.

    // We notice that these equations are linear, without constant term. So we need to fix two
    // points to avoid zero solution.
    int fixed1 = -1, fixed2 = -1;
    fix_points(halfedge_mesh, fixed1, fixed2, 0);

    // For u, it is same as ARAP. Sum_{j \in N(i)} (cot(\theta_ij) + cot(\theta_ji)) * (u_i - u_j) =
    // Sum_{j \in N(i)} (cot(\theta_ij) L_t(i, j) + cot(\theta_ji) L_t(j, i)) * (x_i - x_j)
    for (auto vertex_handle : halfedge_mesh->vertices()) {
        const int from = vertex_handle.idx();
        if (from == fixed1 || from == fixed2) {
            triplets.push_back(Eigen::Triplet<float>(2 * from, 2 * from, 1));
            triplets.push_back(Eigen::Triplet<float>(2 * from + 1, 2 * from + 1, 1));
            continue;
        }
        for (auto halfedge_handle : vertex_handle.outgoing_halfedges()) {
            const int to = halfedge_handle.to().idx();
            const int face = halfedge_handle.face().idx();
            const int faceopp = halfedge_handle.opp().face().idx();
            const int edge = halfedge_handle.idx();
            const int edgeopp = halfedge_handle.opp().idx();
            int id1 = -1, id2 = -1, id3 = -1, id4 = -1;
            for (int i = 0; i < 3; i++) {
                if (face != -1 && x[face][i].first == from) {
                    id1 = i;
                }
                if (face != -1 && x[face][i].first == to) {
                    id2 = i;
                }
                if (faceopp != -1 && x[faceopp][i].first == from) {
                    id3 = i;
                }
                if (faceopp != -1 && x[faceopp][i].first == to) {
                    id4 = i;
                }
            }
            // printf("from = %d, to = %d, face = %d, faceopp = %d, id1 = %d, id2 = %d, id3 = %d,
            // id4 = %d\n", from, to, face, faceopp, id1, id2, id3, id4);
            // x axis
            triplets.push_back(
                Eigen::Triplet<float>(2 * from, 2 * from, cot_theta[edge] + cot_theta[edgeopp]));
            triplets.push_back(
                Eigen::Triplet<float>(2 * from, 2 * to, -cot_theta[edge] - cot_theta[edgeopp]));
            if (face != -1) {
                triplets.push_back(Eigen::Triplet<float>(
                    2 * from,
                    2 * n + 2 * face,
                    -cot_theta[edge] * (x[face][id1].second - x[face][id2].second).x()));
                triplets.push_back(Eigen::Triplet<float>(
                    2 * from,
                    2 * n + 2 * face + 1,
                    -cot_theta[edge] * (x[face][id1].second - x[face][id2].second).y()));
            }
            if (faceopp != -1) {
                triplets.push_back(Eigen::Triplet<float>(
                    2 * from,
                    2 * n + 2 * faceopp,
                    -cot_theta[edgeopp] * (x[faceopp][id3].second - x[faceopp][id4].second).x()));
                triplets.push_back(Eigen::Triplet<float>(
                    2 * from,
                    2 * n + 2 * faceopp + 1,
                    -cot_theta[edgeopp] * (x[faceopp][id3].second - x[faceopp][id4].second).y()));
            }
            // y axis
            triplets.push_back(Eigen::Triplet<float>(
                2 * from + 1, 2 * from + 1, cot_theta[edge] + cot_theta[edgeopp]));
            triplets.push_back(Eigen::Triplet<float>(
                2 * from + 1, 2 * to + 1, -cot_theta[edge] - cot_theta[edgeopp]));
            if (face != -1) {
                triplets.push_back(Eigen::Triplet<float>(
                    2 * from + 1,
                    2 * n + 2 * face,
                    -cot_theta[edge] * (x[face][id1].second - x[face][id2].second).y()));
                triplets.push_back(Eigen::Triplet<float>(
                    2 * from + 1,
                    2 * n + 2 * face + 1,
                    cot_theta[edge] * (x[face][id1].second - x[face][id2].second).x()));
            }
            if (faceopp != -1) {
                triplets.push_back(Eigen::Triplet<float>(
                    2 * from + 1,
                    2 * n + 2 * faceopp,
                    -cot_theta[edgeopp] * (x[faceopp][id3].second - x[faceopp][id4].second).y()));
                triplets.push_back(Eigen::Triplet<float>(
                    2 * from + 1,
                    2 * n + 2 * faceopp + 1,
                    cot_theta[edgeopp] * (x[faceopp][id3].second - x[faceopp][id4].second).x()));
            }
        }
    }
    for (auto face_handle : halfedge_mesh->faces()) {
        const int face = face_handle.idx();
        const auto halfedge_handle = face_handle.halfedges().begin().handle();
        const auto x1 = x[face][0].second;
        const auto x2 = x[face][1].second;
        const auto x3 = x[face][2].second;
        const int point1 = halfedge_handle.to().idx();
        const int point2 = halfedge_handle.next().to().idx();
        const int point3 = halfedge_handle.next().next().to().idx();
        const int edge1 = halfedge_handle.next().idx();
        const int edge2 = halfedge_handle.next().next().idx();
        const int edge3 = halfedge_handle.idx();
        // for a_t: Sum_{i=0}^2 cot(theta_t^i) ((u^i_1-u^(i+1)_1 - a(x^i_1-x^(i+1)_1) -
        // b(x^i_2-x^(i+1)_2))(x^i_1-x^(i+1)_1) + (u^i_2-u^(i+1)_2 + b(x^i_1-x^(i+1)_1) -
        // a(x^i_2-x^(i+1)_2))(x^i_2-x^(i+1)_2)) = 0.
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * point1,
            cot_theta[edge1] * (x1 - x2).x() - cot_theta[edge3] * (x3 - x1).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * point2,
            cot_theta[edge2] * (x2 - x3).x() - cot_theta[edge1] * (x1 - x2).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * point3,
            cot_theta[edge3] * (x3 - x1).x() - cot_theta[edge2] * (x2 - x3).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * n + 2 * face,
            -cot_theta[edge1] * (x1 - x2).x() * (x1 - x2).x() -
                cot_theta[edge2] * (x2 - x3).x() * (x2 - x3).x() -
                cot_theta[edge3] * (x3 - x1).x() * (x3 - x1).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * n + 2 * face + 1,
            -cot_theta[edge1] * (x1 - x2).y() * (x1 - x2).x() -
                cot_theta[edge2] * (x2 - x3).y() * (x2 - x3).x() -
                cot_theta[edge3] * (x3 - x1).y() * (x3 - x1).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * point1 + 1,
            cot_theta[edge1] * (x1 - x2).y() - cot_theta[edge3] * (x3 - x1).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * point2 + 1,
            cot_theta[edge2] * (x2 - x3).y() - cot_theta[edge1] * (x1 - x2).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * point3 + 1,
            cot_theta[edge3] * (x3 - x1).y() - cot_theta[edge2] * (x2 - x3).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * n + 2 * face,
            -cot_theta[edge1] * (x1 - x2).y() * (x1 - x2).y() -
                cot_theta[edge2] * (x2 - x3).y() * (x2 - x3).y() -
                cot_theta[edge3] * (x3 - x1).y() * (x3 - x1).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face,
            2 * n + 2 * face + 1,
            cot_theta[edge1] * (x1 - x2).x() * (x1 - x2).y() +
                cot_theta[edge2] * (x2 - x3).x() * (x2 - x3).y() +
                cot_theta[edge3] * (x3 - x1).x() * (x3 - x1).y()));
        // for b_t: Sum_{i=0}^2 cot(theta_t^i) ((u^i_1-u^(i+1)_1 - a(x^i_1-x^(i+1)_1) -
        // b(x^i_2-x^(i+1)_2))(x^i_2-x^(i+1)_2) - (u^i_2-u^(i+1)_2 + b(x^i_1-x^(i+1)_1) -
        // a(x^i_2-x^(i+1)_2))(x^i_1-x^(i+1)_1)) = 0.
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * point1,
            cot_theta[edge1] * (x1 - x2).y() - cot_theta[edge3] * (x3 - x1).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * point2,
            cot_theta[edge2] * (x2 - x3).y() - cot_theta[edge1] * (x1 - x2).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * point3,
            cot_theta[edge3] * (x3 - x1).y() - cot_theta[edge2] * (x2 - x3).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * n + 2 * face,
            -cot_theta[edge1] * (x1 - x2).x() * (x1 - x2).y() -
                cot_theta[edge2] * (x2 - x3).x() * (x2 - x3).y() -
                cot_theta[edge3] * (x3 - x1).x() * (x3 - x1).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * n + 2 * face + 1,
            -cot_theta[edge1] * (x1 - x2).y() * (x1 - x2).y() -
                cot_theta[edge2] * (x2 - x3).y() * (x2 - x3).y() -
                cot_theta[edge3] * (x3 - x1).y() * (x3 - x1).y()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * point1 + 1,
            -cot_theta[edge1] * (x1 - x2).x() + cot_theta[edge3] * (x3 - x1).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * point2 + 1,
            -cot_theta[edge2] * (x2 - x3).x() + cot_theta[edge1] * (x1 - x2).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * point3 + 1,
            -cot_theta[edge3] * (x3 - x1).x() + cot_theta[edge2] * (x2 - x3).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * n + 2 * face + 1,
            -cot_theta[edge1] * (x1 - x2).x() * (x1 - x2).x() -
                cot_theta[edge2] * (x2 - x3).x() * (x2 - x3).x() -
                cot_theta[edge3] * (x3 - x1).x() * (x3 - x1).x()));
        triplets.push_back(Eigen::Triplet<float>(
            2 * n + 2 * face + 1,
            2 * n + 2 * face,
            cot_theta[edge1] * (x1 - x2).y() * (x1 - x2).x() +
                cot_theta[edge2] * (x2 - x3).y() * (x2 - x3).x() +
                cot_theta[edge3] * (x3 - x1).y() * (x3 - x1).x()));
    }
    A.setFromTriplets(triplets.begin(), triplets.end());
    // for (int i = 0; i < 2 * n + 2 * t; i++) {
    //     for (int j = 0; j < 2 * n + 2 * t; j++) {
    //         printf("%.6f ", A.coeff(i, j));
    //     }
    //     printf("\n");
    // }
    // for (int i = 0; i < 3; i++) {
    //     if (x[fixed_face_idx][i].first == fixed1) {
    //         b(2 * fixed1) = x[fixed_face_idx][i].second.x();
    //         b(2 * fixed1 + 1) = x[fixed_face_idx][i].second.y();
    //         printf(
    //             "fixed1: %d, (%.6f, %.6f)\n",
    //             fixed1,
    //             x[fixed_face_idx][i].second.x(),
    //             x[fixed_face_idx][i].second.y());
    //     }
    //     if (x[fixed_face_idx][i].first == fixed2) {
    //         b(2 * fixed2) = x[fixed_face_idx][i].second.x();
    //         b(2 * fixed2 + 1) = x[fixed_face_idx][i].second.y();
    //         printf(
    //             "fixed2: %d, (%.6f, %.6f)\n",
    //             fixed2,
    //             x[fixed_face_idx][i].second.x(),
    //             x[fixed_face_idx][i].second.y());
    //     }
    // }
    b(2 * fixed1) = 0;
    b(2 * fixed1 + 1) = 0;
    b(2 * fixed2) = 1;
    b(2 * fixed2 + 1) = 0;

    solver.compute(A);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Decomposition failed.");
    }
    Eigen::VectorXf sol = solver.solve(b);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Solve failed.");
    }
    // for (int i = 0; i < n; i++) {
    //     printf("solution u_%d = (%f, %f)\n", i, sol[2 * i], sol[2 * i + 1]);
    // }
    // for (int i = 0; i < t; i++) {
    //     printf("solution a_%d = %f, b_%d = %f\n", i, sol[2 * n + 2 * i], i, sol[2 * n + 2 * i +
    //     1]);
    // }
    std::vector<Eigen::Vector2f> u(n);
    for (int i = 0; i < n; i++) {
        u[i] = Eigen::Vector2f(sol[2 * i], sol[2 * i + 1]);
    }
    set_output(n, u, halfedge_mesh, params);
}

inline std::vector<double> solve_cubic(double a, double c, double d)
{
    std::vector<double> ans(0);
    if (a == 0) {
        // Special case: cx + d = 0
        ans.push_back((c == 0 ? 0 : -d / c));
    }
    else {
        // Special case: ax^3 + cx + d = 0
        // Transform to x^3 + px + q = 0
        double p = c / a;
        double q = d / a;
        double delta = (q * q) / 4 + (p * p * p) / 27;
        if (delta > 0) {
            // 1 real root
            ans.push_back(
                std::pow(-q / 2 + sqrt(delta), 1 / 3.0) + std::pow(-q / 2 - sqrt(delta), 1 / 3.0));
        }
        else if (delta == 0) {
            // 2 real roots
            ans.push_back(2 * std::pow(-q / 2, 1 / 3.0));
        }
        else {
            // 3 real roots
            double r = sqrt(-(p * p * p) / 27);
            double theta = acos(-q / (2 * r)) / 3.0;
            ans.push_back(2 * std::pow(r, 1 / 3.0) * cos(theta));
            ans.push_back(2 * std::pow(r, 1 / 3.0) * cos(theta + 2 * M_PI / 3));
            ans.push_back(2 * std::pow(r, 1 / 3.0) * cos(theta + 4 * M_PI / 3));
        }
    }
    return ans;
}

static void node_hybrid_declare(NodeDeclarationBuilder& b)
{
    b.add_input<decl::Geometry>("Input");
    b.add_input<decl::Int>("Iteration Times").min(0).max(10).default_val(1);
    b.add_input<decl::Float>("Lambda").min(0.0f).max(10000.0).default_val(0.0f);

    b.add_output<decl::Float2Buffer>("TexCoords");
    b.add_output<decl::Geometry>("OutputMesh");
    b.add_output<decl::Geometry>("FlattenedMesh");
}

static void node_hybrid_exec(ExeParams params)
{
    // Get the input from params
    auto input = params.get_input<GOperandBase>("Input");
    int iteration_times = params.get_input<int>("Iteration Times");
    float lambda = params.get_input<float>("Lambda");

    // Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Need Geometry Input.");
    }

    auto halfedge_mesh = operand_to_openmesh(&input);
    int n = halfedge_mesh->n_vertices();
    int h = halfedge_mesh->n_halfedges();
    int t = halfedge_mesh->n_faces();

    // We have variables: u for each vertex, L for each triangle.
    std::vector<Eigen::Vector2f> u(n);
    std::vector<Eigen::Matrix2f> L(t);
    // Initialize u with the UV coordinates from Input.
    for (int i = 0; i < n; i++) {
        u[i] = Eigen::Vector2f(input.get_component<MeshComponent>()->texcoordsArray[i].data());
    }

    // Step 1-Initial Setup: Use a HW4 parameterization result as initial setup.
    // This step is implemented by giving initialized input.

    // Prestep 1: Implement a piecewise linear mapping from 3D mesh to 2D plane. It may be
    // seperated pieces. It serves as a way to know the relative position of the vertices in the
    // 2D plane. In ARAP and ASAP, rotation is allowed, so we can rotate the pieces.
    std::vector<std::vector<std::pair<int, Eigen::Vector2f>>> x(t);
    std::vector<float> cot_theta(h, 0.0f);
    flatten_triangles(halfedge_mesh, x, cot_theta);

    // Prestep 2: Have the global sparse linear system pre-factored.
    // Solve the equations: Sum_{j \in N(i)} (cot(\theta_ij) + cot(\theta_ji)) * (u_i - u_j) =
    // Sum_{j \in N(i)} (cot(\theta_ij) L_t(i, j) + cot(\theta_ji) L_t(j, i)) * (x_i - x_j), where
    // N(i) is the set of neighbors of i, \theta_ij is the angle that is opposite to the edge (i, j)
    // in surface t, L_t(i, j) is L in the surface t that contains the halfedge (i, j).
    // This equation depends only on x and structure of the mesh, so we can pre-factored it.
    Eigen::SparseMatrix<float> A(n, n);
    std::vector<Eigen::Triplet<float>> triplets;
    Eigen::MatrixXf b = Eigen::MatrixXf::Zero(n, 2);
    Eigen::SparseLU<Eigen::SparseMatrix<float>> solver;
    // Calculate A and b.
    for (auto vertex_handle : halfedge_mesh->vertices()) {
        for (auto halfedge_handle : vertex_handle.outgoing_halfedges()) {
            const int from = vertex_handle.idx();
            const int to = halfedge_handle.to().idx();
            const float cot =
                cot_theta[halfedge_handle.idx()] + cot_theta[halfedge_handle.opp().idx()];
            triplets.push_back(Eigen::Triplet<float>(from, from, cot));
            triplets.push_back(Eigen::Triplet<float>(from, to, -cot));
        }
    }
    A.setFromTriplets(triplets.begin(), triplets.end());
    // Fix two points.
    int fixed1 = -1, fixed2 = -1;
    fix_points(halfedge_mesh, fixed1, fixed2, 0);
    for (int i = 0; i < n; i++) {
        A.coeffRef(fixed1, i) = (i == fixed1) ? 1 : 0;
    }
    for (int i = 0; i < n; i++) {
        A.coeffRef(fixed2, i) = (i == fixed2) ? 1 : 0;
    }
    solver.compute(A);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Decomposition failed.");
    }

    // We will iterate the following steps.
    while (iteration_times-- > 0) {
        // Step 2-Local Phase: For each triangle, compute local orthogonal approximation (Lt) by
        // computing SVD of Jacobian(Jt) with fixed u.

        // For hybrid method, we have:
        // 2*lambda * (C_2^2 + C_3^2) / C_2^2 a^3 + (C_1 - 2*lambda) a - C_2 = 0
        // b = C_3 / C_2 * a
        // C_1 = Sum_{i=0}^2 cot(theta_t^i) ||x_t^i - x_t^(i+1)||^2
        // C_2 = Sum_{i=0}^2 cot(theta_t^i) (u_t^i - u_t^(i+1)) * (x_t^i - x_t^(i+1))
        // C_3 = Sum_{i=0}^2 cot(theta_t^i) (u_t^i - u_t^(i+1))_1 * (x_t^i - x_t^(i+1))_2 - (u_t^i -
        // u_t^(i+1))_2 * (x_t^i - x_t^(i+1))_1
        for (auto face_handle : halfedge_mesh->faces()) {
            const auto halfedge_handle = face_handle.halfedges().begin().handle();
            const auto x1 = x[face_handle.idx()][0].second;
            const auto x2 = x[face_handle.idx()][1].second;
            const auto x3 = x[face_handle.idx()][2].second;
            const auto u1 = u[halfedge_handle.to().idx()];
            const auto u2 = u[halfedge_handle.next().to().idx()];
            const auto u3 = u[halfedge_handle.next().next().to().idx()];
            const float cot1 = cot_theta[halfedge_handle.next().idx()];
            const float cot2 = cot_theta[halfedge_handle.next().next().idx()];
            const float cot3 = cot_theta[halfedge_handle.idx()];
            const double C_1 = cot1 * (x1 - x2).dot(x1 - x2) + cot2 * (x2 - x3).dot(x2 - x3) +
                               cot3 * (x3 - x1).dot(x3 - x1);
            const double C_2 = cot1 * (u1 - u2).dot(x1 - x2) + cot2 * (u2 - u3).dot(x2 - x3) +
                               cot3 * (u3 - u1).dot(x3 - x1);
            const double C_3 =
                cot1 * ((u1 - u2).x() * (x1 - x2).y() - (u1 - u2).y() * (x1 - x2).x()) +
                cot2 * ((u2 - u3).x() * (x2 - x3).y() - (u2 - u3).y() * (x2 - x3).x()) +
                cot3 * ((u3 - u1).x() * (x3 - x1).y() - (u3 - u1).y() * (x3 - x1).x());
            std::vector<double> as = solve_cubic(
                2 * lambda * (C_2 * C_2 + C_3 * C_3),
                (C_1 - 2 * lambda) * C_2 * C_2,
                -C_2 * C_2 * C_2);
            std::vector<double> bs = solve_cubic(
                2 * lambda * (C_2 * C_2 + C_3 * C_3),
                (C_1 - 2 * lambda) * C_3 * C_3,
                -C_3 * C_3 * C_3);
            double min_a = 0, min_b = 0;
            // Check which one has the minimal energy.
            double min_energy = std::numeric_limits<double>::max();
            for (auto a : as) {
                for (auto b : bs) {
                    double energy =
                        cot1 *
                            (std::pow((u1 - u2).x() - a * (x1 - x2).x() - b * (x1 - x2).y(), 2) +
                             std::pow((u1 - u2).y() + b * (x1 - x2).x() - a * (x1 - x2).y(), 2)) +
                        cot2 *
                            (std::pow((u2 - u3).x() - a * (x2 - x3).x() - b * (x2 - x3).y(), 2) +
                             std::pow((u2 - u3).y() + b * (x2 - x3).x() - a * (x2 - x3).y(), 2)) +
                        cot3 *
                            (std::pow((u3 - u1).x() - a * (x3 - x1).x() - b * (x3 - x1).y(), 2) +
                             std::pow((u3 - u1).y() + b * (x3 - x1).x() - a * (x3 - x1).y(), 2)) +
                        lambda * std::pow((a * a + b * b - 1), 2);
                    if (energy < min_energy) {
                        min_energy = energy;
                        min_a = a;
                        min_b = b;
                    }
                }
            }
            L[face_handle.idx()](0, 0) = min_a;
            L[face_handle.idx()](0, 1) = min_b;
            L[face_handle.idx()](1, 0) = -min_b;
            L[face_handle.idx()](1, 1) = min_a;
        }
        // Step 3-Global Phase: With Lt fixed, update parameter coordinates(u) by solving a
        // pre-factored global sparse linear system.

        // First set b.
        b = Eigen::MatrixXf::Zero(n, 2);
        for (auto face_handle : halfedge_mesh->faces()) {
            const auto halfedge_handle = face_handle.halfedges().begin().handle();
            const auto x1 = x[face_handle.idx()][0].second;
            const auto x2 = x[face_handle.idx()][1].second;
            const auto x3 = x[face_handle.idx()][2].second;
            const auto u1 = u[halfedge_handle.to().idx()];
            const auto u2 = u[halfedge_handle.next().to().idx()];
            const auto u3 = u[halfedge_handle.next().next().to().idx()];
            b.row(halfedge_handle.to().idx()) +=
                cot_theta[halfedge_handle.next().idx()] * L[face_handle.idx()] * (x1 - x2);
            b.row(halfedge_handle.next().to().idx()) +=
                cot_theta[halfedge_handle.next().next().idx()] * L[face_handle.idx()] * (x2 - x3);
            b.row(halfedge_handle.next().next().to().idx()) +=
                cot_theta[halfedge_handle.idx()] * L[face_handle.idx()] * (x3 - x1);
            b.row(halfedge_handle.next().to().idx()) +=
                cot_theta[halfedge_handle.next().idx()] * L[face_handle.idx()] * (x2 - x1);
            b.row(halfedge_handle.next().next().to().idx()) +=
                cot_theta[halfedge_handle.next().next().idx()] * L[face_handle.idx()] * (x3 - x2);
            b.row(halfedge_handle.to().idx()) +=
                cot_theta[halfedge_handle.idx()] * L[face_handle.idx()] * (x1 - x3);
        }
        // for (int i = 0; i < 3; i++) {
        //     if (x[fixed_face_idx][i].first == fixed1) {
        //         b.row(fixed1) = Eigen::Vector2f(
        //             x[fixed_face_idx][i].second.x(), x[fixed_face_idx][i].second.y());
        //     }
        //     if (x[fixed_face_idx][i].first == fixed2) {
        //         b.row(fixed2) = Eigen::Vector2f(
        //             x[fixed_face_idx][i].second.x(), x[fixed_face_idx][i].second.y());
        //     }
        // }
        b.row(fixed1) = Eigen::Vector2f(0, 0);
        b.row(fixed2) = Eigen::Vector2f(u[fixed2].x(), u[fixed2].y());
        // Solve the linear system.
        Eigen::MatrixXf u_new = solver.solve(b);
        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Solve failed.");
        }
        // Update u.
        for (auto vertex_handle : halfedge_mesh->vertices()) {
            u[vertex_handle.idx()] = u_new.row(vertex_handle.idx());
        }
    }

    // Step 4-Iteration: Repeat Steps 2 and 3 to refine parameterization.
    // Implemented by the while loop.
    set_output(n, u, halfedge_mesh, params);
}

static void node_register()
{
    static NodeTypeInfo ntype_arap, ntype_asap, ntype_hybrid;

    strcpy(ntype_arap.ui_name, "ARAP Parameterization");
    strcpy_s(ntype_arap.id_name, "geom_arap");
    strcpy(ntype_asap.ui_name, "ASAP Parameterization");
    strcpy_s(ntype_asap.id_name, "geom_asap");
    strcpy(ntype_hybrid.ui_name, "ASAP-ARAP Hybrid");
    strcpy_s(ntype_hybrid.id_name, "geom_hybrid");

    geo_node_type_base(&ntype_arap);
    ntype_arap.node_execute = node_arap_exec;
    ntype_arap.declare = node_arap_declare;
    nodeRegisterType(&ntype_arap);

    geo_node_type_base(&ntype_asap);
    ntype_asap.node_execute = node_asap_exec;
    ntype_asap.declare = node_asap_declare;
    nodeRegisterType(&ntype_asap);

    geo_node_type_base(&ntype_hybrid);
    ntype_hybrid.node_execute = node_hybrid_exec;
    ntype_hybrid.declare = node_hybrid_declare;
    nodeRegisterType(&ntype_hybrid);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_arap
