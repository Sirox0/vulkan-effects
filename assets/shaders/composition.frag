#version 450

#extension GL_EXT_samplerless_texture_functions : require

layout(constant_id = 0) const int SSAO_BLUR_SIZE = 4;

layout(location = 0) in vec2 uv;

layout(binding = 0, set = 0) uniform UniformBufferProjectionMatrix {
    mat4 projection;
    mat4 invProjection;
    float nearPlane;
    float farPlane;
};

layout(binding = 1, set = 0) uniform UniformBufferViewMatrix {
    mat4 view;
    mat4 oldView;
};

layout(binding = 0, set = 1) uniform texture2D gbuffer[4];

layout(binding = 0, set = 2) uniform texture2D occlusionMap;

layout(location = 0) out vec4 outColor;

#define MIN_DENOMINATOR 0.00001

const vec4 ambientLightColor = vec4(1.0, 1.0, 1.0, 0.1);
const vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);

const vec3 lightDir = normalize(vec3(-0.5, 0.5, 0.5));

const float PI = acos(-1.0);

float ssaoBlur() {
    ivec2 coord = ivec2(uv * (textureSize(occlusionMap, 0) - 1));

    float res = 0.0;
    for (int y = -SSAO_BLUR_SIZE; y <= SSAO_BLUR_SIZE; y++) {
        for (int x = -SSAO_BLUR_SIZE; x <= SSAO_BLUR_SIZE; x++) {
            res += texelFetch(occlusionMap, coord + ivec2(x, y), 0).r;
        }
    }

    float dim = SSAO_BLUR_SIZE * 2 + 1;
    return res / (dim * dim);
}

float D_GGX(float dotNH, float roughness) {
    float alpha = roughness * roughness;
    float alphaSqr = alpha * alpha;
    float denom = max(dotNH * dotNH * (alphaSqr - 1.0) + 1.0, MIN_DENOMINATOR);
    return alphaSqr / (PI * denom * denom);
}

float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * max(GV, MIN_DENOMINATOR);
}

vec3 FRoughness_Schlick(float c, float metallic, float roughness, vec3 color) {
    vec3 F0 = mix(vec3(0.04), color, metallic);
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - c, 5.0);
}

vec3 F_Schlick(float c, float metallic, vec3 color) {
    vec3 F0 = mix(vec3(0.04), color, metallic);
    return F0 + (1.0 - F0) * pow(1.0 - c, 5.0);
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec2 metallicRoughness, vec3 color) {
    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    float D = D_GGX(dotNH, metallicRoughness.g);
    float G = G_SchlicksmithGGX(dotNL, dotNV, metallicRoughness.g);
    vec3 F = F_Schlick(dotNV, metallicRoughness.r, color);
    float denom = 4.0 * dotNL * dotNV + MIN_DENOMINATOR;
    vec3 spec = D * F * G / denom;

    vec3 Kd = 1.0 - F;
    Kd *= 1.0 - metallicRoughness.r;

    vec3 res = (Kd * color + spec) * dotNL * lightColor.rgb * lightColor.a;

    return dotNL > 0.0 ? res : vec3(0.0);
}

void main() {
    vec3 N = normalize(texelFetch(gbuffer[0], ivec2(gl_FragCoord.xy), 0).xyz * 2.0 - 1.0);
    vec3 V = normalize(-vec3(invProjection * vec4(uv * 2.0 - 1.0, 1.0, 1.0)));

    vec2 metallicRoughness = texelFetch(gbuffer[2], ivec2(gl_FragCoord.xy), 0).rg;
    vec4 origColor = texelFetch(gbuffer[1], ivec2(gl_FragCoord.xy), 0);
    vec3 linearOrigColor = pow(origColor.rgb, vec3(2.2));

    vec3 L = -(mat3(view) * lightDir);
    vec3 Lo = BRDF(L, V, N, metallicRoughness, linearOrigColor);

    vec3 color = Lo + ambientLightColor.rgb * ambientLightColor.a * linearOrigColor * ssaoBlur();

    color = pow(color, vec3(1.0 / 2.2));

    outColor = origColor.a == 0.0 ? origColor : vec4(color, origColor.a);
}