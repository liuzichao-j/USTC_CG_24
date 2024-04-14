#version 430 core

// Left empty. This is optional. For implemetation, you can find many references from nearby shaders. You might need random number generators (RNG) to distribute points in a (Hemi)sphere. You can ask AI for both of them (RNG and sampling in a sphere) or try to find some resources online. Later I will add some links to the document about this.

// Screen Space Ambient Occlusion (SSAO) shader

// Input
uniform vec2 iResolution; // Screen resolution

uniform sampler2D colorSampler; // Color map
uniform sampler2D positionSampler; // Position map
uniform sampler2D depthSampler; // Depth map
uniform sampler2D normalSampler; // Normal map

uniform mat4 projection; // Projection matrix
uniform mat4 view; // View matrix

layout(location = 0) out vec4 Color;

// Random number generator
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233)) * 43758.5453));
}

void main() {
    // SSAO parameters
    const int numSamples = 64; // Number of samples for SSAO
    const float radius = 1.0; // SSAO sampling radius

    // Sample kernel for SSAO
    vec3 sampleKernel[numSamples];
    for (int i = 0; i < numSamples; i++) {
        // Generate random sample in hemisphere
        vec3 samplevec = normalize(vec3(
            rand(vec2(gl_FragCoord.xy * i)),
            rand(vec2(gl_FragCoord.xy * i + 1.0)),
            rand(vec2(gl_FragCoord.xy * i + 2.0))
        ));
        // Scale sample to be within the sampling radius
        samplevec *= radius;
        sampleKernel[i] = samplevec;
    }

    // Get the position, depth, and normal at the current fragment
    vec2 texCoord = gl_FragCoord.xy / iResolution;
    vec3 fragPos = texture(positionSampler, texCoord).xyz;
    float fragDepth = texture(depthSampler, texCoord).r;
    vec3 fragNormal = texture(normalSampler, texCoord).xyz;

    // Calculate ambient occlusion
    float occlusion = 0.0;
    for (int i = 0; i < numSamples; i++) {
        // Sample position in the hemisphere around the fragment
        vec3 samplePos = fragPos + sampleKernel[i];

        // Transform sample position to screen space
        vec4 samplePosScreen = vec4(samplePos, 1.0);
        samplePosScreen = projection * view * samplePosScreen;
        samplePosScreen.xyz /= samplePosScreen.w;
        samplePosScreen.xy = samplePosScreen.xy * 0.5 + 0.5;

        // Get the depth at the sample position
        float sampleDepth = texture(depthSampler, samplePosScreen.xy).r;

        // Calculate occlusion factor based on depth difference
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragDepth - sampleDepth));
        // Use Hermite interpolation for smoother results
        occlusion += (sampleDepth >= samplePos.z + 0.025 ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / float(numSamples));

    // Apply ambient occlusion to the fragment color
    vec3 fragColor = texture(colorSampler, texCoord).rgb;
    fragColor *= occlusion;
    // Color = vec4(occlusion, 0, 0, 1.0);
    Color = vec4(fragColor, 1.0);
}