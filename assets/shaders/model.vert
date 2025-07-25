#version 450

#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;

layout(binding = 0, set = 0) uniform UniformBufferProjectionMatrix {
    mat4 projection;
    mat4 invProjection;
    float nearPlane;
    float farPlane;
};

layout(binding = 1, set = 0) uniform UniformBufferViewMatrix {
    mat4 view;
    mat4 invView;
    mat4 oldView;
};

layout(binding = 0, set = 1, std430) readonly buffer StorageBufferModelMatrix {
    mat4 model;
};

struct material {
    int textureIndex;
    int normalMapIndex;
    int metallicRoughnessIndex;
};

layout(binding = 1, set = 1, std430) readonly buffer StorageBufferMaterials {
    material mats[];
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

layout(location = 0) out vec4 fragpos;
layout(location = 1) out vec4 fragoldpos;
layout(location = 2) out vec3 fragnormal;
layout(location = 3) out vec3 fragtangent;
layout(location = 4) out vec2 fraguv;
layout(location = 5) out flat int textureIndex;
layout(location = 6) out flat int normalMapIndex;
layout(location = 7) out flat int metallicRoughnessIndex;

void main() {
    vec4 Mpos = model * transforms[meshIndices[gl_DrawIDARB].nodeIndex] * vec4(pos, 1.0);
    vec4 MVPpos = projection * view * Mpos;
    gl_Position = MVPpos;

    fragpos = MVPpos;
    fragoldpos = projection * oldView * Mpos;

    mat3 normalMatrix = transpose(inverse(mat3(view * model * transforms[meshIndices[gl_DrawIDARB].nodeIndex])));
    fragnormal = normalMatrix * normal;
    fragtangent = normalMatrix * tangent;

    fraguv = uv;

    textureIndex = mats[meshIndices[gl_DrawIDARB].materialIndex].textureIndex;
    normalMapIndex = mats[meshIndices[gl_DrawIDARB].materialIndex].normalMapIndex;
    metallicRoughnessIndex = mats[meshIndices[gl_DrawIDARB].materialIndex].metallicRoughnessIndex;
}