#ifndef CONFIG_H
#define CONFIG_H

#include "numtypes.h"

typedef struct {
    u32 windowWidth;
    u32 windowHeight;
    u8 vsync;
    u8 vsyncRelaxed;
    u8 fullscreen;

    f32 fov;
    f32 nearPlane;
    f32 farPlane;

    u8 rayTracing;

    f32 ssaoResolutionFactor;
    u32 ssaoKernelSize;
    u32 ssaoNoiseDim;
    f32 ssaoRadius;
    u32 ssaoBlurSize;
    f32 ssaoPower;

    f32 grainIntensity;
    f32 grainSignalToNoise;
    f32 grainNoiseShift;
} config_t;

void configLoad(char* path);

extern config_t config;

#endif