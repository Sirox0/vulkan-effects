#ifndef CONFIG_H
#define CONFIG_H

#include "numtypes.h"

typedef struct {
    u32 windowWidth;
    u32 windowHeight;
    u8 vsync;
    u8 vsyncRelaxed;
    u8 fullscreen;

    u8 wireframe;
    u8 preferredTextureFilter;
    f32 maxAnisotropy;
    char modelDirectoryPath[256];
    char modelFile[64];
    f32 modelScale;
    f32 playerSpeed;
    f32 shiftMultiplier;

    f32 fov;
    f32 nearPlane;
    f32 farPlane;

    f32 ssaoResolutionFactor;
    u32 ssaoKernelSize;
    u32 ssaoNoiseDim;
    f32 ssaoRadius;
    u32 ssaoBlurSize;
    f32 ssaoPower;

    u32 grainEnable;
    f32 grainIntensity;
    f32 grainSignalToNoise;
    f32 grainNoiseShift;

    u32 ditheringEnable;
    f32 ditheringToneCount;
} config_t;

void configLoad(char* path);

extern config_t config;

#endif