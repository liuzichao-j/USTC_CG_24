#include <Eigen/Sparse>

#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"
#include "utils/util_openmesh_bind.h"

/*
** @brief HW4_TutteParameterization
**
** This file presents the basic framework of a "node", which processes inputs
** received from the left and outputs specific variables for downstream nodes to
** use.
** - In the first function, node_declare, you can set up the node's input and
** output variables.
** - The second function, node_exec is the execution part of the node, where we
** need to implement the node's functionality.
** - The third function generates the node's registration information, which
** eventually allows placing this node in the GUI interface.
**
** Your task is to fill in the required logic at the specified locations
** within this template, especially in node_exec.
*/

namespace USTC_CG::node_min_surf {
static void node_min_surf_declare(NodeDeclarationBuilder& b)
{
    // Input-1: Original 3D mesh with boundary
    b.add_input<decl::Geometry>("Input");
    b.add_input<decl::Geometry>("Boundary Mapping Input");
    b.add_input<decl::Int>("WeightType").min(0).max(3).default_val(0);
    // For WeightType: 0 for the original input, 1 for uniform weights, 2 for cotangent weights, 3
    // for Floater weights (Shape-preserving)

    /*
    ** NOTE: You can add more inputs or outputs if necessary. For example, in some cases,
    ** additional information (e.g. other mesh geometry, other parameters) is required to perform
    ** the computation.
    **
    ** Be sure that the input/outputs do not share the same name. You can add one geometry as
    **
    **                b.add_input<decl::Geometry>("Input");
    **
    ** Or maybe you need a value buffer like:
    **
    **                b.add_input<decl::Float1Buffer>("Weights");
    */

    // Output-1: Minimal surface with fixed boundary
    b.add_output<decl::Geometry>("Output");
}

static void node_min_surf_exec(ExeParams params)
{
    // Get the input from params
    auto input = params.get_input<GOperandBase>("Input");
    auto input2 = params.get_input<GOperandBase>("Boundary Mapping Input");
    auto weighttype = params.get_input<int>("WeightType");

    // (TO BE UPDATED) Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Minimal Surface: Need Geometry Input.");
    }
    if (!input2.get_component<MeshComponent>()) {
        // No boundary mapping input, use the original input
        input2 = std::move(input);
    }

    if (weighttype == 0) {
        auto& output = input;
        params.set_output("Output", std::move(output));
        return;
    }

    /* ----------------------------- Preprocess -------------------------------
    ** Create a halfedge structure (using OpenMesh) for the input mesh. The
    ** half-edge data structure is a widely used data structure in geometric
    ** processing, offering convenient operations for traversing and modifying
    ** mesh elements.
    */
    auto halfedge_mesh = operand_to_openmesh(&input);
    auto halfedge_mesh2 = operand_to_openmesh(&input2);
    // Get fixed positions
    std::vector<OpenMesh::Vec3f> fixed_positions(halfedge_mesh2->n_vertices());
    for (const auto& vertex_handle : halfedge_mesh2->vertices()) {
        if (vertex_handle.is_boundary()) {
            fixed_positions[vertex_handle.idx()] = halfedge_mesh2->point(vertex_handle);
        }
    }

