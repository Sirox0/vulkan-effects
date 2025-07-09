#version 450

layout(location = 0) in vec3 pos;

layout(binding = 0, set = 0) readonly buffer StorageBufferLight {
    mat4 lightVP;
    vec4 lightPos;
};

layout(binding = 0, set = 1, std430) readonly buffer StorageBufferModelMatrix {
    mat4 model;
};

void main() {
    gl_Position = lightVP * model * vec4(pos, 1.0);
}