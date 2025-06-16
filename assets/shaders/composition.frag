#version 450

layout(location = 0) in vec2 uv;

layout(binding = 0, set = 0) uniform sampler2D gbufferAlbedo;
layout(binding = 1, set = 0) uniform sampler2D gbufferPosition;
layout(binding = 2, set = 0) uniform sampler2D gbufferNormal;

layout(binding = 0, set = 1) uniform sampler2D occlusionMapBlurred;

layout(push_constant) uniform PC {
    vec3 viewLightPos;
};

layout(location = 0) out vec4 color;

const vec4 ambientLightColor = vec4(1.0, 1.0, 1.0, 0.3);
const vec4 diffuseLightColor = vec4(1.0, 1.0, 1.0, 1.0);
const vec3 lightPos = vec3(0.0, -1.75, 0.0);

void main() {
    vec3 light = vec3(0.0);

    // diffuse light
    vec3 nlightDir = viewLightPos - texture(gbufferPosition, uv).xyz;
    float attenuation = inversesqrt(length(nlightDir));
    vec3 dlightColor = diffuseLightColor.rgb * diffuseLightColor.a * attenuation;

    light += max(dot(texture(gbufferNormal, uv).xyz, normalize(nlightDir)), 0.0) * dlightColor;

    // ambient light
    light += ambientLightColor.rgb * ambientLightColor.a * texture(occlusionMapBlurred, uv).r;

    color = texture(gbufferAlbedo, uv);
    color.rgb *= light;
}