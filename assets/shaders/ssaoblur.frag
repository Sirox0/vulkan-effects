#version 450

layout(constant_id = 0) const int SSAO_BLUR_SIZE = 4;

layout(location = 0) in vec2 uv;

layout(binding = 0) uniform sampler2D occlusionMap;

layout(location = 0) out float occlusionBlur;

void main() {
    float res = 0.0;
    for (int y = -SSAO_BLUR_SIZE; y <= SSAO_BLUR_SIZE; y++) {
        for (int x = -SSAO_BLUR_SIZE; x <= SSAO_BLUR_SIZE; x++) {
            res += texelFetch(occlusionMap, ivec2(gl_FragCoord.xy) + ivec2(x, y), 0).r;
        }
    }

    float dim = SSAO_BLUR_SIZE * 2 + 1;
    occlusionBlur = res / (dim * dim);
}