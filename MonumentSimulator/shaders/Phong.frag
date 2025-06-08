#version 450

// Descrittori
// set 0, binding 0: Global UBO (cameraPos, lightDir, lightColor, ecc.)
// set 1, binding 1: sampler2D della baseColor della montagna

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    // NOTE: every vec3 is 16 byte in std140, so we add padding
    vec3 cameraPos;          // position of the camera (world‐space)
    vec3 lightDir;           // light direction (world‐space, directed to the mesh)
    vec3 lightColor;         // light color/intensity
} gubo;

layout(set = 1, binding = 1) uniform sampler2D texBaseColor;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    // Flip verticale, se necessario:
    vec2 uv = vec2(fragUV.x, 1.0 - fragUV.y);
    vec3 albedo = texture(texBaseColor, uv).rgb;
    outColor = vec4(albedo, 1.0);
}