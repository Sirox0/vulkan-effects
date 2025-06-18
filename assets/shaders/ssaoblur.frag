#version 450

layout(constant_id = 0) const int SSAO_BLUR_SIZE = 4;

layout(location = 0) in vec2 uv;

layout(binding = 0) uniform sampler2D occlusionMap;

layout(location = 0) out float occlusionBlur;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(occlusionMap, 0));

    float n = 0.0;
    float res = 0.0;
    for (int y = -SSAO_BLUR_SIZE; y <= SSAO_BLUR_SIZE; y++) {
        for (int x = -SSAO_BLUR_SIZE; x <= SSAO_BLUR_SIZE; x++) {
            vec2 offset = texelSize * vec2(x, y);
            res += texture(occlusionMap, uv + offset).r;

            n += 1.0;
        }
    }

    occlusionBlur = res / n;
}