#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <errno.h>
#define SORR_MKDIR(path) _mkdir(path)
#else
#include <errno.h>
#define SORR_MKDIR(path) mkdir((path), 0755)
#endif

#include "SDL.h"

#include "offsets.h"
#include "sysprocs_st.h"

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFDIR) != 0)
#endif

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

typedef struct sorr_ios_data_layout
{
    char bundle_root[1024];
    char support_root[1024];
    char savegame_dir[1024];
    char xbox_dir[1024];
    char logs_dir[1024];
} sorr_ios_data_layout;

static int sorr_ios_join_path(char *out, size_t out_size, const char *base, const char *leaf)
{
    const char *separator = "/";
    size_t base_len;
    int written;

    if (!out || out_size == 0 || !base || !leaf)
    {
        return 0;
    }

    base_len = strlen(base);
    if (base_len > 0 && (base[base_len - 1] == '/' || base[base_len - 1] == '\\'))
    {
        separator = "";
    }

    written = snprintf(out, out_size, "%s%s%s", base, separator, leaf);
    return written > 0 && (size_t)written < out_size;
}

static int sorr_ios_mkdir_if_needed(const char *path)
{
    struct stat st;

    if (!path || !*path)
    {
        return 0;
    }

    if (stat(path, &st) == 0)
    {
        return S_ISDIR(st.st_mode);
    }

    if (SORR_MKDIR(path) == 0)
    {
        return 1;
    }

    return errno == EEXIST;
}

static int sorr_ios_create_dir_marker(const char *label, const char *path)
{
    if (!sorr_ios_mkdir_if_needed(path))
    {
        SDL_Log("SORR iOS shell: %s dir create failed path=%s errno=%d",
                label,
                path ? path : "(null)",
                errno);
        return 0;
    }

    SDL_Log("SORR iOS shell: %s dir created path=%s", label, path);
    return 1;
}

static int sorr_ios_write_read_probe(const char *logs_dir)
{
    char proof_path[1024];
    char readback[128];
    const char *proof_text = "sorr-ios-data-layout-ok\n";
    FILE *fp;

    if (!sorr_ios_join_path(proof_path, sizeof(proof_path), logs_dir, "ios_data_layout_probe.txt"))
    {
        SDL_Log("SORR iOS shell: data layout test file path build failed");
        return 0;
    }

    fp = fopen(proof_path, "wb");
    if (!fp)
    {
        SDL_Log("SORR iOS shell: data layout test file write open failed path=%s errno=%d",
                proof_path,
                errno);
        return 0;
    }

    if (fwrite(proof_text, 1, strlen(proof_text), fp) != strlen(proof_text))
    {
        SDL_Log("SORR iOS shell: data layout test file write failed path=%s errno=%d",
                proof_path,
                errno);
        fclose(fp);
        return 0;
    }

    fclose(fp);

    fp = fopen(proof_path, "rb");
    if (!fp)
    {
        SDL_Log("SORR iOS shell: data layout test file read open failed path=%s errno=%d",
                proof_path,
                errno);
        return 0;
    }

    memset(readback, 0, sizeof(readback));
    if (!fgets(readback, sizeof(readback), fp))
    {
        SDL_Log("SORR iOS shell: data layout test file read failed path=%s errno=%d",
                proof_path,
                errno);
        fclose(fp);
        return 0;
    }

    fclose(fp);

    if (strcmp(readback, proof_text) != 0)
    {
        SDL_Log("SORR iOS shell: data layout test file read mismatch path=%s", proof_path);
        return 0;
    }

    SDL_Log("SORR iOS shell: data layout test file write/read ok path=%s", proof_path);
    return 1;
}

static int sorr_ios_prepare_data_layout(sorr_ios_data_layout *layout)
{
    char *base_path;
    const char *home;
    char library_dir[1024];
    char app_support_dir[1024];

    if (!layout)
    {
        return 0;
    }

    memset(layout, 0, sizeof(*layout));

    base_path = SDL_GetBasePath();
    if (!base_path)
    {
        SDL_Log("SORR iOS shell: SDL_GetBasePath failed: %s", SDL_GetError());
        return 0;
    }

    snprintf(layout->bundle_root, sizeof(layout->bundle_root), "%s", base_path);
    SDL_free(base_path);
    SDL_Log("SORR iOS shell: bundle root path=%s", layout->bundle_root);

    home = getenv("HOME");
    if (!home || !*home)
    {
        SDL_Log("SORR iOS shell: HOME environment variable is unavailable");
        return 0;
    }

    if (!sorr_ios_join_path(library_dir, sizeof(library_dir), home, "Library") ||
        !sorr_ios_join_path(app_support_dir, sizeof(app_support_dir), library_dir, "Application Support") ||
        !sorr_ios_join_path(layout->support_root, sizeof(layout->support_root), app_support_dir, "SORR") ||
        !sorr_ios_join_path(layout->savegame_dir, sizeof(layout->savegame_dir), layout->support_root, "savegame") ||
        !sorr_ios_join_path(layout->xbox_dir, sizeof(layout->xbox_dir), layout->support_root, "xbox") ||
        !sorr_ios_join_path(layout->logs_dir, sizeof(layout->logs_dir), layout->support_root, "logs"))
    {
        SDL_Log("SORR iOS shell: data layout path construction failed");
        return 0;
    }

    if (!sorr_ios_mkdir_if_needed(library_dir) ||
        !sorr_ios_mkdir_if_needed(app_support_dir) ||
        !sorr_ios_mkdir_if_needed(layout->support_root))
    {
        SDL_Log("SORR iOS shell: support root create failed path=%s errno=%d",
                layout->support_root,
                errno);
        return 0;
    }

    SDL_Log("SORR iOS shell: support root path=%s", layout->support_root);

    if (!sorr_ios_create_dir_marker("savegame", layout->savegame_dir) ||
        !sorr_ios_create_dir_marker("xbox", layout->xbox_dir) ||
        !sorr_ios_create_dir_marker("logs", layout->logs_dir))
    {
        return 0;
    }

    return sorr_ios_write_read_probe(layout->logs_dir);
}

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
    sorr_ios_data_layout data_layout;

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

    if (!sorr_ios_prepare_data_layout(&data_layout))
    {
        SDL_Log("SORR iOS shell: data layout scaffold failed");
        SDL_Quit();
        return 1;
    }

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
    SDL_Log("SORR iOS shell: SDL_CreateWindow success");

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_Log("SORR iOS shell: SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_Log("SORR iOS shell: SDL_CreateRenderer success");

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
