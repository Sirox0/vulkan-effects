#version 450

#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) in vec2 uv;

layout(binding = 0, set = 0) uniform UniformBufferProjectionMatrix {
    mat4 projection;
};

layout(binding = 1, set = 0) uniform UniformBufferViewMatrix {
    mat4 view;
};

layout(binding = 0, set = 1) uniform texture2D gbuffer[3];

layout(binding = 0, set = 2) uniform texture2D occlusionMapBlurred;

layout(location = 0) out vec4 outColor;

const vec4 ambientLightColor = vec4(1.0, 1.0, 1.0, 0.3);
const vec4 diffuseLightColor = vec4(1.0, 1.0, 1.0, 1.0);

//const vec3 lightPos = vec3(0.0, -0.1, 0.0);
const vec3 lightDir = normalize(vec3(-0.5, 0.5, 0.5));

void main() {
    vec3 light = vec3(0.0);

    // diffuse light
    vec3 nlightDir = mat3(view) * -lightDir;
    //vec4 viewLightPos = view * vec4(lightPos, 1.0);
    //vec3 nlightDir = viewLightPos.xyz - texelFetch(gbuffer[0], ivec2(gl_FragCoord.xy), 0).xyz;
    float attenuation = inversesqrt(length(nlightDir.xyz)) / 1.25;
    vec3 dlightColor = diffuseLightColor.rgb * diffuseLightColor.a;

    vec3 normal = texelFetch(gbuffer[1], ivec2(gl_FragCoord.xy), 0).xyz * 2.0 - 1.0;

    light += max(dot(normal, normalize(nlightDir.xyz)), 0.0) * dlightColor;

    // ambient light
    light += ambientLightColor.rgb * ambientLightColor.a * texelFetch(occlusionMapBlurred, ivec2(uv * (textureSize(occlusionMapBlurred, 0) - 1)), 0).r;

    vec4 color = texelFetch(gbuffer[2], ivec2(gl_FragCoord.xy), 0);
    color.rgb *= color.a > 0.0 ? light : vec3(1.0);

    outColor = color;
}