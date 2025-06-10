#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#include <stdlib.h>

#include "config.h"
#include "vkInit.h"
#include "vkFunctions.h"
#include "game.h"

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_EVENTS);
    {
        u32 w = 0, h = 0;
        u8 f = 0;
        if (argc <= 4) {
            if (argc >= 2) f = atoi(argv[1]);
            if (argc >= 3) vkglobals.preferImmediate = atoi(argv[2]);
            w = DEFAULT_WINDOW_WIDTH;
            h = DEFAULT_WINDOW_HEIGHT;
        } else if (argc > 4) {
            f = atoi(argv[1]);
            vkglobals.preferImmediate = atoi(argv[2]);
            w = atoi(argv[3]);
            h = atoi(argv[4]);
        }
        
        vkglobals.window = SDL_CreateWindow(TITLE, w, h, SDL_WINDOW_VULKAN | (f ? SDL_WINDOW_FULLSCREEN : 0));
    }
    if (vkglobals.window == VK_NULL_HANDLE) {
        printf("failed to create window\n");
        exit(1);
    }

    SDL_SetWindowRelativeMouseMode(vkglobals.window, true);

    vkInit();
    gameInit();

    SDL_Event event;
    while (gameglobals.loopActive) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                gameglobals.loopActive = 0;
                break;
            } else gameEvent(&event);
        }

        u32 startTime = SDL_GetTicks();

        gameRender();

        gameglobals.deltaTime = SDL_GetTicks() - startTime;
    }

    vkDeviceWaitIdle(vkglobals.device);
    gameQuit();
    vkQuit();

    SDL_DestroyWindow(vkglobals.window);
    SDL_Quit();
    
    return 0;
}