#version 430

// Fragment shader
layout(location = 0) out vec3 position;
layout(location = 1) out float depth;
layout(location = 2) out vec2 texcoords;
layout(location = 3) out vec3 diffuseColor;
layout(location = 4) out vec2 metallicRoughness;
layout(location = 5) out vec3 normal;
// Data layout: (x, y, z, depth, u, v, r, g, b, metallic, roughness, nx, ny, nz) for each fragment

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vTexcoord;
uniform mat4 projection;
uniform mat4 view;

uniform sampler2D diffuseColorSampler;

// This only works for current scenes provided by the TAs 
// because the scenes we provide is transformed from gltf
uniform sampler2D normalMapSampler;
uniform sampler2D metallicRoughnessSampler;

void main() {
    position = vertexPosition;
    vec4 clipPos = projection * view * (vec4(position, 1.0));
    depth = clipPos.z / clipPos.w;
    texcoords = vTexcoord;

    diffuseColor = texture2D(diffuseColorSampler, vTexcoord).xyz;
    metallicRoughness = texture2D(metallicRoughnessSampler, vTexcoord).zy;

    vec3 normalmap_value = texture2D(normalMapSampler, vTexcoord).xyz;
    normal = normalize(vertexNormal);

    // HW6_TODO: Apply normal map here. Use normal textures to modify vertex normals.

    // Calculate tangent and bitangent
    vec3 edge1 = dFdx(vertexPosition);
    // Get partial position / partial x
    vec3 edge2 = dFdy(vertexPosition);
    // Get partial position / partial y
    vec2 deltaUV1 = dFdx(vTexcoord);
    // Get partial uv / partial x
    vec2 deltaUV2 = dFdy(vTexcoord);
    // Get partial uv / partial y

    // float f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    // 1 / |Jacobian| means how to make area smaller. 

    vec3 tangent = edge1 * deltaUV2.y - edge2 * deltaUV1.y;
    // tangent vector should be partial position / partial u. Also J(position, v) / J(u, v). 
    // vec3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);
    // bitangent vector should be partial position / partial v. Also J(position, u) / J(v, u). 
    
    // Robust tangent and bitangent evaluation
    if(length(tangent) < 1E-7) {
        vec3 bitangent = -edge1 * deltaUV2.x + edge2 * deltaUV1.x;
        tangent = normalize(cross(bitangent, normal));
    }

    tangent = normalize(tangent - dot(tangent, normal) * normal);
    // bitangent = normalize(bitangent - dot(bitangent, normal) * normal);
    vec3 bitangent = normalize(cross(tangent,normal));

    mat3 TBN = mat3(tangent, bitangent, normal);
    normal = normalize(TBN * normalmap_value);
    // Normal mapping
}