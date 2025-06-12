#version 450

layout(location = 0) in vec2 uv;
layout(binding = 1) uniform sampler2DArray textures;
layout(push_constant) uniform PC {
    uint textureIndex;
};

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textures, vec3(uv, textureIndex & 3));
}