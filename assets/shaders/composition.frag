#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_samplerless_texture_functions : require

#include "common.glsl"

layout(constant_id = 0) const int SSAO_DENOISE_SIZE = 4;
layout(constant_id = 1) const float SSAO_DENOISE_EXPONENT = 5.0;
layout(constant_id = 2) const float SSAO_DENOISE_FACTOR = 0.75;

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

layout(binding = 0, set = 3) readonly buffer StorageBufferLight {
    mat4 lightVP;
    vec4 lightPos;
};

layout(binding = 0, set = 4) uniform sampler2DShadow shadowmap;

layout(location = 0) out vec4 outColor;

#define MIN 0.00001

const vec4 ambientLightColor = vec4(1.0, 1.0, 1.0, 0.1);
const vec4 lightColor = vec4(1.0, 0.0, 0.0, 1.0);

const vec3 lightDir = normalize(vec3(-0.5, 0.5, 0.5));

const float PI = acos(-1.0);

float D_GGX(float dotNH, float roughness) {
    float alpha = roughness * roughness;
    float alphaSqr = alpha * alpha;
    float denom = max(dotNH * dotNH * (alphaSqr - 1.0) + 1.0, MIN);
    return alphaSqr / (PI * denom * denom);
}

float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * max(GV, MIN);
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
    float denom = 4.0 * dotNL * dotNV + MIN;
    vec3 spec = D * F * G / denom;

    vec3 Kd = 1.0 - F;
    Kd *= 1.0 - metallicRoughness.r;

    vec3 res = (Kd * color + spec) * dotNL * lightColor.rgb * lightColor.a;

    return dotNL > 0.0 ? res : vec3(0.0);
}

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);

void main() {
    vec3 N = normalize(texelFetch(gbuffer[0], ivec2(gl_FragCoord.xy), 0).xyz * 2.0 - 1.0);
    vec3 V = normalize(-vec3(invProjection * vec4(uv * 2.0 - 1.0, 1.0, 1.0)));

    vec2 metallicRoughness = texelFetch(gbuffer[2], ivec2(gl_FragCoord.xy), 0).rg;
    vec4 origColor = texelFetch(gbuffer[1], ivec2(gl_FragCoord.xy), 0);
    vec3 linearOrigColor = pow(origColor.rgb, vec3(2.2));

    vec3 viewRay = vec3(invProjection * vec4(uv * 2.0 - 1.0, 1.0, 1.0));
    float depth = texelFetch(gbuffer[3], ivec2(uv * (textureSize(gbuffer[3], 0) - 1)), 0).r;
    vec3 pos = viewRay * linearDepth(depth, nearPlane, farPlane);

    vec4 shadowUV = (biasMat * lightVP) * (inverse(view) * vec4(pos, 1.0));
    shadowUV.z += 0.00002;

    float shadow = PCF(shadowmap, shadowUV, 4, 1.0 / vec2(textureSize(shadowmap, 0)));

    vec3 L = normalize(lightPos.xyz - pos);
    vec3 Lo = BRDF(L, V, N, metallicRoughness, linearOrigColor) * shadow;

    vec3 color = Lo + ambientLightColor.rgb * ambientLightColor.a * linearOrigColor * bilateral(occlusionMap, uv, SSAO_DENOISE_SIZE, SSAO_DENOISE_EXPONENT, SSAO_DENOISE_FACTOR);

    color = pow(color, vec3(1.0 / 2.2));

    outColor = origColor.a == 0.0 ? origColor : vec4(color, origColor.a);
}