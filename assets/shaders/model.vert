#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;

layout(binding = 0, set = 0) uniform P {
    mat4 projection;
};

layout(binding = 0, set = 1) uniform MV {
    mat4 model;
    mat4 view;
};

layout(location = 0) out vec3 fragpos;
layout(location = 1) out vec3 fragnormal;
layout(location = 2) out vec3 fragtangent;
layout(location = 3) out vec2 fraguv;

void main() {
    vec4 MVpos = view * model * vec4(pos, 1.0);
    gl_Position = projection * MVpos;

    fragpos = MVpos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    fragnormal = normalMatrix * normal;
    fragtangent = normalMatrix * tangent;

    fraguv = uv;
}