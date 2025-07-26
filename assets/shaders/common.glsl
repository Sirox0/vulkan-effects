#extension GL_EXT_samplerless_texture_functions : require

float luma(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

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
    ivec2 coord = ivec2(floor(inuv * (textureSize(tex, 0) - 1)));

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

vec3 gammaCorrection(vec3 v, vec3 gamma) { return pow(v, 1.0 / gamma); }
vec3 gammaCorrectionInv(vec3 v, vec3 gamma) { return pow(v, gamma); }

vec3 gaussianBlur17TapThreshold(texture2D tex, ivec2 direction, float threshold, float intensity) {
    const float weights[17] = float[17](
        0.00598,
        0.01262,
        0.02508,
        0.04420,
        0.06974,
        0.09799,
        0.12112,
        0.12632,
        0.13198,
        0.12632,
        0.12112,
        0.09799,
        0.06974,
        0.04420,
        0.02508,
        0.01262,
        0.00598
    );

    vec3 result = vec3(0.0);

    for (int i = -8; i <= 8; i++) {
        vec3 smpl = texelFetch(tex, ivec2(gl_FragCoord.xy) + direction * i, 0).rgb * intensity;
        smpl = luma(smpl) > threshold ? smpl : vec3(0.0);
        result += smpl * weights[i + 8];
    }

    return result;
}

vec3 gaussianBlur17Tap(texture2D tex, ivec2 direction, float intensity) {
    const float weights[17] = float[17](
        0.00598,
        0.01262,
        0.02508,
        0.04420,
        0.06974,
        0.09799,
        0.12112,
        0.12632,
        0.13198,
        0.12632,
        0.12112,
        0.09799,
        0.06974,
        0.04420,
        0.02508,
        0.01262,
        0.00598
    );

    vec3 result = vec3(0.0);

    for (int i = -8; i <= 8; i++) {
        vec3 smpl = texelFetch(tex, ivec2(gl_FragCoord.xy) + direction * i, 0).rgb * intensity;
        result += smpl * weights[i + 8];
    }

    return result;
}

float volumetricLight(sampler2DShadow shadowmapTex, mat4 transform, vec3 lightDir, vec3 camPos, vec3 geomPos, float scatteringFactor, uint steps) {
    vec3 ray = geomPos - camPos;
    float rayLen = length(ray);
    vec3 dir = ray / rayLen;
    float stepLen = 1.0 / float(steps);
    vec3 rayStep = dir * stepLen * rayLen;
    float dotVL = dot(dir, lightDir);

    float scatteringFactorSqr = scatteringFactor * scatteringFactor;
    float scattering = (1.0 - scatteringFactorSqr) / (4.0 * acos(-1.0) * pow(1.0 + scatteringFactorSqr - 2.0 * scatteringFactor * dotVL, 1.5));

    float L = 0.0;
    vec4 rayPos = vec4(camPos, 1.0);

    for (uint i = 0; i < steps; i++) {
        vec4 rayLightSpace = transform * rayPos;
        
        float smpl = textureProj(shadowmapTex, rayLightSpace);
        L += smpl * scattering;

        rayPos.xyz += rayStep;
    }

    return L * stepLen;
}