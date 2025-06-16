#version 450

// modified FilmGrain shader from SweetFX

layout(constant_id = 0) const float GRAIN_INTESITY = 0.25;
layout(constant_id = 1) const float GRAIN_SIGNAL_TO_NOISE = 4.0;
layout(constant_id = 2) const float GRAIN_VARIANCE = 0.15;
layout(constant_id = 3) const float GRAIN_NOISE_SHIFT = 0.0;

layout(location = 0) in vec2 uv;

layout(binding = 0) uniform sampler2D frame;

layout(push_constant) uniform PC {
    float time;
};

layout(location = 0) out vec4 outColor;

const float PI = acos(-1.0);

void main() {
    vec4 color = texture(frame, uv);
    // use eye-perceived weights
    float invluma = dot(color.rgb, vec3(-0.2126, -0.7152, -0.0722)) + 1.0;

    // generate noise
    float seed = dot(uv, vec2(12.9898, 78.233)) * time;
    vec2 noise = vec2(fract(sin(seed) * 43758.5453), fract(cos(seed) * 33758.5453));

    float signalToNoise = pow(invluma, GRAIN_SIGNAL_TO_NOISE);
    float variance = (GRAIN_VARIANCE * GRAIN_VARIANCE) * signalToNoise;

    bool triggerLog0 = noise.x < 0.0001;
    vec2 polar = vec2(sqrt(-log((triggerLog0 ? 0.0001 : noise.x))), (2.0 * PI) * noise.y);
    polar.x = triggerLog0 ? PI : polar.x;

    float gaussNoise = GRAIN_VARIANCE * polar.x * cos(polar.y) + GRAIN_NOISE_SHIFT;

    float grain = mix(1.0 + GRAIN_INTESITY, 1.0 - GRAIN_INTESITY, gaussNoise);

    outColor = color * grain;
}