#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_samplerless_texture_functions : require

#include "common.glsl"

layout(constant_id = 0) const float BLOOM_INTENSITY = 1.0;
layout(constant_id = 1) const float BLOOM_THRESHOLD = 1.0;

layout(binding = 0) uniform texture2D frame;

layout(location = 0) out vec4 blurred;

void main() {
    blurred = vec4(gaussianBlur17TapThreshold(frame, ivec2(1.0, 0.0), BLOOM_THRESHOLD, BLOOM_INTENSITY), 1.0);
}