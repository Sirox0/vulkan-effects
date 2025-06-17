#ifndef CONFIG_H
#define CONFIG_H

#define TITLE "walker"
#define DEFAULT_WINDOW_WIDTH 1024
#define DEFAULT_WINDOW_HEIGHT 1024

#define NEAR_PLANE 0.01f
#define FAR_PLANE 6.0f

// the resolution of occlusion map is window resolution divided by this
#define SSAO_RESOLUTION_FACTOR 4
#define SSAO_KERNEL_SIZE 12
#define SSAO_NOISE_DIM 4
#define SSAO_RADIUS 0.3f
#define SSAO_BLUR_SCALE 2
#define SSAO_POWER 8.0f

#define GRAIN_INTESITY 18.0f
#define GRAIN_SIGNAL_TO_NOISE 1.0f
#define GRAIN_NOISE_SHIFT 0.0f

#endif