#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 oldpos;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec2 uv;
layout(location = 5) in flat int textureIndex;
layout(location = 6) in flat int normalMapIndex;

layout(binding = 0, set = 0) uniform UniformBufferProjectionMatrix {
    mat4 projection;
};

layout(binding = 1, set = 0) uniform UniformBufferViewMatrix {
    mat4 view;
    mat4 oldView;
};

layout(binding = 3, set = 1) uniform sampler2D textures[];

layout(location = 0) out vec4 gbufferPosition;
layout(location = 1) out vec4 gbufferVelocity;
layout(location = 2) out vec4 gbufferNormal;
layout(location = 3) out vec4 gbufferAlbedo;

void main() {
    gbufferPosition = vec4(pos.xyz, 0.0);
    vec4 MVPpos = projection * pos;
    gbufferVelocity = vec4(((MVPpos.xy / MVPpos.w) - (oldpos.xy / oldpos.w)), 0.0, 0.0);

    if (normalMapIndex >= 0) {
        vec3 N = normalize(normal);
        vec3 T = normalize(tangent);
        vec3 B = cross(T, N);
        mat3 TBN = mat3(T, B, N);

        vec3 normalMap = TBN * (texture(textures[normalMapIndex], uv).rgb * 2.0 - 1.0);
        gbufferNormal = vec4(normalize(normalMap) * 0.5 + 0.5, 0.0);
    } else {
        gbufferNormal = vec4(normalize(normal) * 0.5 + 0.5, 0.0);
    }

    if (textureIndex >= 0) {
        gbufferAlbedo = texture(textures[textureIndex], uv);
    } else {
        gbufferAlbedo = vec4(1.0);
    }
}