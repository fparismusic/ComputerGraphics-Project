#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;
    float time;  // Include time in the same uniform block
} ubo;

layout(binding = 1) uniform sampler2D skybox;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {

    outColor = texture(skybox, fragTexCoord);
}
