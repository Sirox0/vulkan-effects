#version 460

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 oldpos;

layout(location = 2) out vec4 gbufferMetallicRoughnessVelocity;

void main() {
    gbufferMetallicRoughnessVelocity = vec4(0.0, 0.0, (pos.xy / pos.w) - (oldpos.xy / oldpos.w));
}