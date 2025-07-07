#ifndef CONFIG_H
#define CONFIG_H

#include "numtypes.h"

typedef struct {
    u32 windowWidth;
    u32 windowHeight;
    u8 vsync;
    u8 vsyncRelaxed;
    u8 fullscreen;

    u8 validation;
    u8 wireframe;
    u8 preferredTextureFilter;
    f32 maxAnisotropy;
    char modelDirectoryPath[256];
    char modelFile[64];
    f32 modelScale;
    f32 playerSpeed;
    f32 shiftMultiplier;
    f32 targetFps;

    u8 mouseSmoothingEnable;
    f32 mouseSmoothingSpeed;

    f32 fov;
    f32 nearPlane;
    f32 farPlane;

    u32 ssaoResolutionWidth;
    u32 ssaoResolutionHeight;
    u32 ssaoSamples;
    f32 ssaoRadius;
    f32 ssaoMultiplier;
    f32 ssaoScale;
    f32 ssaoBias;
    f32 ssaoMaxDistance;
    f32 ssaoGoldenAngle;
    u32 ssaoDenoiseSize;
    f32 ssaoDenoiseExponent;
    f32 ssaoDenoiseFactor;

    u32 grainEnable;
    f32 grainIntensity;
    f32 grainSignalToNoise;
    f32 grainNoiseShift;

    u32 ditheringEnable;
    f32 ditheringToneCount;

    u8 motionBlurEnable;
    u32 motionBlurMaxSamples;
    f32 motionBlurVelocityReductionFactor;

    u8 fxaaEnable;
    f32 fxaaReduceMin;
    f32 fxaaReduceMul;
    f32 fxaaSpanMax;
} config_t;

void configLoad(char* path);

extern config_t config;

#endif