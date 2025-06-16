#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in uint textureidx;

layout(binding = 0, set = 0) uniform P {
    mat4 projection;
};

layout(binding = 0, set = 1) uniform MV {
    mat4 model;
    mat4 view;
};

layout(location = 0) out vec3 fragpos;
layout(location = 1) out vec3 fragnormal;
layout(location = 2) out vec2 fraguv;
layout(location = 3) out flat uint fragtextureidx;

void main() {
    vec4 MVpos = view * model * vec4(pos, 1.0);
    gl_Position = projection * MVpos;
    fragpos = MVpos.xyz;

    fragnormal = transpose(inverse(mat3(view * model))) * normal;

    fraguv = uv;
    fragtextureidx = textureidx;
}