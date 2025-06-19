#version 450

layout(location = 0) in vec2 uv;

layout(binding = 0, set = 0) uniform sampler2D gbuffer[3];

layout(binding = 0, set = 1) uniform sampler2D occlusionMapBlurred;

layout(push_constant) uniform PC {
    vec3 viewLightPos;
};

layout(location = 0) out vec4 outColor;

const vec4 ambientLightColor = vec4(1.0, 1.0, 1.0, 0.3);
const vec4 diffuseLightColor = vec4(1.0, 1.0, 1.0, 1.0);
const vec3 lightPos = vec3(0.0, -1.75, 0.0);

void main() {
    vec3 light = vec3(0.0);

    // diffuse light
    vec3 nlightDir = viewLightPos - texelFetch(gbuffer[0], ivec2(gl_FragCoord.xy), 0).xyz;
    float attenuation = inversesqrt(length(nlightDir)) / 1.25;
    vec3 dlightColor = diffuseLightColor.rgb * diffuseLightColor.a * attenuation;

    light += max(dot(texelFetch(gbuffer[1], ivec2(gl_FragCoord.xy), 0).xyz * 2.0 - 1.0, normalize(nlightDir)), 0.0) * dlightColor;

    // ambient light
    light += ambientLightColor.rgb * ambientLightColor.a * texelFetch(occlusionMapBlurred, ivec2(uv * textureSize(occlusionMapBlurred, 0)), 0).r;

    vec4 color = texelFetch(gbuffer[2], ivec2(gl_FragCoord.xy), 0);
    color.rgb *= light;

    outColor = color;
}