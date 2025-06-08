#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    // NOTE: every vec3 is 16 byte in std140, so we add padding
    vec3 cameraPos;          // position of the camera (world‐space)
    vec3 lightDir;           // light direction (world‐space, directed to the mesh)
    vec3 lightColor;         // light color/intensity
} gubo;

layout(set = 1 , binding = 0) uniform SkyBoxUniformBufferObject {
    mat4 mvpMat;
    float time;  // Include time in the same uniform block
} ubo;

layout(set = 1, binding = 1) uniform sampler2D skybox;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {
    outColor = texture(skybox, fragTexCoord);
}
