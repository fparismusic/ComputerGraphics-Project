#version 450
#extension GL_ARB_separate_shader_objects : enable // Enable separate shader objects

layout(binding = 0) uniform UniformBufferObject {
    float visible;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUV;

void main() {
    // clip‐space (intermediate phase before NDC and screen-space)
    // 0.5 sets the HUD at half depth, centering it in front of the camera
    // 1.0 keeps correct perspective division (no distortion)
    // if ubo.visible == 0, the HUD is effectively hidden (not rendered)
    gl_Position = vec4(inPosition * ubo.visible, 0.5f, 1.0f); // if visible=0 the HUD will not be rendered
    outUV = inUV;
}