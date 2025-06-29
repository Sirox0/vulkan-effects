#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;
layout(location = 4) in flat int textureIndex;

layout(binding = 3, set = 1) uniform sampler2D textures[];

layout(location = 0) out vec4 gbufferPosition;
layout(location = 1) out vec4 gbufferNormal;
layout(location = 2) out vec4 gbufferAlbedo;

void main() {
    gbufferPosition = vec4(pos, 1.0);
    gbufferNormal = vec4(normalize(normal) * 0.5 + 0.5, 1.0);
    if (textureIndex >= 0) {
        gbufferAlbedo = texture(textures[textureIndex], uv);
    } else {
        gbufferAlbedo = vec4(1.0);
    }
}