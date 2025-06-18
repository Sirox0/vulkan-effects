#include <iniparser.h>

#include <stdio.h>
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

    config.nearPlane = iniparser_getdouble(conf, "projection:near-plane", 0.01f);
    config.farPlane = iniparser_getdouble(conf, "projection:far-plane", 6.0f);

    config.ssaoResolutionFactor = iniparser_getdouble(conf, "ssao:resolution-factor", 4.0f);
    config.ssaoKernelSize = iniparser_getint(conf, "ssao:kernel-size", 12);
    config.ssaoNoiseDim = iniparser_getint(conf, "ssao:noise-dimension", 4);
    config.ssaoRadius = iniparser_getdouble(conf, "ssao:radius", 0.3f);
    config.ssaoBlurSize = iniparser_getint(conf, "ssao:blur-size", 2);
    config.ssaoPower = iniparser_getdouble(conf, "ssao:power", 8.0f);
    
    config.grainIntensity = iniparser_getdouble(conf, "grain:intensity", 18.0f);
    config.grainSignalToNoise = iniparser_getdouble(conf, "grain:signal-to-noise", 1.0f);
    config.grainNoiseShift = iniparser_getdouble(conf, "grain:noise-shift", 0.0f);

    iniparser_freedict(conf);
}