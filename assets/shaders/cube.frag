#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 pos;
layout(location = 2) in vec3 normal;
layout(binding = 1) uniform sampler2DArray textures;
layout(push_constant) uniform PC {
    uint textureIndex;
};

layout(location = 0) out vec4 outColor;

const vec4 ambientLightColor = vec4(1.0, 1.0, 1.0, 0.1);
const vec4 diffuseLightColor = vec4(1.0, 1.0, 1.0, 1.0);
const vec3 lightPos = vec3(0.0, -1.75, 0.0);

void main() {
    vec3 light = vec3(0.0);
    
    // ambient light
    light += ambientLightColor.rgb * ambientLightColor.a;

    // diffuse light
    vec3 nlightDir = lightPos - pos.xyz;
    float attenuation = 1.0 / sqrt(length(nlightDir));
    vec3 dlightColor = diffuseLightColor.rgb * diffuseLightColor.a * attenuation;

    light += max(dot(normalize(normal), normalize(nlightDir)), 0.0) * dlightColor;

    outColor = texture(textures, vec3(uv, textureIndex & 3));
    outColor.rgb *= light;
}