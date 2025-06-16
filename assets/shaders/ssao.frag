#version 450

layout(constant_id = 0) const uint SSAO_KERNEL_SIZE = 64;
layout(constant_id = 1) const float SSAO_RADIUS = 0.3;
layout(constant_id = 2) const float SSAO_POWER = 2.0;

layout(location = 0) in vec2 uv;

layout(binding = 0, set = 0) uniform P {
    mat4 projection;
};

layout(binding = 0, set = 1) uniform SSAOKernel {
    vec4 kernelSamples[SSAO_KERNEL_SIZE];
};

layout(binding = 1, set = 1) uniform sampler2D ssaoNoise;

layout(binding = 0, set = 2) uniform sampler2D gbuffer[3];

layout(location = 0) out float outOcclusion;

void main() {
    vec3 pos = texture(gbuffer[0], uv).xyz;
    vec3 normal = texture(gbuffer[1], uv).xyz * 2.0 - 1.0;

    vec2 textureDim = textureSize(gbuffer[0], 0);
    vec2 noiseDim = textureSize(ssaoNoise, 0);
    vec2 noiseUV = uv * vec2(textureDim / noiseDim);

    vec3 randV = texture(ssaoNoise, noiseUV).xyz;

    vec3 tangent = normalize(randV - normal * dot(randV, normal));
    vec3 bitangent = cross(tangent, normal);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;

    for (uint i = 0; i < SSAO_KERNEL_SIZE; i++) {
        vec3 samplePos = TBN * kernelSamples[i].xyz;
        samplePos = pos + samplePos * SSAO_RADIUS;

        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float depth = texture(gbuffer[0], offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, SSAO_RADIUS / abs(pos.z - depth));
        occlusion += (depth <= samplePos.z - 0.025 ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - occlusion / float(SSAO_KERNEL_SIZE);

    outOcclusion = pow(occlusion, SSAO_POWER);
}