#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "vkInit.h"
#include "vkFunctions.h"
#include "game.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    configLoad("config.ini");

    SDL_Init(SDL_INIT_EVENTS);
    {
        vkglobals.window = SDL_CreateWindow("walker", config.windowWidth, config.windowHeight, SDL_WINDOW_VULKAN | (config.fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
    }
    if (vkglobals.window == VK_NULL_HANDLE) {
        printf("failed to create window\n");
        exit(1);
    }

    SDL_SetWindowRelativeMouseMode(vkglobals.window, true);

    srand(time(NULL));

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

        gameglobals.time = SDL_GetTicks();

        gameRender();

        gameglobals.deltaTime = SDL_GetTicks() - gameglobals.time;
    }

    vkDeviceWaitIdle(vkglobals.device);
    gameQuit();
    vkQuit();

    SDL_DestroyWindow(vkglobals.window);
    SDL_Quit();
    
    return 0;
}