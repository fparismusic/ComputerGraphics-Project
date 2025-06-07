#version 450

// Descriptors
// set 0, binding 0: Global UBO (cameraPos, lightDir, lightColor, ecc.)
// set 1, binding 0: UBO specific of the mesh/model (mvpMat, model, normalMatrix)

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;         // proj * view * model
    mat4 mMat;          // model matrix
    mat4 nMat;         // (inverse transpose of model’s 3×3)
} ubo;

layout(location = 0) in vec3 inPosition;  // position (model‐space)
layout(location = 1) in vec2 inUV;        // coordinates UV
layout(location = 2) in vec3 inNormal;    // normal (model‐space)

layout(location = 0) out vec3 fragPos;     // position world‐space
layout(location = 1) out vec2 fragUV;      // UV coordinates
layout(location = 2) out vec3 fragNormal;  // normal world‐space

void main() {
    // We take the position in world-space
    fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;

    // We take the normal in world-space
    fragNormal = normalize(mat3(ubo.nMat) * inNormal);

    // Pass the UV coordinates to the fragment shader
    fragUV = inUV;

    // Calculate the final position in clip space
    gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
}