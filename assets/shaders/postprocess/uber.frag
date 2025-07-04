#version 450

#extension GL_EXT_samplerless_texture_functions : require

layout(constant_id = 0) const float TARGET_FPS = 60.0;

layout(constant_id = 1) const bool GRAIN_ENABLE = true;
layout(constant_id = 2) const float GRAIN_INTESITY = 0.25;
layout(constant_id = 3) const float GRAIN_SIGNAL_TO_NOISE = 4.0;
layout(constant_id = 4) const float GRAIN_NOISE_SHIFT = 0.0;

layout(constant_id = 5) const bool DITHERING_ENABLE = true;
layout(constant_id = 6) const float DITHERING_TONE_COUNT = 32.0;

layout(constant_id = 7) const bool MOTION_BLUR_ENABLE = true;
layout(constant_id = 8) const uint MOTION_BLUR_MAX_SAMPLES = 8;
layout(constant_id = 9) const float MOTION_BLUR_VELOCITY_REDUCTION_FACTOR = 4.0;

layout(constant_id = 10) const bool FXAA_ENABLE = true;
layout(constant_id = 11) const float FXAA_REDUCE_MIN = 0.0078125;
layout(constant_id = 12) const float FXAA_REDUCE_MUL = 0.03125;
layout(constant_id = 13) const float FXAA_SPAN_MAX = 8.0;

layout(location = 0) in vec2 uv;

layout(binding = 0, set = 0) uniform texture2D frame;

layout(binding = 0, set = 1) uniform texture2D gbuffer[4];

layout(push_constant) uniform PC {
    float time;
    float fps;
};

layout(location = 0) out vec4 outColor;

float bayerDitherMatrix16x16[16][16] = float[16][16](
    float[16](0.0, 0.5, 0.125, 0.625, 0.03125, 0.53125, 0.15625, 0.65625, 0.0078125, 0.5078125, 0.1328125, 0.6328125, 0.0390625, 0.5390625, 0.1640625, 0.6640625),
    float[16](0.75, 0.25, 0.875, 0.375, 0.78125, 0.28125, 0.90625, 0.40625, 0.7578125, 0.2578125, 0.8828125, 0.3828125, 0.7890625, 0.2890625, 0.9140625, 0.4140625),
    float[16](0.1875, 0.6875, 0.0625, 0.5625, 0.21875, 0.71875, 0.09375, 0.59375, 0.1953125, 0.6953125, 0.0703125, 0.5703125, 0.2265625, 0.7265625, 0.1015625, 0.6015625),
    float[16](0.9375, 0.4375, 0.8125, 0.3125, 0.96875, 0.46875, 0.84375, 0.34375, 0.9453125, 0.4453125, 0.8203125, 0.3203125, 0.9765625, 0.4765625, 0.8515625, 0.3515625),
    float[16](0.046875, 0.546875, 0.171875, 0.671875, 0.015625, 0.515625, 0.140625, 0.640625, 0.0546875, 0.5546875, 0.1796875, 0.6796875, 0.0234375, 0.5234375, 0.1484375, 0.6484375),
    float[16](0.796875, 0.296875, 0.921875, 0.421875, 0.765625, 0.265625, 0.890625, 0.390625, 0.8046875, 0.3046875, 0.9296875, 0.4296875, 0.7734375, 0.2734375, 0.8984375, 0.3984375),
    float[16](0.234375, 0.734375, 0.109375, 0.609375, 0.203125, 0.703125, 0.078125, 0.578125, 0.2421875, 0.7421875, 0.1171875, 0.6171875, 0.2109375, 0.7109375, 0.0859375, 0.5859375),
    float[16](0.984375, 0.484375, 0.859375, 0.359375, 0.953125, 0.453125, 0.828125, 0.328125, 0.9921875, 0.4921875, 0.8671875, 0.3671875, 0.9609375, 0.4609375, 0.8359375, 0.3359375),
    float[16](0.01171875, 0.51171875, 0.13671875, 0.63671875, 0.04296875, 0.54296875, 0.16796875, 0.66796875, 0.00390625, 0.50390625, 0.12890625, 0.62890625, 0.03515625, 0.53515625, 0.16015625, 0.66015625),
    float[16](0.76171875, 0.26171875, 0.88671875, 0.38671875, 0.79296875, 0.29296875, 0.91796875, 0.41796875, 0.75390625, 0.25390625, 0.87890625, 0.37890625, 0.78515625, 0.28515625, 0.91015625, 0.41015625),
    float[16](0.19921875, 0.69921875, 0.07421875, 0.57421875, 0.23046875, 0.73046875, 0.10546875, 0.60546875, 0.19140625, 0.69140625, 0.06640625, 0.56640625, 0.22265625, 0.72265625, 0.09765625, 0.59765625),
    float[16](0.94921875, 0.44921875, 0.82421875, 0.32421875, 0.98046875, 0.48046875, 0.85546875, 0.35546875, 0.94140625, 0.44140625, 0.81640625, 0.31640625, 0.97265625, 0.47265625, 0.84765625, 0.34765625),
    float[16](0.05859375, 0.55859375, 0.18359375, 0.68359375, 0.02734375, 0.52734375, 0.15234375, 0.65234375, 0.05078125, 0.55078125, 0.17578125, 0.67578125, 0.01953125, 0.51953125, 0.14453125, 0.64453125),
    float[16](0.80859375, 0.30859375, 0.93359375, 0.43359375, 0.77734375, 0.27734375, 0.90234375, 0.40234375, 0.80078125, 0.30078125, 0.92578125, 0.42578125, 0.76953125, 0.26953125, 0.89453125, 0.39453125),
    float[16](0.24609375, 0.74609375, 0.12109375, 0.62109375, 0.21484375, 0.71484375, 0.08984375, 0.58984375, 0.23828125, 0.73828125, 0.11328125, 0.61328125, 0.20703125, 0.70703125, 0.08203125, 0.58203125),
    float[16](0.99609375, 0.49609375, 0.87109375, 0.37109375, 0.96484375, 0.46484375, 0.83984375, 0.33984375, 0.98828125, 0.48828125, 0.86328125, 0.36328125, 0.95703125, 0.45703125, 0.83203125, 0.33203125)
);

