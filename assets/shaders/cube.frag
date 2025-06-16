#version 450


layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in flat uint textureidx;

layout(binding = 1, set = 1) uniform sampler2DArray textures;

layout(location = 0) out vec4 gbufferAlbedo;
layout(location = 1) out vec4 gbufferPosition;
layout(location = 2) out vec4 gbufferNormal;

void main() {
    gbufferAlbedo = texture(textures, vec3(uv, textureidx));
    gbufferPosition = vec4(pos, 1.0);
    gbufferNormal = vec4(normalize(normal), 1.0);
}