#include <iniparser.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "numtypes.h"
#include "config.h"

config_t config;

void configLoad(char* path) {
    dictionary* conf = iniparser_load(path);

    config.windowWidth = iniparser_getint(conf, "window:width", 1024);
    config.windowHeight = iniparser_getint(conf, "window:height", 1024);
    config.vsync = iniparser_getboolean(conf, "window:vsync", 1);
    config.vsyncRelaxed = iniparser_getboolean(conf, "window:vsync-relaxed", 1);
    config.fullscreen = iniparser_getboolean(conf, "window:fullscreen", 0);

    config.wireframe = iniparser_getboolean(conf, "general:wireframe", 0);
    config.preferredTextureFilter = iniparser_getint(conf, "general:preferred-texture-filter", 2);
    config.maxAnisotropy = iniparser_getdouble(conf, "general:max-anisotropy", 0.0f);

    const char* modelDirectoryPath = iniparser_getstring(conf, "general:model-directory-path", "unknown");
    if (strlen(modelDirectoryPath) > 256) {
        printf("general:model-directory-path must contain no more than 256 symbols\n");
        exit(1);
    } 
    strcpy(config.modelDirectoryPath, modelDirectoryPath);

    const char* modelPath = iniparser_getstring(conf, "general:model-file", "unknown");
    if (strlen(modelPath) > 64) {
        printf("general:model-file must contain no more than 64 symbols\n");
        exit(1);
    } 
    strcpy(config.modelFile, modelPath);

    config.modelScale = iniparser_getdouble(conf, "general:model-scale", 1.0f);
    config.playerSpeed = iniparser_getdouble(conf, "general:player-speed", 1.0f);
    config.shiftMultiplier = iniparser_getdouble(conf, "general:shift-multiplier", 3.0f);
    config.targetFps = iniparser_getdouble(conf, "general:target-fps", 60.0f);

    config.mouseSmoothingEnable = iniparser_getboolean(conf, "mouse-smoothing:enable", 0);
    config.mouseSmoothingSpeed = iniparser_getdouble(conf, "mouse-smoothing:speed", 100.0f);

    config.fov = iniparser_getdouble(conf, "projection:fov", 80.0f);
    config.nearPlane = iniparser_getdouble(conf, "projection:near-plane", 0.01f);
    config.farPlane = iniparser_getdouble(conf, "projection:far-plane", 6.0f);

    config.ssaoResolutionFactor = iniparser_getdouble(conf, "ssao:resolution-factor", 4.0f);
    config.ssaoKernelSize = iniparser_getint(conf, "ssao:kernel-size", 12);
    config.ssaoNoiseDim = iniparser_getint(conf, "ssao:noise-dimension", 4);
    config.ssaoRadius = iniparser_getdouble(conf, "ssao:radius", 0.3f);
    config.ssaoBlurSize = iniparser_getint(conf, "ssao:blur-size", 2);
    config.ssaoPower = iniparser_getdouble(conf, "ssao:power", 8.0f);
    
    config.grainEnable = iniparser_getboolean(conf, "grain:enable", 1);
    config.grainIntensity = iniparser_getdouble(conf, "grain:intensity", 18.0f);
    config.grainSignalToNoise = iniparser_getdouble(conf, "grain:signal-to-noise", 1.0f);
    config.grainNoiseShift = iniparser_getdouble(conf, "grain:noise-shift", 0.0f);

    config.ditheringEnable = iniparser_getboolean(conf, "dithering:enable", 1);
    config.ditheringToneCount = iniparser_getdouble(conf, "dithering:tone-count", 32.0f);

    config.motionBlurEnable = iniparser_getboolean(conf, "motion-blur:enable", 1);
    config.motionBlurMaxSamples = iniparser_getint(conf, "motion-blur:max-samples", 8);
    config.motionBlurVelocityReductionFactor = iniparser_getdouble(conf, "motion-blur:velocity-reduction-factor", 4.0f);

    iniparser_freedict(conf);
}