float luma(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

float grain(vec3 color) {
    // inverse luma
    float invluma = dot(color, vec3(-0.2126, -0.7152, -0.0722)) + 1.0;

    // generate noise
    float x = (uv.x + 4.0) * (uv.y * 4.0) * (time * 10.0);
    float grain = mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01) - 0.005 + GRAIN_NOISE_SHIFT;

    return grain * GRAIN_INTESITY * pow(invluma, GRAIN_SIGNAL_TO_NOISE);
}

vec3 dither(vec3 color) {
    ivec2 idx = ivec2(gl_FragCoord.xy) % 16;
    return floor(color * (DITHERING_TONE_COUNT - 1) + bayerDitherMatrix16x16[idx.y][idx.x] - 0.5) / (DITHERING_TONE_COUNT - 1);
}

vec3 motionBlur(vec3 color) {
    vec2 velocityTexelSize = 1.0 / vec2(textureSize(gbuffer[2], 0));

    vec2 vel = texelFetch(gbuffer[2], ivec2(gl_FragCoord.xy), 0).ba;
    vel *= fps / TARGET_FPS;

    float speed = length(vel / velocityTexelSize) / MOTION_BLUR_VELOCITY_REDUCTION_FACTOR;
    uint samples = abs(int(speed));
    samples = clamp(samples, 1, MOTION_BLUR_MAX_SAMPLES);

    vec3 res = color;
    ivec2 dim = textureSize(frame, 0) - 1;
    for (uint i = 1; i < samples; i++) {
        ivec2 offset = ivec2(vel * (float(i) / float(samples - 1) - 0.5) * dim);
        offset = clamp(ivec2(gl_FragCoord.xy) + offset, ivec2(0), dim);
        res += texelFetch(frame, offset, 0).rgb;
    }
    res /= float(samples);

    return res;
}

vec3 fxaa(vec3 middlePixel) {
    vec3 northWestPixel = texelFetchOffset(frame, ivec2(gl_FragCoord.xy), 0, ivec2(-1, -1)).rgb;
    vec3 northEastPixel = texelFetchOffset(frame, ivec2(gl_FragCoord.xy), 0, ivec2(1, -1)).rgb;
    vec3 southWestPixel = texelFetchOffset(frame, ivec2(gl_FragCoord.xy), 0, ivec2(-1, 1)).rgb;
    vec3 southEastPixel = texelFetchOffset(frame, ivec2(gl_FragCoord.xy), 0, ivec2(1, 1)).rgb;

    float northWestLuma = luma(northWestPixel);
    float northEastLuma = luma(northEastPixel);
    float southWestLuma = luma(southWestPixel);
    float southEastLuma = luma(southEastPixel);
    float middleLuma = luma(middlePixel);

    float minLuma = min(middleLuma, min(northWestLuma, min(northEastLuma, min(southWestLuma, southEastLuma))));
    float maxLuma = max(middleLuma, max(northWestLuma, max(northEastLuma, max(southWestLuma, southEastLuma))));

    vec2 dir = vec2(-((northWestLuma + northEastLuma) - (southWestLuma + southEastLuma)), (northWestLuma + southWestLuma) - (northEastLuma + southEastLuma));

    float invDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + max((northWestLuma + northEastLuma + southWestLuma + southEastLuma) * FXAA_REDUCE_MUL, FXAA_REDUCE_MIN));
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * invDirMin));

    vec3 color1 = 0.5 * (texelFetch(frame, ivec2(gl_FragCoord.xy) + int(dir * (1.0 / 3.0 - 0.5)), 0).rgb +
        texelFetch(frame, ivec2(gl_FragCoord.xy) + int(dir * (2.0 / 3.0 - 0.5)), 0).rgb);
    vec3 color2 = color1 * 0.5 + 0.25 * (texelFetch(frame, ivec2(gl_FragCoord.xy) + int(dir * -0.5), 0).rgb +
        texelFetch(frame, ivec2(gl_FragCoord.xy) + int(dir * 0.5), 0).rgb);
    
    float luma2 = luma(color2);
    if (luma2 < minLuma || luma2 > maxLuma) return color1;
    else return color2;
}

void main() {
    vec4 color = texelFetch(frame, ivec2(gl_FragCoord.xy), 0);

    if (FXAA_ENABLE) color.rgb = fxaa(color.rgb);

    if (MOTION_BLUR_ENABLE) color.rgb = motionBlur(color.rgb);

    if (DITHERING_ENABLE) color.rgb = dither(color.rgb);

    if (GRAIN_ENABLE) color.rgb += grain(color.rgb);

    outColor = color;
}