#extension GL_EXT_samplerless_texture_functions : require

float linearDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (near - far));
}

float hash12(vec2 p) {
	vec3 p3  = fract(vec3(p.xyx) * vec3(.1031,.11369,.13787));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float bilateral(texture2D tex, vec2 inuv, int size, float exponent, float factor) {
    ivec2 coord = ivec2(inuv * (textureSize(tex, 0) - 1));

    float center = texelFetch(tex, coord, 0).r;

    float res = 0.0;
    float totalWeights = 0.0;
    for (int y = -size; y <= size; y++) {
        for (int x = -size; x <= size; x++) {
            float spl = texelFetch(tex, coord + ivec2(x, y), 0).r;

            float weight = pow(1.0 - abs(spl - center) * 0.25, exponent);
            res += spl * weight;
            totalWeights += weight;
        }
    }

    return mix(center, res / totalWeights,factor);
}

float PCF(sampler2DShadow tex, vec4 coord, int size, vec2 texelSize) {
    float res = 0.0;
    for (int y = -size; y <= size; y++) {
        for (int x = -size; x <= size; x++) {
            vec4 offset = vec4(x, y, 0.0, 0.0);
            offset.xy *= texelSize * coord.w;

            res += textureProj(tex, coord + offset);
        }
    }

    int dim = size * 2 + 1;
    return res / float(dim * dim);
}