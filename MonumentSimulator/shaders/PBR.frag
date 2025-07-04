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

// Compute the TBN (Tangent, Bitangent, Normal) matrix to transform from tangent space to world space
mat3 computeTBN(vec3 N, vec3 T, float tangentW) {
    // Calculate bitangent as cross product of normal and tangent, scaled by handedness (w)
    vec3 B = cross(N, T) * tangentW;
    // Construct orthonormal basis matrix
    return mat3(normalize(T), normalize(B), normalize(N));
}

// Get the normal from the normal map texture and transform it to world space using TBN
vec3 getNormalFromMap(mat3 TBN) {
    // Sample normal from normal map (values in [0,1]), convert to [-1,1] range
    vec3 tangentNormal = texture(normalSampler, fragUV).xyz * 2.0 - 1.0;
    // Transform normal from tangent space to world space and normalize
    return normalize(TBN * tangentNormal);
}

// Fresnel-Schlick approximation for reflectance depending on viewing angle
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    // F0 is reflectance at normal incidence, cosTheta is angle between view and half-vector
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX / Trowbridge-Reitz Normal Distribution Function for microfacet distribution
// Models how rough or smooth the surface microfacets are oriented relative to the halfway vector H
// (Real surfaces are not perfectly smooth)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0001f);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// Schlick-GGX Geometry function for single direction (view or light)
// Calculates how much light reflects vs refracts at different viewing angles using the Fresnel effect
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Combined geometry term using Smith's method (both view and light)
// Accounts for shadowing and masking effects caused by microfacets blocking each other
// (Some microfacets can be blocked from the light or view by others)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0001f), roughness) *
    GeometrySchlickGGX(max(dot(N, L), 0.0001f), roughness);
}

void main() {
    vec3 albedo = texture(baseColorSampler, fragUV).rgb;        // Sample base color (albedo)
    vec4 mr = texture(mrSampler, fragUV);                       // Sample metallic and roughness packed into mrSampler
    float metallic  = mr.r;
    float roughness = mr.g;
    vec3 emissive = texture(emissiveSampler, fragUV).rgb;      // Sample emissive color

    // For accuracy we normalize
    vec3 N = normalize(fragNormal);
    vec3 T = normalize(fragTangent.xyz);
    float w = fragTangent.w;
    mat3 TBN = computeTBN(N, T, w); // Building TBN matrix for normal mapping
    vec3 Nmap = getNormalFromMap(TBN); // Computing normal from normal map in world space

    vec3 V = normalize(gubo.cameraPos - fragPos);               // View vector
    vec3 L = normalize(gubo.lightDir);                          // Light direction vector
    vec3 H = normalize(V + L);                                  // Half-vector between view and light
    vec3 radiance = gubo.lightColor.rgb * gubo.lightIntensity;  // Light radiance

    // Calculate base reflectance at normal incidence (F0)
    // For dielectrics 0.04, for metals albedo color (metallic blend)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = DistributionGGX(Nmap, H, roughness);
    float G   = GeometrySmith(Nmap, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // Calculate specular BRDF term
    vec3 specular = (NDF * G * F) /
                    max(4.0 * max(dot(Nmap, V), 0.0001f) * max(dot(Nmap, L), 0.0), 0.0001f);

    vec3 kS = F;                // kS = specular reflection amount
    vec3 kD = vec3(1.0) - kS;   // kD = diffuse reflection amount
    kD *= 1.0 - metallic; // Metals have no diffuse component, so scale diffuse by (1 - metallic)

    // Lambertian diffuse component scaled by PI to normalize
    float NdotL = max(dot(Nmap, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // Ambient term to simulate indirect lighting
    vec3 ambient = vec3(0.015f) * albedo;

    vec3 color = ambient + Lo + emissive;

    outColor = vec4(color, 1.0);
}