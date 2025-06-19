#version 450

layout(constant_id = 0) const float GRAIN_INTESITY = 0.25;
layout(constant_id = 1) const float GRAIN_SIGNAL_TO_NOISE = 4.0;
layout(constant_id = 2) const float GRAIN_NOISE_SHIFT = 0.0;

layout(location = 0) in vec2 uv;

layout(binding = 0) uniform sampler2D frame;

layout(push_constant) uniform PC {
    float time;
};

layout(location = 0) out vec4 outColor;

const float PI = acos(-1.0);

float luma(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 grain(vec3 color) {
    // inverse luma
    float invluma = dot(color, vec3(-0.2126, -0.7152, -0.0722)) + 1.0;

    // generate noise
    float x = (uv.x + 4.0) * (uv.y * 4.0) * (time * 10.0);
    float grain = mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01) - 0.005 + GRAIN_NOISE_SHIFT;

    return color + grain * GRAIN_INTESITY * pow(invluma, GRAIN_SIGNAL_TO_NOISE);
}

void main() {
    vec4 color = texelFetch(frame, ivec2(gl_FragCoord.xy), 0);

    color.rgb = grain(color.rgb);

    outColor = color;
}