// #define __GNUC__
#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"

namespace USTC_CG::node_create_geom {

static void node_create_grid_declare(NodeDeclarationBuilder &b)
{
    b.add_input<decl::Int>("resolution").min(1).max(20).default_val(2);
    b.add_input<decl::Float>("size").min(1).max(20);
    b.add_output<decl::Geometry>("Geometry");
}

static void node_create_grid_exec(ExeParams params)
{
    int resolution = params.get_input<int>("resolution") + 1;
    float size = params.get_input<float>("size");
    GOperandBase operand_base;
    std::shared_ptr<MeshComponent> mesh = std::make_shared<MeshComponent>(&operand_base);
    operand_base.attach_component(mesh);

    auto &points = mesh->vertices;
    auto &texcoord = mesh->texcoordsArray;
    auto &faceVertexIndices = mesh->faceVertexIndices;
    auto &faceVertexCounts = mesh->faceVertexCounts;

    for (int i = 0; i < resolution; ++i) {
        for (int j = 0; j < resolution; ++j) {
            float y = size * static_cast<float>(i) / (resolution - 1);
            float z = size * static_cast<float>(j) / (resolution - 1);

            float u = static_cast<float>(i) / (resolution - 1);
            float v = static_cast<float>(j) / (resolution - 1);
            points.push_back(pxr::GfVec3f(0, y, z));
            texcoord.push_back(pxr::GfVec2f(u, v));
        }
    }

    for (int i = 0; i < resolution - 1; ++i) {
        for (int j = 0; j < resolution - 1; ++j) {
            faceVertexCounts.push_back(4);
            faceVertexIndices.push_back(i * resolution + j);
            faceVertexIndices.push_back(i * resolution + j + 1);
            faceVertexIndices.push_back((i + 1) * resolution + j + 1);
            faceVertexIndices.push_back((i + 1) * resolution + j);
        }
    }

    params.set_output("Geometry", std::move(operand_base));
}

static void node_create_tri_grid_declare(NodeDeclarationBuilder &b)
{
    b.add_input<decl::Int>("resolution").min(1).max(20).default_val(2);
    b.add_input<decl::Float>("size").min(1).max(20);
    b.add_output<decl::Geometry>("Geometry");
}

static void node_create_tri_grid_exec(ExeParams params)
{
    int resolution = params.get_input<int>("resolution") + 1;
    float size = params.get_input<float>("size");
    GOperandBase operand_base;
    std::shared_ptr<MeshComponent> mesh = std::make_shared<MeshComponent>(&operand_base);
    operand_base.attach_component(mesh);

    auto &points = mesh->vertices;
    auto &texcoord = mesh->texcoordsArray;
    auto &faceVertexIndices = mesh->faceVertexIndices;
    auto &faceVertexCounts = mesh->faceVertexCounts;

    for (int i = 0; i < resolution; ++i) {
        for (int j = 0; j < resolution; ++j) {
            float y = size * static_cast<float>(i) / (resolution - 1);
            float z = size * static_cast<float>(j) / (resolution - 1);

            float u = static_cast<float>(i) / (resolution - 1);
            float v = static_cast<float>(j) / (resolution - 1);
            points.push_back(pxr::GfVec3f(0, y, z));
            texcoord.push_back(pxr::GfVec2f(u, v));
        }
    }

    for (int i = 0; i < resolution - 1; ++i) {
        for (int j = 0; j < resolution - 1; ++j) {
            faceVertexCounts.push_back(3);
            faceVertexIndices.push_back(i * resolution + j);
            faceVertexIndices.push_back(i * resolution + j + 1);
            faceVertexIndices.push_back((i + 1) * resolution + j);
            // printf(
            //     "triangle at %d %d %d with points (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n",
            //     i * resolution + j,
            //     i * resolution + j + 1,
            //     (i + 1) * resolution + j,
            //     points[i * resolution + j][0],
            //     points[i * resolution + j][1],
            //     points[i * resolution + j][2],
            //     points[i * resolution + j + 1][0],
            //     points[i * resolution + j + 1][1],
            //     points[i * resolution + j + 1][2],
            //     points[(i + 1) * resolution + j][0],
            //     points[(i + 1) * resolution + j][1],
            //     points[(i + 1) * resolution + j][2]);

            // Follow specific order to have all face normals pointing outwards
            faceVertexCounts.push_back(3);
            faceVertexIndices.push_back(i * resolution + j + 1);
            faceVertexIndices.push_back((i + 1) * resolution + j + 1);
            faceVertexIndices.push_back((i + 1) * resolution + j);

            // printf(
            //     "triangle at %d %d %d with points (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n",
            //     i * resolution + j + 1,
            //     (i + 1) * resolution + j,
            //     (i + 1) * resolution + j + 1,
            //     points[i * resolution + j + 1][0],
            //     points[i * resolution + j + 1][1],
            //     points[i * resolution + j + 1][2],
            //     points[(i + 1) * resolution + j + 1][0],
            //     points[(i + 1) * resolution + j + 1][1],
            //     points[(i + 1) * resolution + j + 1][2],
            //     points[(i + 1) * resolution + j][0],
            //     points[(i + 1) * resolution + j][1],
            //     points[(i + 1) * resolution + j][2]);
        }
    }

    params.set_output("Geometry", std::move(operand_base));
}

static void node_create_tri_declare(NodeDeclarationBuilder &b)
{
    b.add_input<decl::Float>("size").min(1).max(20);
    b.add_output<decl::Geometry>("Geometry");
}

static void node_create_tri_exec(ExeParams params)
{
    float size = params.get_input<float>("size");
    GOperandBase operand_base;
    std::shared_ptr<MeshComponent> mesh = std::make_shared<MeshComponent>(&operand_base);
    operand_base.attach_component(mesh);

    auto &points = mesh->vertices;
    auto &texcoord = mesh->texcoordsArray;
    auto &faceVertexIndices = mesh->faceVertexIndices;
    auto &faceVertexCounts = mesh->faceVertexCounts;

    points.push_back(pxr::GfVec3f(0, 0, 0));
    points.push_back(pxr::GfVec3f(size, 0, 0));
    points.push_back(pxr::GfVec3f(0, size, 0));
    points.push_back(pxr::GfVec3f(0, 0, size));
    texcoord.push_back(pxr::GfVec2f(0, 0));
    texcoord.push_back(pxr::GfVec2f(1, 0));
    texcoord.push_back(pxr::GfVec2f(0, 1));
    texcoord.push_back(pxr::GfVec2f(0, 0));
    faceVertexCounts.push_back(3);
    faceVertexIndices.push_back(0);
    faceVertexIndices.push_back(2);
    faceVertexIndices.push_back(1);
    faceVertexCounts.push_back(3);
    faceVertexIndices.push_back(0);
    faceVertexIndices.push_back(3);
    faceVertexIndices.push_back(2);
    faceVertexCounts.push_back(3);
    faceVertexIndices.push_back(0);
    faceVertexIndices.push_back(1);
    faceVertexIndices.push_back(3);

    params.set_output("Geometry", std::move(operand_base));
}

static void node_register()
{
#define CreateMesh(lower, Upper)                             \
    static NodeTypeInfo lower##_ntype;                       \
    strcpy_s(lower##_ntype.ui_name, "Create " #Upper);       \
    strcpy_s(lower##_ntype.id_name, "geom_create_" #lower);  \
    geo_node_type_base(&lower##_ntype);                      \
    lower##_ntype.node_execute = node_create_##lower##_exec; \
    lower##_ntype.declare = node_create_##lower##_declare;   \
    nodeRegisterType(&lower##_ntype);

    CreateMesh(grid, Grid);
    // CreateMesh(ico_sphere, Ico Sphere);
    CreateMesh(tri_grid, Triangle Grid);
    CreateMesh(tri, Triangle);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_create_geom
