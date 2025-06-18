#version 450
#extension GL_ARB_separate_shader_objects : enable

// UBO globale (set=0, binding=0)
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec3 cameraPos;
    vec3 lightDir;
    vec3 lightColor;
    float lightIntensity;
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

const float PI = 3.14159265359;

mat3 computeTBN(vec3 N, vec3 T, float tangentW) {
    vec3 B = cross(N, T) * tangentW;
    return mat3(normalize(T), normalize(B), normalize(N));
}

vec3 getNormalFromMap(mat3 TBN) {
    vec3 tangentNormal = texture(normalSampler, fragUV).xyz * 2.0 - 1.0;
    return normalize(TBN * tangentNormal);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0001f);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0001f), roughness) *
    GeometrySchlickGGX(max(dot(N, L), 0.0001f), roughness);
}

void main() {
    vec3 albedo     = texture(baseColorSampler, fragUV).rgb;
    vec4 mr = texture(mrSampler, fragUV);
    float metallic  = mr.r;
    float roughness = mr.g;
    vec3 emissive = texture(emissiveSampler, fragUV).rgb;

    vec3 N = normalize(fragNormal);
    vec3 T = normalize(fragTangent.xyz);
    float w = fragTangent.w;
    mat3 TBN = computeTBN(N, T, w);
    vec3 Nmap = getNormalFromMap(TBN);

    vec3 V = normalize(gubo.cameraPos - fragPos);
    vec3 L = normalize(gubo.lightDir);
    vec3 H = normalize(V + L);
    vec3 radiance = gubo.lightColor.rgb * gubo.lightIntensity;


    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = DistributionGGX(Nmap, H, roughness);
    float G   = GeometrySmith(Nmap, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = (NDF * G * F) /
                    max(4.0 * max(dot(Nmap, V), 0.0001f) * max(dot(Nmap, L), 0.0), 0.0001f);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(Nmap, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.015f) * albedo;

    vec3 color = ambient + Lo + emissive;

    outColor = vec4(color, 1.0);
}