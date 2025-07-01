#version 460

layout(location = 0) in vec3 uvw;
layout(location = 1) in vec4 pos;
layout(location = 2) in vec4 oldpos;

layout(binding = 0, set = 1) uniform samplerCube skyboxCubemap;

layout(location = 0) out vec4 gbufferPosition;
layout(location = 1) out vec4 gbufferVelocity;
layout(location = 2) out vec4 gbufferNormal;
layout(location = 3) out vec4 gbufferAlbedo;

void main() {
    gbufferAlbedo = texture(skyboxCubemap, uvw);
    gbufferAlbedo.a = 0.0;
    gbufferVelocity = vec4((pos.xy / pos.w) - (oldpos.xy / oldpos.w), 0.0, 1.0);
}