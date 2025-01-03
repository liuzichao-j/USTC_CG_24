#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"
#include "utils/util_openmesh_bind.h"

/*
** @brief HW4_TutteParameterization
**
** This file contains two nodes whose primary function is to map the boundary of a mesh to a plain
** convex closed curve (circle of square), setting the stage for subsequent Laplacian equation
** solution and mesh parameterization tasks.
**
** Key to this node's implementation is the adept manipulation of half-edge data structures
** to identify and modify the boundary of the mesh.
**
** Task Overview:
** - The two execution functions (node_map_boundary_to_square_exec,
** node_map_boundary_to_circle_exec) require an update to accurately map the mesh boundary to a and
** circles. This entails identifying the boundary edges, evenly distributing boundary vertices along
** the square's perimeter, and ensuring the internal vertices' positions remain unchanged.
** - A focus on half-edge data structures to efficiently traverse and modify mesh boundaries.
*/

namespace USTC_CG::node_boundary_mapping {

/*
** HW4_TODO: Node to map the mesh boundary to a circle.
*/

static void node_map_boundary_to_circle_declare(NodeDeclarationBuilder& b)
{
    // Input-1: Original 3D mesh with boundary
    b.add_input<decl::Geometry>("Input");
    // b.add_input<decl::Int>("Area Scale").min(1).max(100).default_val(1);

    // Output-1: Processed 3D mesh whose boundary is mapped to a square and the interior vertices
    // remains the same
    b.add_output<decl::Geometry>("Output");
}

static void node_map_boundary_to_circle_exec(ExeParams params)
{
    // Get the input from params
    auto input = params.get_input<GOperandBase>("Input");
    // auto area_scale = params.get_input<int>("Area Scale");

    // Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Boundary Mapping: Need Geometry Input.");
    }

    /* ----------------------------- Preprocess -------------------------------
    ** Create a halfedge structure (using OpenMesh) for the input mesh. The
    ** half-edge data structure is a widely used data structure in geometric
    ** processing, offering convenient operations for traversing and modifying
    ** mesh elements.
    */
    auto halfedge_mesh = operand_to_openmesh(&input);

    // Traverse all boundary edges, get every edge's length and calculate the total length.
    OpenMesh::SmartHalfedgeHandle start_halfedge;
    std::vector<float> lens;
    for (const auto& halfedge_handle : halfedge_mesh->halfedges()) {
        if (halfedge_handle.is_boundary()) {
            start_halfedge = halfedge_handle;
            lens.push_back((halfedge_mesh->point(halfedge_handle.to()) -
                            halfedge_mesh->point(halfedge_handle.from()))
                               .norm());
            break;
        }
    }
    if (start_halfedge.is_valid()) {
        auto halfedge_handle = start_halfedge.next();
        while (halfedge_handle.idx() != start_halfedge.idx()) {
            lens.push_back((halfedge_mesh->point(halfedge_handle.to()) -
                            halfedge_mesh->point(halfedge_handle.from()))
                               .norm());
            halfedge_handle = halfedge_handle.next();
        }
    }
    float len = 0.0f;
    for (const auto& l : lens) {
        len += l;
    }

    // Get the minimal z value, to have better planar mapping
    // float min_z = 0.0f;
    // for (const auto& vertex_handle : halfedge_mesh->vertices()) {
    //     min_z = std::min(min_z, halfedge_mesh->point(vertex_handle)[2]);
    // }
    // min_z -= (float)area_scale;

    /* ----------- [HW4_TODO] TASK 2.1: Boundary Mapping (to circle) ------------
    ** In this task, you are required to map the boundary of the mesh to a circle
    ** shape while ensuring the internal vertices remain unaffected. This step is
    ** crucial for setting up the mesh for subsequent parameterization tasks.
    **
    ** Algorithm Pseudocode for Boundary Mapping to Circle
    ** ------------------------------------------------------------------------
    ** 1. Identify the boundary loop(s) of the mesh using the half-edge structure.
    **
    ** 2. Calculate the total length of the boundary loop to determine the spacing
    **    between vertices when mapped to a square.
    **
    ** 3. Sequentially assign each boundary vertex a new position along the square's
    **    perimeter, maintaining the calculated spacing to ensure proper distribution.
    **
    ** 4. Keep the interior vertices' positions unchanged during this process.
    **
    ** Note: How to distribute the points on the circle?
    **
    ** Note: It would be better to normalize the boundary to a unit circle in [0,1]x[0,1] for
    ** texture mapping.
    */

