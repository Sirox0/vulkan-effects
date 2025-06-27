#version 460

layout(location = 0) in vec3 uvw;

layout(binding = 0, set = 2) uniform samplerCube skyboxCubemap;

layout(location = 0) out vec4 gbufferPosition;
layout(location = 1) out vec4 gbufferNormal;
layout(location = 2) out vec4 gbufferAlbedo;

void main() {
    gbufferAlbedo = texture(skyboxCubemap, uvw);
    gbufferAlbedo.a = 0.0;
}