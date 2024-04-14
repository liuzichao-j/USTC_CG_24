#version 430 core

// Define a uniform struct for lights
struct Light {
    // The matrices are used for shadow mapping. You need to fill it according to how we are filling it when building the normal maps (node_render_shadow_mapping.cpp). 
    // Now, they are filled with identity matrix. You need to modify C++ code in node_render_deferred_lighting.cpp.
    // Position and color are filled.
    mat4 light_projection;
    mat4 light_view;
    vec3 position;
    float radius;
    vec3 color; // Just use the same diffuse and specular color.
    int shadow_map_id;
};

layout(binding = 0) buffer lightsBuffer {
Light lights[4];
};

uniform vec2 iResolution;

uniform sampler2D diffuseColorSampler;
uniform sampler2D normalMapSampler; // You should apply normal mapping in rasterize_impl.fs
uniform sampler2D metallicRoughnessSampler;
uniform sampler2DArray shadow_maps;
uniform sampler2D position;

// uniform float alpha;
uniform vec3 camPos;

uniform int light_count;

layout(location = 0) out vec4 Color;

void main() {
vec2 uv = gl_FragCoord.xy / iResolution;
// Place in screen space
// if (uv.x > 0.55 && uv.x < 0.56 && uv.y > 0.23 && uv.y < 0.25) {
//     Color = vec4(1.0, 0.0, 0.0, 1.0);
//     return;
// }

vec3 pos = texture2D(position, uv).xyz;
// Place in world space
vec3 normal = texture2D(normalMapSampler, uv).xyz;
// After normal mapping. 

vec4 metalnessRoughness = texture2D(metallicRoughnessSampler, uv);
float metal = metalnessRoughness.x;
float roughness = metalnessRoughness.y;

Color = vec4(0.0, 0.0, 0.0, 1.0);
// Color += vec4(uv, 0, 1.0);

bool enable_shadow = true;
bool enable_pcss = true;

for(int i = 0; i < light_count; i++) {

// float shadow_map_value = texture(shadow_maps, vec3(uv, lights[i].shadow_map_id)).x;
// Get the shadow map value of the light i in place uv (screen space, also the view from light i). 

// Visualization of shadow map
// Color += vec4(shadow_map_value, 0, 0, 1);

// HW6_TODO: first comment the line above ("Color +=..."). That's for quick Visualization.
// You should first do the Blinn Phong shading here. You can use roughness to modify alpha. Or you can pass in an alpha value through the uniform above.

vec3 vlight = normalize(lights[i].position - pos);
vec3 vnormal = normalize(normal);
vec3 vview = normalize(camPos - pos);
// vec3 vreflect = reflect(-vlight, vnormal);
vec3 vreflect = normalize(-vlight + 2.0 * dot(vlight, vnormal) * vnormal);
float k_diffuse = max(1 - metal * 0.8, 0.0);
float diffuse = dot(vlight, vnormal);
if (diffuse < 0.0) {
    diffuse = 0;
    // vnormal = -vnormal;
    // diffuse = -diffuse;
}
float k_specular = max(metal * 0.8, 0.0);
float specular = pow(max(dot(vview, vreflect), 0.0), (1 - roughness));
// Color += vec4(specular, 0, 0, 1.0);
// if (diffuse == 0.0) {
//     specular = 0.0;
// }
// else {
//     specular = pow(specular, (1 - roughness) * 10.0);
// }
float k_ambient = 0.1;
vec4 diffuseColor = texture(diffuseColorSampler, uv);
// if (uv.x > 0.55 && uv.x < 0.56 && uv.y > 0.23 && uv.y < 0.25) {
//     Color = vec4((k_diffuse * diffuse + k_specular * specular) * lights[i].color, 1.0) * diffuseColor;
//     return;
// }

Color += vec4(k_ambient * lights[i].color, 1.0) * diffuseColor;

if (enable_shadow == false) {
    // Calculate the color in screen space uv for light i on known world space position pos.
    Color += vec4((k_diffuse * diffuse + k_specular * specular) * lights[i].color, 1.0) * diffuseColor;
}
else {
    // After finishing Blinn Phong shading, you can do shadow mapping with the help of the provided shadow_map_value. You will need to refer to the node, node_render_shadow_mapping.cpp, for the light matrices definition. Then you need to fill the mat4 light_projection; mat4 light_view; with similar approach that we fill position and color.
    // For shadow mapping, as is discussed in the course, you should compare the value "position depth from the light's view" against the "blocking object's depth.", then you can decide whether it's shadowed.

    // Calculate the position in light clip space
    vec4 clipPos = lights[i].light_projection * lights[i].light_view * vec4(pos, 1.0);
    // Color += lights[i].light_projection * lights[i].light_view * vec4(pos, 1.0);
    // Color += vec4(clipPos.xy / clipPos.w, 0, 1.0);
    
    // Depth value in light clip space
    float depth = clipPos.z / clipPos.w;
    // Color += vec4(depth, 0, 0, 1.0);
    
    // Calculate the position in shader map space
    vec2 shadow_uv = clipPos.xy / clipPos.w * 0.5 + 0.5;
    // Color += vec4(shadow_uv, 0, 1);
    // Color += vec4(texture(shadow_maps, vec3(uv, lights[i].shadow_map_id)).x, 0, 0, 1.0);
    
    if (shadow_uv.x < 0.0 || shadow_uv.x > 1.0 || shadow_uv.y < 0.0 || shadow_uv.y > 1.0) {
        // Cannot see from the light. Don't consider shadow.
        Color += vec4((k_diffuse * diffuse + k_specular * specular) * lights[i].color, 1.0) * diffuseColor;
    }
    else {
        if (enable_pcss == false) {
            // Shadow map value
            float shadow_map_value = texture(shadow_maps, vec3(shadow_uv, lights[i].shadow_map_id)).x;
            // Color += vec4(shadow_map_value, 0, 0, 1);
            // Decide whether it's shadowed. shadow_map_value is the depth value in the light clip space uv. 
            if (depth < 1.003 * shadow_map_value || shadow_map_value < 0.1) {
                // Not shadowed
                Color += vec4((k_diffuse * diffuse + k_specular * specular) * lights[i].color, 1.0) * diffuseColor;
            }
        }
        else {
            // PCSS
            float shadow = 0.0;
            const int shadow_samples = 16;
            float shadow_radius = lights[i].radius * depth / (1.0 + depth);
            // Color += vec4(shadow_radius, 0, 0, 1.0);
            float shadow_step = 2 * shadow_radius / shadow_samples;
            float shadow_map_value = 0.0;
            for (int j = 0; j < shadow_samples; j++) {
                for (int k = 0; k < shadow_samples; k++) {
                    float x = -shadow_radius + shadow_step / 2.0 + j * shadow_step;
                    float y = -shadow_radius + shadow_step / 2.0 + k * shadow_step;
                    shadow_map_value = texture(shadow_maps, vec3(shadow_uv + vec2(x, y), lights[i].shadow_map_id)).x;
                    // Color += vec4(shadow_map_value, 0, 0, 1.0) / (shadow_samples * shadow_samples);
                    if (depth < 1.003 * shadow_map_value || shadow_map_value < 0.1) {
                        shadow += 1.0;
                    }
                    // if(i == 1 && j == 0) {
                    //     Color += vec4(shadow_uv, 0, 1.0);
                    // }
                }
            }
            // for (float x = -shadow_radius + shadow_step / 2.0; x <= shadow_radius - shadow_step / 2.0; x += shadow_step) {
            //     for (float y = -shadow_radius + shadow_step / 2.0; y <= shadow_radius - shadow_step / 2.0; y += shadow_step) {
            //         float shadow_map_value = texture(shadow_maps, vec3(shadow_uv + vec2(x, y), lights[i].shadow_map_id)).x;
            //         if (depth < 1.003 * shadow_map_value || shadow_map_value < 0.1) {
            //             shadow += 1.0;
            //         }
            //     }
            // }
            shadow /= (shadow_samples * shadow_samples);
            // Color += vec4(shadow, 0, 0, 1.0);
            Color += vec4((k_diffuse * diffuse + k_specular * specular) * lights[i].color * shadow, 1.0) * diffuseColor;
        }
    }
}

}

}