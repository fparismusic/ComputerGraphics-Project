#version 450

// UBO globale (set=0, binding=0)
layout(set = 0, binding = 0) uniform GlobalUBO {
    vec3 cameraPos;
    vec3 lightDir;
    vec3 lightColor;
} gubo;

layout(set = 1, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 1, binding = 2) uniform sampler2D mrSampler; // metallic-roughness
layout(set = 1, binding = 3) uniform sampler2D emissiveSampler;
layout(set = 1, binding = 4) uniform sampler2D normalSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = texture(baseColorSampler, fragUV).rgb;
    outColor = vec4(albedo, 1.0);
}