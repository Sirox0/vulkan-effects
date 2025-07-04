#version 460

layout(location = 0) in vec3 uvw;
layout(location = 1) in vec4 pos;
layout(location = 2) in vec4 oldpos;

layout(binding = 0, set = 1) uniform samplerCube skyboxCubemap;

layout(location = 0) out vec4 gbufferNormal;
layout(location = 1) out vec4 gbufferAlbedo;
layout(location = 2) out vec4 gbufferMetallicRoughnessVelocity;

void main() {
    gbufferAlbedo.rgb = texture(skyboxCubemap, uvw).rgb;
    gbufferAlbedo.a = 0.0;
    gbufferMetallicRoughnessVelocity = vec4(0.0, 0.0, (pos.xy / pos.w) - (oldpos.xy / oldpos.w));
}