    // Traverse all boundary edges and get the place of each boundary vertex on the circle
    int cnt = 0;
    if (start_halfedge.is_valid()) {
        auto halfedge_handle = start_halfedge;
        float nowlength = 0.0f;
        do {
            // printf(
            //     "get one boundary edge from (%f, %f, %f) to (%f, %f, %f)\n",
            //     halfedge_mesh->point(halfedge_handle.from())[0],
            //     halfedge_mesh->point(halfedge_handle.from())[1],
            //     halfedge_mesh->point(halfedge_handle.from())[2],
            //     halfedge_mesh->point(halfedge_handle.to())[0],
            //     halfedge_mesh->point(halfedge_handle.to())[1],
            //     halfedge_mesh->point(halfedge_handle.to())[2]);

            nowlength += lens[cnt];
            float theta = 2 * std::acos(-1) * nowlength / len;

            // printf(
            //     "set boundary vertex %d from (%f, %f, %f) to (%f, %f, %f) on total length "
            //     "precentage %lf\n",
            //     halfedge_handle.from().idx(),
            //     halfedge_mesh->point(halfedge_handle.to())[0],
            //     halfedge_mesh->point(halfedge_handle.to())[1],
            //     halfedge_mesh->point(halfedge_handle.to())[2],
            //     (0.5f + 0.5f * std::cos(theta)),
            //     (0.5f + 0.5f * std::sin(theta)),
            //     0.0f,
            //     nowlength / len);

            halfedge_mesh->set_point(
                halfedge_handle.to(),
                OpenMesh::Vec3f(
                    (0.5f + 0.5f * std::cos(theta)), (0.5f + 0.5f * std::sin(theta)), 0.0f));

            halfedge_handle = halfedge_handle.next();
            cnt++;
        } while (halfedge_handle.idx() != start_halfedge.idx());
    }

    /* ----------------------------- Postprocess ------------------------------
    ** Convert the result mesh from the halfedge structure back to GOperandBase format as the node's
    ** output.
    */
    auto operand_base = openmesh_to_operand(halfedge_mesh.get());

    // auto& output = input;
    // output.get_component<MeshComponent>()->vertices =
    //     operand_base->get_component<MeshComponent>()->vertices;
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
    output.get_component<MeshComponent>()->displayColor = colors;
    */

    // Set the output of the nodes
    params.set_output("Output", std::move(*operand_base));
}

/*
** HW4_TODO: Node to map the mesh boundary to a square.
*/

static void node_map_boundary_to_square_declare(NodeDeclarationBuilder& b)
{
    // Input-1: Original 3D mesh with boundary
    b.add_input<decl::Geometry>("Input");

    // Output-1: Processed 3D mesh whose boundary is mapped to a square and the interior vertices
    // remains the same
    b.add_output<decl::Geometry>("Output");
}

