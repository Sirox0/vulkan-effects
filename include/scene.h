#ifndef SCENE_H
#define SCENE_H

#include <SDL3/SDL_events.h>

#include <stdlib.h>

typedef struct {
    void* globals;

    void (*init)();
    void (*event)(SDL_Event* e);
    void (*render)();
    void (*quit)();
} scene_t;

extern scene_t curscene;

inline void sceneSwitch(scene_t newscene) {
    curscene.quit();
    free(curscene.globals);

    curscene = newscene;
    curscene.init();
}

#endif