    /* ---------------- [HW4_TODO] TASK 1: Minimal Surface --------------------
    ** In this task, you are required to generate a 'minimal surface' mesh with
    ** the boundary of the input mesh as its boundary.
    **
    ** Specifically, the positions of the boundary vertices of the input mesh
    ** should be fixed. By solving a global Laplace equation on the mesh,
    ** recalculate the coordinates of the vertices inside the mesh to achieve
    ** the minimal surface configuration.
    **
    ** (Recall the Poisson equation with Dirichlet Boundary Condition in HW3)
    */
    int n = halfedge_mesh->n_vertices();
    // For easy solving, we delete the fixed boundary vertices by giving another index
    std::vector<int> point_to_id(n, -1);
    int idcnt = 0;
    for (const auto& vertex_handle : halfedge_mesh->vertices()) {
        // init id for vertices not on boundary
        if (!vertex_handle.is_boundary()) {
            const int& index = vertex_handle.idx();
            point_to_id[index] = idcnt;
            idcnt++;
        }
    }
    // We have equation for minimal surface: Sum_{v_j \in N(i)} w_j*v_j - sum_w * v_i = 0
    // Transform it to Ax = b, where x = [v_1, v_2, ..., v_n].
    Eigen::SparseMatrix<float> A = Eigen::SparseMatrix<float>(idcnt, idcnt);
    std::vector<Eigen::Triplet<float>> triplets;
    Eigen::SparseLU<Eigen::SparseMatrix<float>> solver;
    Eigen::MatrixXf b = Eigen::MatrixXf::Zero(idcnt, 3);
    for (const auto& vertex_handle : halfedge_mesh->vertices()) {
        if (vertex_handle.is_boundary()) {
            // The formula only works for internal vertices
            continue;
        }

        // calculate the weight sum
        float sum_w = 0;
        const auto& position = halfedge_mesh->point(vertex_handle);
        const int& index = vertex_handle.idx();
        const int& id = point_to_id[index];

        if (weighttype != 3) {
            for (const auto& halfedge_handle : vertex_handle.outgoing_halfedges()) {
                // get one neighbor vertex
                const auto& v = halfedge_handle.to();
                float w = 0;
                // calculate different weights
                if (weighttype == 1) {
                    // Uniform weights
                    w = 1;
                }
                else if (weighttype == 2) {
                    // Cotangent weights
                    const auto& v1 = halfedge_handle.next().to();
                    const auto& v2 = halfedge_handle.opp().next().to();
                    const auto vec10 = halfedge_mesh->point(v1) - position;
                    const auto vec11 = halfedge_mesh->point(v1) - halfedge_mesh->point(v);
                    float cos1 = vec10.dot(vec11) / (vec10.norm() * vec11.norm());
                    float theta1 = acosf(cos1);
                    float cot1 = 1 / tan(theta1);
                    const auto vec20 = halfedge_mesh->point(v2) - position;
                    const auto vec21 = halfedge_mesh->point(v2) - halfedge_mesh->point(v);
                    float cos2 = vec20.dot(vec21) / (vec20.norm() * vec21.norm());
                    float theta2 = acosf(cos2);
                    float cot2 = 1 / tan(theta2);
                    w = cot1 + cot2;
                }
                else {
                    throw std::exception("Unknown weight type");
                }
                sum_w += w;

                // For boundary vertices, fix their positions
                if (v.is_boundary()) {
                    // Use fixed positions instead of the original positions
                    b(id, 0) += w * fixed_positions[v.idx()][0];
                    b(id, 1) += w * fixed_positions[v.idx()][1];
                    b(id, 2) += w * fixed_positions[v.idx()][2];
                }
                else {
                    triplets.push_back(Eigen::Triplet<float>(id, point_to_id[v.idx()], -w));
                }
            }
        }
        else {
            // throw std::runtime_error("Floater weights not implemented");
            // return;

            // Floater weights
            // Step 1: Flatten the 1-ring neighborhood of the vertex
            std::vector<OpenMesh::Vec3f> old_positions;
            std::vector<double> angles;
            std::vector<std::vector<double>> new_positions;
            auto start_edge = vertex_handle.outgoing_halfedges().begin().handle();
            OpenMesh::SmartHalfedgeHandle edge = start_edge;
            do {
                const auto& v = edge.to();
                old_positions.push_back(halfedge_mesh->point(v));
                edge = edge.prev().opp();
            } while (edge != start_edge);
            for (int i = 0; i < old_positions.size(); i++) {
                const auto& v1 = old_positions[i];
                const auto& v2 = old_positions[(i + 1) % old_positions.size()];
                const auto vec1 = v1 - position;
                const auto vec2 = v2 - position;
                // printf(
                //     "vec1: %f %f %f vec2: %f %f %f\n",
                //     vec1[0],
                //     vec1[1],
                //     vec1[2],
                //     vec2[0],
                //     vec2[1],
                //     vec2[2]);
                double cos1 = vec1.dot(vec2) / (vec1.norm() * vec2.norm());
                double theta1 = acos(cos1);
                angles.push_back(theta1);
            }
            double sum_angle = 0.0f;
            for (const auto& angle : angles) {
                sum_angle += angle;
            }
            for (int i = 0; i < angles.size(); i++) {
                angles[i] *= 2 * acos(-1) / sum_angle;
            }
            // We want to have p = 0, p_1 = (r, 0) and sum of angles = 2pi
            double nowangle = 0.0f;
            for (int i = 0; i < old_positions.size(); i++) {
                const auto& v = old_positions[i];
                const auto vec = v - position;
                double norm = vec.norm();
                std::vector<double> new_position = { norm * cosf(nowangle),
                                                     norm * sinf(nowangle),
                                                     0.0f };
                new_positions.push_back(new_position);
                nowangle += angles[i];
            }
            // Step 2: For each point p_l, found the opposite point p_r(l) and p_r(l)+1, then
            // calculate mu_kl.
            int n = old_positions.size();
            std::vector<std::vector<double>> mu(n, std::vector<double>(n, 0.0f));
            for (int i = 0; i < n; i++) {
                // Line p - p_l
                double k1;
                if (fabs(new_positions[i][0]) < 1e-15) {
                    k1 = new_positions[i][1] / 1e-15;
                }
                else {
                    k1 = new_positions[i][1] / new_positions[i][0];
                }
                // Line p_r - p_r+1
                int r = -1;
                for (int j = 1; j < n; j++) {
                    int nowr = (i + j) % n, nowr1 = (i + j + 1) % n;
                    if (fabs(new_positions[nowr][1] - k1 * new_positions[nowr][0]) < 1e-15) {
                        // target point p_r
                        r = nowr;
                        break;
                    }
                    double k2;
                    if (fabs(new_positions[nowr1][0] - new_positions[nowr][0]) < 1e-15) {
                        k2 = (new_positions[nowr1][1] - new_positions[nowr][1]) / 1e-15;
                    }
                    else {
                        k2 = (new_positions[nowr1][1] - new_positions[nowr][1]) /
                             (new_positions[nowr1][0] - new_positions[nowr][0]);
                    }
                    double b2 = new_positions[nowr][1] - k2 * new_positions[nowr][0];
                    double x;
                    if (fabs(k1 - k2) < 1e-15) {
                        x = b2 / 1e-15;
                    }
                    else {
                        x = b2 / (k1 - k2);
                    }
                    if ((x - new_positions[nowr1][0]) * (x - new_positions[nowr][0]) <= 0) {
                        // x is in line p_r - p_r+1
                        r = nowr;
                        break;
                    }
                }
                if (r == -1) {
                    throw std::runtime_error("Cannot find opposite point");
                }
                // Calculate mu_kl, which is related to the barycentric coordinates of triangle p_i,
                // p_r, p_r+1
                const auto& p1 = new_positions[i];
                const auto& p2 = new_positions[r];
                const auto& p3 = new_positions[(r + 1) % n];
                float area = 0.5f * fabsf(
                                        p1[0] * p2[1] + p2[0] * p3[1] + p3[0] * p1[1] -
                                        p1[1] * p2[0] - p2[1] * p3[0] - p3[1] * p1[0]);
                if (area < 1e-6f) {
                    throw std::runtime_error("Area too small");
                }
                // We have p = 0.
                float area1 = 0.5f * fabsf(p2[0] * p3[1] - p2[1] * p3[0]);
                float area2 = 0.5f * fabsf(p1[0] * p3[1] - p1[1] * p3[0]);
                float area3 = 0.5f * fabsf(p1[0] * p2[1] - p1[1] * p2[0]);
                // Now we have barcentric coordinates of p_i, p_r, p_r+1
                mu[i][i] = area1 / area;
                mu[r][i] = area2 / area;
                mu[(r + 1) % n][i] = area3 / area;
                // printf(
                //     "mu[%d][%d] = %f mu[%d][%d] = %f mu[%d][%d] = %f\n",
                //     i,
                //     i,
                //     mu[i][i],
                //     r,
                //     i,
                //     mu[r][i],
                //     (r + 1) % n,
                //     i,
                //     mu[(r + 1) % n][i]);
            }
            // Step 3: Calculate the weight of each edge
            int cnt = 0;
            edge = start_edge;
            do {
                const auto& v = edge.to();
                float w = 0;
                for (int i = 0; i < n; i++) {
                    w += mu[cnt][i];
                }
                w = w / n;
                // printf("w[%d] = %f\n", cnt, w);
                if (v.is_boundary()) {
                    b(id, 0) += w * fixed_positions[v.idx()][0];
                    b(id, 1) += w * fixed_positions[v.idx()][1];
                    b(id, 2) += w * fixed_positions[v.idx()][2];
                }
                else {
                    triplets.push_back(Eigen::Triplet<float>(id, point_to_id[v.idx()], -w));
                }
                cnt++;
                edge = edge.prev().opp();
            } while (edge != start_edge);
            sum_w = 1;
        }
        triplets.push_back(Eigen::Triplet<float>(id, id, sum_w));
    }

