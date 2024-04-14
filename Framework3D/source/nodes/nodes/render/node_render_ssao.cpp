#include "NODES_FILES_DIR.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "Nodes/socket_types/basic_socket_types.hpp"
#include "camera.h"
#include "light.h"
#include "pxr/imaging/hd/tokens.h"
#include "render_node_base.h"
#include "resource_allocator_instance.hpp"
#include "rich_type_buffer.hpp"
#include "utils/draw_fullscreen.h"

namespace USTC_CG::node_ssao {
static void node_declare(NodeDeclarationBuilder& b)
{
    b.add_input<decl::Texture>("Color");
    b.add_input<decl::Texture>("Position");
    b.add_input<decl::Texture>("Depth");
    b.add_input<decl::Texture>("Normal");

    // HW6: For HBAO you might need normal texture.

    b.add_input<decl::String>("Shader").default_val("shaders/ssao.fs");
    b.add_output<decl::Texture>("Color");
}

static void node_exec(ExeParams params)
{
    auto color = params.get_input<TextureHandle>("Color");
    auto position = params.get_input<TextureHandle>("Position");
    auto depth = params.get_input<TextureHandle>("Depth");
    auto normal = params.get_input<TextureHandle>("Normal");

    auto size = color->desc.size;

    unsigned int VBO, VAO;

    CreateFullScreenVAO(VAO, VBO);

    TextureDesc texture_desc;
    texture_desc.size = size;
    texture_desc.format = HdFormatFloat32Vec4;
    auto color_texture = resource_allocator.create(texture_desc);

    auto shaderPath = params.get_input<std::string>("Shader");

    ShaderDesc shader_desc;
    shader_desc.set_vertex_path(
        std::filesystem::path(RENDER_NODES_FILES_DIR) /
        std::filesystem::path("shaders/fullscreen.vs"));

    shader_desc.set_fragment_path(
        std::filesystem::path(RENDER_NODES_FILES_DIR) / std::filesystem::path(shaderPath));
    auto shader = resource_allocator.create(shader_desc);
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture->texture_id, 0);

    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    shader->shader.use();
    shader->shader.setVec2("iResolution", size);

    // HW6: Bind the textures like other passes here.
    shader->shader.setInt("colorSampler", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color->texture_id);
    // Texture unit 0: color texture.

    shader->shader.setInt("positionSampler", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, position->texture_id);
    // Texture unit 1: position texture.

    shader->shader.setInt("depthSampler", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depth->texture_id);
    // Texture unit 2: depth texture.

    shader->shader.setInt("normalSampler", 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, normal->texture_id);
    // Texture unit 3: normal texture.

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    DestroyFullScreenVAO(VAO, VBO);
    resource_allocator.destroy(shader);
    glDeleteFramebuffers(1, &framebuffer);
    params.set_output("Color", color_texture);

    auto shader_error = shader->shader.get_error();
    if (!shader_error.empty()) {
        throw std::runtime_error(shader_error);
    }
}

static void node_register()
{
    static NodeTypeInfo ntype;

    strcpy(ntype.ui_name, "SSAO");
    strcpy_s(ntype.id_name, "render_ssao");

    render_node_type_base(&ntype);
    ntype.node_execute = node_exec;
    ntype.declare = node_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_ssao
