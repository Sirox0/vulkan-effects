#version 450

#extension GL_ARB_shader_draw_parameters : require

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

struct material {
    int textureIndex;
    int normalMapIndex;
};

layout(binding = 0, set = 2, std430) readonly buffer StorageBufferMaterials {
    material mats[];
};

layout(binding = 1, set = 2, std430) readonly buffer StorageBufferMaterialIndicies {
    uint matIndices[];
};

layout(location = 0) out vec3 fragpos;
layout(location = 1) out vec3 fragnormal;
layout(location = 2) out vec3 fragtangent;
layout(location = 3) out vec2 fraguv;
layout(location = 4) out flat int textureIndex;

void main() {
    vec4 MVpos = view * model * vec4(pos, 1.0);
    gl_Position = projection * MVpos;

    fragpos = MVpos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    fragnormal = normalMatrix * normal;
    fragtangent = normalMatrix * tangent;

    fraguv = uv;

    textureIndex = mats[matIndices[gl_DrawIDARB]].textureIndex;
}