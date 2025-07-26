#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_samplerless_texture_functions : require

#include "common.glsl"

layout(constant_id = 0) const uint SSAO_SAMPLES = 12;
layout(constant_id = 1) const float SSAO_RADIUS = 0.3;
layout(constant_id = 2) const float SSAO_MULTIPLIER = 4.0;
layout(constant_id = 3) const float SSAO_SCALE = 1.0;
layout(constant_id = 4) const float SSAO_BIAS = 0.05;
layout(constant_id = 5) const float SSAO_MAX_DISTANCE = 0.5;
layout(constant_id = 6) const float SSAO_GOLDEN_ANGLE = 2.4;

layout(location = 0) in vec2 uv;

layout(binding = 0, set = 0) uniform UniformBufferProjectionMatrix {
    mat4 projection;
    mat4 invProjection;
    float nearPlane;
    float farPlane;
};

layout(binding = 1, set = 0) uniform UniformBufferViewMatrix {
    mat4 view;
    mat4 invView;
    mat4 oldView;
};

layout(binding = 0, set = 1) uniform texture2D gbuffer[4];

layout(location = 0) out float outOcclusion;

// based on https://www.shadertoy.com/view/Ms33WB

float ao(vec2 tcoord, vec2 inuv, vec3 p, vec3 normal, ivec2 textureDim) {
    vec3 viewRay = vec3(invProjection * vec4((tcoord + inuv) * 2.0 - 1.0, 1.0, 1.0));
    vec3 pos = viewRay * linearDepth(texelFetch(gbuffer[3], ivec2(floor((tcoord + inuv) * (textureDim - 1))), 0).r, nearPlane, farPlane);

    vec3 diff = pos - p;
    float l = length(diff);
    vec3 v = diff / l;
    float d = l * SSAO_SCALE;
    float ao = max(dot(normal, v) - SSAO_BIAS, 0.0) * (1.0 / (1.0 + d));
    return ao * smoothstep(SSAO_MAX_DISTANCE, SSAO_MAX_DISTANCE * 0.5, l);
}

void main() {
    ivec2 textureDim = textureSize(gbuffer[3], 0);

    vec3 viewRay = vec3(invProjection * vec4(uv * 2.0 - 1.0, 1.0, 1.0));
    vec3 pos = viewRay * linearDepth(texelFetch(gbuffer[3], ivec2(floor(uv * (textureDim - 1))), 0).r, nearPlane, farPlane);

    vec3 normal = texelFetch(gbuffer[0], ivec2(floor(uv * (textureDim - 1))), 0).xyz * 2.0 - 1.0;

    float occlusion = 0.0;
    float inv = 1.0 / float(SSAO_SAMPLES);

    float phase = hash12(uv * 467.759) * 2 * acos(-1.0);
    float rStep = inv * (SSAO_RADIUS / pos.z);
    float radius = rStep;
    vec2 spiralUV;

    for (uint i = 0; i < SSAO_SAMPLES; i++) {
        spiralUV = vec2(sin(phase), cos(phase));

        occlusion += ao(uv, spiralUV * radius, pos, normal, textureDim);

        radius += rStep;
        phase += SSAO_GOLDEN_ANGLE;
    }

    occlusion *= inv;
    outOcclusion = 1.0 - occlusion * SSAO_MULTIPLIER;
}