    /*
    ** Algorithm Pseudocode for Minimal Surface Calculation
    ** ------------------------------------------------------------------------
    ** 1. Initialize mesh with input boundary conditions.
    **    - For each boundary vertex, fix its position.
    **    - For internal vertices, initialize with initial guess if necessary.
    **
    ** 2. Construct Laplacian matrix for the mesh.
    **    - Compute weights for each edge based on the chosen weighting scheme
    **      (e.g., uniform weights for simplicity).
    **    - Assemble the global Laplacian matrix.
    **
    ** 3. Solve the Laplace equation for interior vertices.
    **    - Apply Dirichlet boundary conditions for boundary vertices.
    **    - Solve the linear system (Laplacian * X = 0) to find new positions
    **      for internal vertices.
    **
    ** 4. Update mesh geometry with new vertex positions.
    **    - Ensure the mesh respects the minimal surface configuration.
    **
    ** Note: This pseudocode outlines the general steps for calculating a
    ** minimal surface mesh given fixed boundary conditions using the Laplace
    ** equation. The specific implementation details may vary based on the mesh
    ** representation and numerical methods used.
    **
    */
    A.setFromTriplets(triplets.begin(), triplets.end());
    // Solve the linear system
    solver.compute(A);
    if (solver.info() != Eigen::Success) {
        // Decomposition failed
        throw std::exception("Decomposition failed");
    }
    Eigen::MatrixXf x = solver.solve(b);
    if (solver.info() != Eigen::Success) {
        // Solve failed
        throw std::exception("Solve failed");
    }
    // Update geometry with new vertex positions.
    for (const auto& vertex_handle : halfedge_mesh->vertices()) {
        if (!vertex_handle.is_boundary()) {
            const int& index = vertex_handle.idx();
            const int& id = point_to_id[index];
            halfedge_mesh->set_point(vertex_handle, OpenMesh::Vec3f(x(id, 0), x(id, 1), x(id, 2)));
        }
        else {
            // Use fixed positions instead of the original positions
            halfedge_mesh->set_point(vertex_handle, fixed_positions[vertex_handle.idx()]);
        }
    }