static void node_map_boundary_to_square_exec(ExeParams params)
{
    // Get the input from params
    auto input = params.get_input<GOperandBase>("Input");

    // Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Input does not contain a mesh");
    }

    /* ----------------------------- Preprocess -------------------------------
    ** Create a halfedge structure (using OpenMesh) for the input mesh.
    */
    auto halfedge_mesh = operand_to_openmesh(&input);

    // Traverse all boundary edges, get every edge's length and calculate the total length.
    OpenMesh::SmartHalfedgeHandle start_halfedge;
    std::vector<float> lens;
    for (const auto& halfedge_handle : halfedge_mesh->halfedges()) {
        if (halfedge_handle.is_boundary()) {
            start_halfedge = halfedge_handle;
            lens.push_back((halfedge_mesh->point(halfedge_handle.to()) -
                            halfedge_mesh->point(halfedge_handle.from()))
                               .norm());
            break;
        }
    }
    if (start_halfedge.is_valid()) {
        auto halfedge_handle = start_halfedge.next();
        while (halfedge_handle.idx() != start_halfedge.idx()) {
            lens.push_back((halfedge_mesh->point(halfedge_handle.to()) -
                            halfedge_mesh->point(halfedge_handle.from()))
                               .norm());
            halfedge_handle = halfedge_handle.next();
        }
    }
    float len = 0.0f;
    for (const auto& l : lens) {
        len += l;
    }

    /* ----------- [HW4_TODO] TASK 2.2: Boundary Mapping (to square) ------------
    ** In this task, you are required to map the boundary of the mesh to a circle
    ** shape while ensuring the internal vertices remain unaffected.
    **
    ** Algorithm Pseudocode for Boundary Mapping to Square
    ** ------------------------------------------------------------------------
    ** (omitted)
    **
    ** Note: Can you perserve the 4 corners of the square after boundary mapping?
    **
    ** Note: It would be better to normalize the boundary to a unit circle in [0,1]x[0,1] for
    ** texture mapping.
    */

    
    // Find points on the four corners of the square
    int cnt = 0;
    // Store the index of the boundary edge which starts from the corner point
    int corner_cnt[4] = { -1, -1, -1, -1 };
    // Store the length from the start point to a corner point
    float corner_length[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    if (start_halfedge.is_valid()) {
        auto halfedge_handle = start_halfedge;
        float nowlength = 0.0f;
        do {
            for (int i = 0; i < 4; i++) {
                // printf(
                //     "%f %f %f %f\n",
                //     nowlength,
                //     (len / 4.0f * (float)i),
                //     (len / 4.0f * (float)(i + 1)),
                //     nowlength + lens[cnt]);
                if (nowlength <= (len / 4.0f * (float)i) &&
                    (len / 4.0f * (float)i) < nowlength + lens[cnt] &&
                    nowlength + lens[cnt] <= (len / 4.0f * (float)(i + 1))) {
                    corner_cnt[i] = cnt;
                    corner_length[i] = nowlength;
                    // printf(
                    //     "Get corner %d (from vertex %d, edge %d) at %f\n",
                    //     i,
                    //     halfedge_handle.from().idx(),
                    //     halfedge_handle.idx(),
                    //     nowlength);
                }
            }
            nowlength += lens[cnt];
            halfedge_handle = halfedge_handle.next();
            cnt++;
        } while (halfedge_handle.idx() != start_halfedge.idx());
    }
    corner_length[4] = len;
    if (corner_cnt[0] == -1 || corner_cnt[1] == -1 || corner_cnt[2] == -1 || corner_cnt[3] == -1) {
        throw std::runtime_error("Cannot find 4 corners");
    }

    // Traverse all boundary edges and get the place of each boundary vertex on the square
    cnt = 0;
    if (start_halfedge.is_valid()) {
        auto halfedge_handle = start_halfedge;
        float nowlength = 0.0f;
        do {
            nowlength += lens[cnt];
            for (int i = 0; i < 4; i++) {
                if (corner_length[i] < nowlength && nowlength <= corner_length[i + 1]) {
                    // Means the edge is on the edge between corner i and i+1
                    float x, y;
                    float ratio =
                        (nowlength - corner_length[i]) / (corner_length[i + 1] - corner_length[i]);
                    if (i == 0) {
                        // From (0, 0) to (1, 0), calculate by ratio
                        x = ratio;
                        y = 0.0f;
                    }
                    else if (i == 1) {
                        // From (1, 0) to (1, 1)
                        x = 1.0f;
                        y = ratio;
                    }
                    else if (i == 2) {
                        // From (1, 1) to (0, 1)
                        x = 1.0f - ratio;
                        y = 1.0f;
                    }
                    else if (i == 3) {
                        // From (0, 1) to (0, 0)
                        x = 0.0f;
                        y = 1.0f - ratio;
                    }
                    halfedge_mesh->set_point(halfedge_handle.to(), OpenMesh::Vec3f(x, y, 0.0f));
                }
            }
            halfedge_handle = halfedge_handle.next();
            cnt++;
        } while (halfedge_handle.idx() != start_halfedge.idx());
    }

    /* ----------------------------- Postprocess ------------------------------
    ** Convert the result mesh from the halfedge structure back to GOperandBase format as the node's
    ** output.
    */
    auto operand_base = openmesh_to_operand(halfedge_mesh.get());

    // Set the output of the nodes
    params.set_output("Output", std::move(*operand_base));
}

static void node_register()
{
    static NodeTypeInfo ntype_square, ntype_circle;

    strcpy(ntype_square.ui_name, "Map Boundary to Square");
    strcpy_s(ntype_square.id_name, "geom_map_boundary_to_square");

    geo_node_type_base(&ntype_square);
    ntype_square.node_execute = node_map_boundary_to_square_exec;
    ntype_square.declare = node_map_boundary_to_square_declare;
    nodeRegisterType(&ntype_square);

    strcpy(ntype_circle.ui_name, "Map Boundary to Circle");
    strcpy_s(ntype_circle.id_name, "geom_map_boundary_to_circle");

    geo_node_type_base(&ntype_circle);
    ntype_circle.node_execute = node_map_boundary_to_circle_exec;
    ntype_circle.declare = node_map_boundary_to_circle_declare;
    nodeRegisterType(&ntype_circle);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_boundary_mapping
