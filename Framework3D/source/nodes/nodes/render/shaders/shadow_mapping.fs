#version 430 core

uniform mat4 light_view;
uniform mat4 light_projection;
in vec3 vertexPosition;
layout(location = 0) out float shadow_map0;

void main() {
    vec4 clipPos = light_projection * light_view * (vec4(vertexPosition, 1.0));
    // vertexPosition is in world space
    // clipPos is in light clip space
    shadow_map0 = (clipPos.z / clipPos.w);
    // Calculate the depth value in the light clip space. 
}