    /* ----------------------------- Postprocess ------------------------------
    ** Convert the minimal surface mesh from the halfedge structure back to
    ** GOperandBase format as the node's output.
    */
    auto operand_base = openmesh_to_operand(halfedge_mesh.get());

    // Add texture coordinates
    auto& texcoord = operand_base->get_component<MeshComponent>()->texcoordsArray;
    texcoord.clear();
    for (const auto& vertex_handle : halfedge_mesh->vertices()) {
        if (!vertex_handle.is_boundary()) {
            const int& index = vertex_handle.idx();
            const int& id = point_to_id[index];
            texcoord.push_back(pxr::GfVec2f(x(id, 0), x(id, 1)));
        }
        else {
            texcoord.push_back(pxr::GfVec2f(
                halfedge_mesh->point(vertex_handle)[0], halfedge_mesh->point(vertex_handle)[1]));
        }
    }

    /*
    pxr::VtArray<pxr::GfVec3f> colors(halfedge_mesh->n_vertices());
    for (const auto& vertex_handle : halfedge_mesh->vertices()) {
        if (halfedge_mesh->is_boundary(vertex_handle)) {
            colors[vertex_handle.idx()] = pxr::GfVec3f(1.0f, 0.0f, 0.0f);
        }
        else {
            colors[vertex_handle.idx()] = pxr::GfVec3f(0.0f, 0.0f, 0.0f);
        }
    }
    operand_base->get_component<MeshComponent>()->displayColor = colors;
    */

    /*
    float min_z = std::numeric_limits<float>::max();
    for (const auto& vertex_handle : halfedge_mesh->vertices()) {
        min_z = std::min(min_z, halfedge_mesh->point(vertex_handle)[2]);
    }
    for (const auto& vertex_handle : halfedge_mesh->vertices()) {
        halfedge_mesh->point(vertex_handle)[2] -= min_z;
    }
     */

    // Set the output of the nodes
    params.set_output("Output", std::move(*operand_base));
}

static void node_register()
{
    static NodeTypeInfo ntype;

    strcpy(ntype.ui_name, "Minimal Surface");
    strcpy_s(ntype.id_name, "geom_min_surf");

    geo_node_type_base(&ntype);
    ntype.node_execute = node_min_surf_exec;
    ntype.declare = node_min_surf_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_min_surf
