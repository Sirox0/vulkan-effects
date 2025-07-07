#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "vk.h"
#include "vkFunctions.h"
#include "modelViewScene.h"
#include "scene.h"

scene_t curscene = {};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    configLoad("config.ini");

    SDL_Init(SDL_INIT_EVENTS);
    {
        vkglobals.window = SDL_CreateWindow("vulkan-effects", config.windowWidth, config.windowHeight, SDL_WINDOW_VULKAN | (config.fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
    }
    if (vkglobals.window == VK_NULL_HANDLE) {
        printf("failed to create window\n");
        exit(1);
    }

    SDL_SetWindowRelativeMouseMode(vkglobals.window, true);

    srand(time(NULL));

    vkInit();

    curscene.globals = malloc(sizeof(ModelViewSceneGlobals_t));
    curscene.init = modelViewSceneInit;
    curscene.event = modelViewSceneEvent;
    curscene.render = modelViewSceneRender;
    curscene.quit = modelViewSceneQuit;

    curscene.init();

    SDL_Event event;
    while (vkglobals.loopActive) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                vkglobals.loopActive = 0;
                break;
            } else curscene.event(&event);
        }

        vkglobals.time = SDL_GetTicksNS();

        curscene.render();

        vkglobals.deltaTime = SDL_GetTicksNS() - vkglobals.time;
        vkglobals.fps = 1e9f / vkglobals.deltaTime;
    }

    vkDeviceWaitIdle(vkglobals.device);
    curscene.quit();
    free(curscene.globals);
    vkQuit();

    SDL_DestroyWindow(vkglobals.window);
    SDL_Quit();
    
    return 0;
}