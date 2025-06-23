#version 450
#extension GL_ARB_separate_shader_objects : enable

// Descrittori
// set 0, binding 0: Global UBO (cameraPos, lightDir, lightColor, ecc.)
// set 1, binding 1: sampler2D della baseColor della montagna

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
        vec3 cameraPos;
        vec3 lightDir;
        vec3 lightColor;
        float lightIntensity;
} gubo;

layout(set = 1, binding = 1) uniform sampler2D texBaseColor;
layout(set = 1, binding = 2) uniform sampler2D texExtra;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    // Albedo
    vec3 albedo = texture(texBaseColor, fragUV).rgb;

    // Normal map in world‑space [-1,1] -> mountain normal
    vec3 n_world = normalize(texture(texExtra, fragUV).xyz * 2.0 - 1.0);

    // L=light V=view
    vec3 L = normalize(gubo.lightDir);
    vec3 V = normalize(gubo.cameraPos - fragPos);

    // Diffuse
    float NdotL = max(dot(n_world, L), 0.0);

    // Blinn-Phong specular
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(n_world, H), 0.0), 32.0);

    // Components
    vec3 ambient  = albedo; // not so much correct, but works for now
    vec3 diffuse = NdotL * albedo * gubo.lightColor * gubo.lightIntensity;
    vec3 specular = spec * gubo.lightColor * gubo.lightIntensity;

    vec3 col = ambient + diffuse + specular;
    outColor = vec4(col, 1.0);
}