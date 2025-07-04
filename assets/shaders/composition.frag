#version 450

#extension GL_EXT_samplerless_texture_functions : require

layout(constant_id = 0) const int SSAO_BLUR_SIZE = 4;

layout(location = 0) in vec2 uv;

layout(binding = 1, set = 0) uniform UniformBufferViewMatrix {
    mat4 view;
    mat4 oldView;
};

layout(binding = 0, set = 1) uniform texture2D gbuffer[4];

layout(binding = 0, set = 2) uniform texture2D occlusionMap;

layout(location = 0) out vec4 outColor;

const vec4 ambientLightColor = vec4(1.0, 1.0, 1.0, 0.3);
const vec4 diffuseLightColor = vec4(1.0, 1.0, 1.0, 1.0);

const vec3 lightDir = normalize(vec3(-0.5, 0.5, 0.5));

float ssaoBlur() {
    ivec2 coord = ivec2(uv * (textureSize(occlusionMap, 0) - 1));

    float res = 0.0;
    for (int y = -SSAO_BLUR_SIZE; y <= SSAO_BLUR_SIZE; y++) {
        for (int x = -SSAO_BLUR_SIZE; x <= SSAO_BLUR_SIZE; x++) {
            res += texelFetch(occlusionMap, coord + ivec2(x, y), 0).r;
        }
    }

    float dim = SSAO_BLUR_SIZE * 2 + 1;
    return res / (dim * dim);
}

void main() {
    vec3 light = vec3(0.0);

    // diffuse light
    vec3 nlightDir = mat3(view) * -lightDir;
    float attenuation = inversesqrt(length(nlightDir.xyz)) / 1.25;
    vec3 dlightColor = diffuseLightColor.rgb * diffuseLightColor.a;

    vec3 normal = texelFetch(gbuffer[0], ivec2(gl_FragCoord.xy), 0).xyz * 2.0 - 1.0;

    light += max(dot(normal, normalize(nlightDir.xyz)), 0.0) * dlightColor;

    // ambient light
    light += ambientLightColor.rgb * ambientLightColor.a * ssaoBlur();

    vec4 color = texelFetch(gbuffer[1], ivec2(gl_FragCoord.xy), 0);
    color.rgb *= color.a > 0.0 ? light : vec3(1.0);

    outColor = color;
}