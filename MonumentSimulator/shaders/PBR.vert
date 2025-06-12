#version 450
#extension GL_ARB_separate_shader_objects : enable

// UBO (set=1, binding=0)
layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;   // proj * view * model
    mat4 mMat;     // model matrix
    mat4 nMat;     // inverse-transpose(model)
} ubo;

layout(location = 0) in vec3 inPosition;  // model position
layout(location = 1) in vec2 inUV;        // coords UV
layout(location = 2) in vec3 inNormal;    // nornal model
layout(location = 3) in vec4 inTangent;   // model tangent (w = bitangent-sign)

layout(location = 0) out vec3 fragPos;      // posizione mondo
layout(location = 1) out vec2 fragUV;       // UV
layout(location = 2) out vec3 fragNormal;   // normale mondo
layout(location = 3) out vec4 fragTangent;  // tangente mondo

void main() {
    // We take the position in world-space
    fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;

    // We take the normal in world-space
    mat3 Nmat = mat3(ubo.nMat);
    fragNormal = normalize(Nmat* inNormal);
    fragTangent = vec4(normalize(Nmat * inTangent.xyz), inTangent.w);

    // UV coordinates
    fragUV = inUV;

    // Clip‐space
    gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
}