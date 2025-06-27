#version 450

layout(location = 0) in vec3 pos;

layout(binding = 0, set = 0) uniform Proj {
    mat4 projection;
};

layout(binding = 0, set = 1) uniform ModelView {
    mat4 model;
    mat4 view;
};

layout(location = 0) out vec3 uvw;

void main() {
    gl_Position = projection * view * vec4(pos, 1.0);

    uvw = pos;
    uvw.xy *= -1.0;
}