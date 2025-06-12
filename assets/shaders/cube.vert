#version 450

layout(location = 0) in vec3 pos;
layout(push_constant) uniform PC {
    uint textureIndex;
};

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out vec2 fraguv;

void main() {
    gl_Position = projection * view * model * vec4(pos, 1.0);

    vec2 coords;
    switch (textureIndex >> 2) {
        case 0: coords = pos.xz; break;
        case 1: coords = pos.zy; break;
        case 2: coords = pos.xy; break;
    }
    fraguv = clamp(coords / 2.0, 0.0, 1.0);
}