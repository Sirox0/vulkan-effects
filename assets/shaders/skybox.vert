#version 450

layout(location = 0) in vec3 pos;

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

layout(location = 0) out vec3 fraguvw;
layout(location = 1) out vec4 fragpos;
layout(location = 2) out vec4 fragoldpos;

void main() {
    vec4 Mpos = vec4(pos, 1.0);
    vec4 MVPpos = projection * view * Mpos;
    gl_Position =  MVPpos;

    fragpos = MVPpos;
    fragoldpos = projection * oldView * Mpos;

    fraguvw = pos;
    fraguvw.xy *= -1.0;
}