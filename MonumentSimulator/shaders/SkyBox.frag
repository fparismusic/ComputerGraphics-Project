#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    mat4 view;
    mat4 proj;

    vec3 cameraPos;
    float time;

    vec3 lightDir;
    vec3 lightColor;
    float lightIntensity;
} gubo;

layout(set = 1, binding = 1) uniform sampler2D skybox;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {
    // We sample the sky texture
    float v = clamp(fragTexCoord.y, 0.0, 1.0);
    vec3 srgbColor = texture(skybox, vec2(fragTexCoord.x, v)).rgb;

    // We apply a Gamma correction: convert from sRGB to linear space
    // (linearize the color for correct lighting operations)
    float gamma = 2.2; //standard gamma value
    vec3 linearColor = pow(srgbColor, vec3(gamma));

    // Computing a fog factor that increases near the horizon
    float fogStart = 0.4;   // v-coordinate where fog starts
    float fogEnd   = 0.7;   // v-coordinate where fog is fully horizonColor
    float fogFactor = smoothstep(fogStart, fogEnd, fragTexCoord.y);

    // Define a horizon tint using scene light
    vec3 horizonColor = gubo.lightColor * gubo.lightIntensity;
    // Blend between sky and horizon tint
    linearColor = mix(linearColor, horizonColor, fogFactor);

    // Time-based subtle brightness modulation
    linearColor *= 1.0 + 0.03 * sin(gubo.time);

    // Converting back to sRGB space
    vec3 finalColor = pow(linearColor, vec3(1.0 / gamma));

    // --- Output ---
    outColor = vec4(finalColor, 1.0);
}
