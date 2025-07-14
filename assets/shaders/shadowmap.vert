#version 450

#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) in vec3 pos;

layout(binding = 0, set = 0) readonly buffer StorageBufferLight {
    mat4 lightVP;
    vec4 lightPos;
};

layout(binding = 0, set = 1, std430) readonly buffer StorageBufferModelMatrix {
    mat4 model;
};

layout(binding = 2, set = 1, std430) readonly buffer StorageBufferTransforms {
    mat4 transforms[];
};

struct MeshIndices {
    int materialIndex;
    uint nodeIndex;
};

layout(binding = 3, set = 1, std430) readonly buffer StorageBufferMaterialIndicies {
    MeshIndices meshIndices[];
};

void main() {
    gl_Position = lightVP * model * transforms[meshIndices[gl_DrawIDARB].nodeIndex] * vec4(pos, 1.0);
}