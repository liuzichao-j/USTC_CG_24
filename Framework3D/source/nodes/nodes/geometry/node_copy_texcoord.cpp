#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"
#include "utils/util_openmesh_bind.h"

namespace USTC_CG::node_copy_texcoord {

static void node_copy_texcoord_declare(NodeDeclarationBuilder& b)
{
    // Input-1: Original 3D mesh without texture coordinates
    b.add_input<decl::Geometry>("Input");
    // Input-2: Minimal surface with texture coordinates
    b.add_input<decl::Geometry>("Minimal");

    // Output-1: Original 3D mesh with texture coordinates
    b.add_output<decl::Geometry>("Output");
}

static void node_copy_texcoord_exec(ExeParams params)
{
    // Get the input from params
    auto input = params.get_input<GOperandBase>("Input");
    auto minimal = params.get_input<GOperandBase>("Minimal");

    // Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>() || !minimal.get_component<MeshComponent>()) {
        throw std::runtime_error("Minimal Surface: Need Geometry Input.");
    }

    // Get the original mesh and the texcoord array to copy
    auto halfedge_mesh = operand_to_openmesh(&input);
    auto& texcoord_old = minimal.get_component<MeshComponent>()->texcoordsArray;
    auto operand_base = openmesh_to_operand(halfedge_mesh.get());
    auto& texcoord_new = operand_base->get_component<MeshComponent>()->texcoordsArray;
    if (input.get_component<MeshComponent>()->vertices.size() != texcoord_old.size()) {
        throw std::runtime_error("Number of vertices Mismatch!");
    }

    // Copy the texcoord array
    texcoord_new.clear();
    texcoord_new.assign(texcoord_old.begin(), texcoord_old.end());

    // Set the output of the nodes
    params.set_output("Output", std::move(*operand_base));
}

static void node_register()
{
    static NodeTypeInfo ntype;

    strcpy(ntype.ui_name, "Copy Texcoord");
    strcpy_s(ntype.id_name, "geom_copy_texcoord");

    geo_node_type_base(&ntype);
    ntype.node_execute = node_copy_texcoord_exec;
    ntype.declare = node_copy_texcoord_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_copy_texcoord
