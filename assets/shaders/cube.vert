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
layout(location = 1) out vec4 fragpos;
layout(location = 2) out vec3 fragnormal;

void main() {
    fragpos = model * vec4(pos, 1.0);
    gl_Position = projection * view * fragpos;

    vec2 coords;
    switch (textureIndex >> 2) {
        case 0:
            coords = pos.xz;
            fragnormal = normalize(mat3(model) * vec3(0.0, 1.0 * -sign(pos.y), 0.0));
            break;
        case 1:
            coords = pos.zy;
            fragnormal = normalize(mat3(model) * vec3(1.0 * -sign(pos.x), 0.0, 0.0));
            break;
        case 2:
            fragnormal = normalize(mat3(model) * vec3(0.0, 0.0, 1.0 * -sign(pos.z)));
            coords = pos.xy;
            break;
    }
    fraguv = clamp(coords / 2.0, 0.0, 1.0);
}