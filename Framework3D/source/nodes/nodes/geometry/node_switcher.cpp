#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"

namespace USTC_CG::node_switcher {

static void node_switcher_declare(NodeDeclarationBuilder& b)
{
    // Input: 3D mesh and a boolean switch
    b.add_input<decl::Geometry>("Input");
    b.add_input<decl::Int>("Switch").min(0).max(1).default_val(0);

    // Output: 3D mesh
    b.add_output<decl::Geometry>("Output");
}

static void node_switcher_exec(ExeParams params)
{
    // Get the input from params
    auto input = params.get_input<GOperandBase>("Input");
    auto switcher = params.get_input<int>("Switch");

    // (TO BE UPDATED) Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Need Geometry Input.");
    }
    if (switcher == 0) {
        throw std::runtime_error("No Pass");
    }
    auto& output = input;
    // The same output as the input
    params.set_output("Output", std::move(output));
}

static void node_register()
{
    static NodeTypeInfo ntype;

    strcpy(ntype.ui_name, "Switcher");
    strcpy_s(ntype.id_name, "geom_switcher");

    geo_node_type_base(&ntype);
    ntype.node_execute = node_switcher_exec;
    ntype.declare = node_switcher_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_switcher
