#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"

#include "offsets.h"
#include "sysprocs_st.h"

#ifndef SORR_IOS_SHELL_CLEAR_R
#define SORR_IOS_SHELL_CLEAR_R 12
#endif

#ifndef SORR_IOS_SHELL_CLEAR_G
#define SORR_IOS_SHELL_CLEAR_G 18
#endif

#ifndef SORR_IOS_SHELL_CLEAR_B
#define SORR_IOS_SHELL_CLEAR_B 28
#endif

FN_HOOK *module_finalize_list = NULL;
int module_finalize_allocated = 0;
int module_finalize_count = 0;
void *globaldata = NULL;

void bgdrtm_entry(int argc, char *argv[]);

static int sorr_ios_runtime_entry_probe(int argc, char **argv)
{
    SDL_Log("SORR iOS shell: reached Bennu runtime handoff probe argc=%d argv0=%s",
            argc,
            (argc > 0 && argv && argv[0]) ? argv[0] : "(null)");
    globaldata = calloc(OS_ID + (int)sizeof(uint32_t), 1);
    if (!globaldata)
    {
        SDL_Log("SORR iOS shell: failed to allocate minimal bgdrtm globaldata");
        return 1;
    }

    bgdrtm_entry(argc, argv);
    SDL_Log("SORR iOS shell: bgdrtm_entry returned");
    SDL_Log("SORR iOS shell: SorR.dat intentionally not loaded in milestone 1");
    return 0;
}

int main(int argc, char *argv[])
{
#ifdef SDL_MAIN_HANDLED
    SDL_SetMainReady();
#endif

    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    SDL_Log("SORR iOS shell: app entry");

    if (SDL_Init(0) != 0)
    {
        SDL_Log("SORR iOS shell: SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Log("SORR iOS shell: SDL_Init ok");

    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        SDL_Log("SORR iOS shell: SDL video/events/timer init failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Log("SORR iOS shell: SDL video/events/timer init ok");

    SDL_Window *window = SDL_CreateWindow("SorR iOS Shell",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          960,
                                          540,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("SORR iOS shell: SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_Log("SORR iOS shell: SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (sorr_ios_runtime_entry_probe(argc, argv) != 0)
    {
        SDL_Log("SORR iOS shell: runtime handoff probe failed");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Log("SORR iOS shell: entering responsive idle loop");

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer,
                               SORR_IOS_SHELL_CLEAR_R,
                               SORR_IOS_SHELL_CLEAR_G,
                               SORR_IOS_SHELL_CLEAR_B,
                               255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_Log("SORR iOS shell: clean shutdown");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    free(globaldata);
    globaldata = NULL;
    return 0;
}
