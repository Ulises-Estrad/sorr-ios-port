#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdarg.h>

#include "bgddl.h"
#include "dlvaracc.h"
#include "files.h"
#include "i_procdef_st.h"
#include "offsets.h"
#include "xstrings.h"

#include "SDL.h"
#include "SDL_mixer.h"

#ifdef PORTABLE_RUNTIME_DIAG
#include "portable_diag.h"
#endif

#ifndef PORTABLE_DIAG_LOG
#define PORTABLE_DIAG_LOG(tag, fmt, ...) ((void)0)
#endif

#define SOUND_FREQ 0
#define SOUND_MODE 1
#define SOUND_CHANNELS 2

extern DLVARFIXUP __bgdexport(mod_sound, globals_fixup)[];

#define SORR_IOS_AUDIO_HANDLE_MAX 4096
#define SORR_IOS_AUDIO_PATH_MAX 512
#define SORR_IOS_AUDIO_KIND_EMPTY 0
#define SORR_IOS_AUDIO_KIND_WAV 1
#define SORR_IOS_AUDIO_KIND_INERT_WAV 2
#define SORR_IOS_AUDIO_KIND_MUSIC 3
#define SORR_IOS_AUDIO_CHANNEL_LIMIT 32

typedef enum sorr_ios_audio_zero_category
{
    SORR_IOS_AUDIO_ZERO_MUSIC_PLAY = 0,
    SORR_IOS_AUDIO_ZERO_MUSIC_CONTROL,
    SORR_IOS_AUDIO_ZERO_MUSIC_QUERY,
    SORR_IOS_AUDIO_ZERO_WAV_CONTROL,
    SORR_IOS_AUDIO_ZERO_WAV_QUERY,
    SORR_IOS_AUDIO_ZERO_WAV_VOLUME,
    SORR_IOS_AUDIO_ZERO_CHANNEL_EFFECT,
    SORR_IOS_AUDIO_ZERO_PLAY_WAV_GUARD,
    SORR_IOS_AUDIO_ZERO_CATEGORY_COUNT
} sorr_ios_audio_zero_category;

typedef struct sorr_ios_audio_handle
{
    int kind;
    int retired;
    Mix_Chunk *chunk;
    Mix_Music *music;
    void *music_data;
    size_t music_data_size;
    int volume;
    unsigned int serial;
    unsigned int play_count;
    unsigned int release_count;
    char path[SORR_IOS_AUDIO_PATH_MAX];
} sorr_ios_audio_handle;

volatile unsigned int sorr_ios_sound_stub_zero_count = 0;
volatile unsigned int sorr_ios_sound_stub_minus_one_count = 0;
volatile unsigned int sorr_ios_audio_init_attempt_count = 0;
volatile unsigned int sorr_ios_audio_init_ok_count = 0;
volatile unsigned int sorr_ios_audio_init_fail_count = 0;
volatile unsigned int sorr_ios_audio_wav_load_ok_count = 0;
volatile unsigned int sorr_ios_audio_wav_load_fail_count = 0;
volatile unsigned int sorr_ios_audio_wav_play_count = 0;
volatile unsigned int sorr_ios_audio_inert_handle_count = 0;
volatile unsigned int sorr_ios_audio_queue_clear_count = 0;
volatile unsigned int sorr_ios_audio_music_load_attempt_count = 0;
volatile unsigned int sorr_ios_audio_music_open_ok_count = 0;
volatile unsigned int sorr_ios_audio_music_open_fail_count = 0;
volatile unsigned int sorr_ios_audio_live_handle_count = 0;
volatile unsigned int sorr_ios_audio_live_wav_count = 0;
volatile unsigned int sorr_ios_audio_live_inert_wav_count = 0;
volatile unsigned int sorr_ios_audio_live_music_count = 0;
volatile unsigned int sorr_ios_audio_max_live_handle_count = 0;
volatile unsigned int sorr_ios_audio_zero_music_play_count = 0;
volatile unsigned int sorr_ios_audio_zero_music_control_count = 0;
volatile unsigned int sorr_ios_audio_zero_music_query_count = 0;
volatile unsigned int sorr_ios_audio_zero_wav_control_count = 0;
volatile unsigned int sorr_ios_audio_zero_wav_query_count = 0;
volatile unsigned int sorr_ios_audio_zero_wav_volume_count = 0;
volatile unsigned int sorr_ios_audio_zero_channel_effect_count = 0;
volatile unsigned int sorr_ios_audio_zero_play_wav_guard_count = 0;
volatile unsigned int sorr_ios_audio_music_mem_load_ok_count = 0;
volatile unsigned int sorr_ios_audio_music_mem_load_fail_count = 0;
volatile unsigned int sorr_ios_audio_music_play_attempt_count = 0;
volatile unsigned int sorr_ios_audio_music_play_ok_count = 0;
volatile unsigned int sorr_ios_audio_music_play_fail_count = 0;
volatile unsigned int sorr_ios_audio_music_control_count = 0;
volatile unsigned int sorr_ios_audio_music_query_count = 0;
volatile unsigned int sorr_ios_audio_music_free_count = 0;
volatile unsigned int sorr_ios_audio_music_halt_count = 0;
volatile unsigned int sorr_ios_audio_wav_reuse_count = 0;
volatile unsigned int sorr_ios_audio_wav_release_keep_count = 0;
volatile unsigned int sorr_ios_audio_wav_retired_play_count = 0;
volatile unsigned int sorr_ios_audio_music_last_playing = 0;
volatile unsigned long long sorr_ios_audio_music_last_bytes = 0;
volatile unsigned long long sorr_ios_audio_music_total_bytes = 0;
volatile unsigned long long sorr_ios_audio_music_last_ptr = 0;
volatile unsigned long long sorr_ios_audio_music_last_handle = 0;
char sorr_ios_audio_last_music_path[SORR_IOS_AUDIO_PATH_MAX] = "";
char sorr_ios_audio_last_music_status[64] = "none";
char sorr_ios_audio_last_wav_path[SORR_IOS_AUDIO_PATH_MAX] = "";
char sorr_ios_audio_last_wav_status[64] = "none";
volatile unsigned long long sorr_ios_audio_last_wav_bytes = 0;

static int sorr_ios_audio_initialized = 0;
static int sorr_ios_audio_open_attempted = 0;
static unsigned int sorr_ios_audio_next_serial = 1;
static SDL_AudioSpec sorr_ios_audio_have;
static sorr_ios_audio_handle sorr_ios_audio_handles[SORR_IOS_AUDIO_HANDLE_MAX];
static const char *sorr_ios_audio_zero_category_names[SORR_IOS_AUDIO_ZERO_CATEGORY_COUNT] =
{
    "music_play",
    "music_control",
    "music_query",
    "wav_control",
    "wav_query",
    "wav_volume",
    "channel_effect",
    "play_wav_guard"
};

extern char sorr_ios_d3_visible_event_log_path[];

#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
extern void *portable_x64_sysproc_pointer_param(int *cell);
#define sorr_ios_audio_pointer_param(cell) portable_x64_sysproc_pointer_param(cell)
#else
#define sorr_ios_audio_pointer_param(cell) ((void *)(*(cell)))
#endif

static const char *sorr_ios_audio_kind_name(int kind)
{
    switch (kind)
    {
        case SORR_IOS_AUDIO_KIND_WAV:
            return "wav";
        case SORR_IOS_AUDIO_KIND_INERT_WAV:
            return "inert-wav";
        case SORR_IOS_AUDIO_KIND_MUSIC:
            return "music";
        case SORR_IOS_AUDIO_KIND_EMPTY:
            return "empty";
        default:
            return "unknown";
    }
}

static int sorr_ios_audio_instance_id(INSTANCE *my)
{
    if (!my || !my->locdata)
    {
        return -1;
    }
    return *(int *)((uint8_t *)my->locdata + PROCESS_ID);
}

static const char *sorr_ios_audio_instance_name(INSTANCE *my)
{
    if (!my || !my->proc || !my->proc->name)
    {
        return "(null)";
    }
    return my->proc->name;
}

static void sorr_ios_audio_diag_log(INSTANCE *my, const char *format, ...)
{
    static char diag_path[1024];
    static int diag_path_initialized = 0;
    const char *env_path;
    FILE *fp;
    va_list args;

    if (!format)
    {
        return;
    }

    if (!diag_path_initialized)
    {
        env_path = SDL_getenv("SORR_IOS_AUDIO_DIAG_PATH");
        if (env_path && env_path[0])
        {
            snprintf(diag_path, sizeof(diag_path), "%s", env_path);
        }
        diag_path_initialized = 1;
    }

    if (!diag_path[0])
    {
        return;
    }

    fp = fopen(diag_path, "ab");
    if (!fp)
    {
        return;
    }

    fprintf(fp,
            "ticks=%u proc=%s#%d ",
            SDL_GetTicks(),
            sorr_ios_audio_instance_name(my),
            sorr_ios_audio_instance_id(my));
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);
    fputc('\n', fp);
    fclose(fp);
}

static void sorr_ios_audio_runtime_log(INSTANCE *my, const char *format, ...)
{
    FILE *fp;
    va_list args;

    if (!format || !sorr_ios_d3_visible_event_log_path[0])
    {
        return;
    }

    fp = fopen(sorr_ios_d3_visible_event_log_path, "ab");
    if (!fp)
    {
        return;
    }

    fprintf(fp,
            "audio_runtime ticks=%u proc=%s#%d ",
            SDL_GetTicks(),
            sorr_ios_audio_instance_name(my),
            sorr_ios_audio_instance_id(my));
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);
    fputc('\n', fp);
    fclose(fp);
}

static void sorr_ios_audio_note_wav_path(const char *path, const char *status, size_t bytes)
{
    snprintf(sorr_ios_audio_last_wav_path,
             sizeof(sorr_ios_audio_last_wav_path),
             "%s",
             path ? path : "");
    snprintf(sorr_ios_audio_last_wav_status,
             sizeof(sorr_ios_audio_last_wav_status),
             "%s",
             status ? status : "unknown");
    sorr_ios_audio_last_wav_bytes = (unsigned long long)bytes;
}

static void sorr_ios_audio_update_max_live(void)
{
    if (sorr_ios_audio_live_handle_count > sorr_ios_audio_max_live_handle_count)
    {
        sorr_ios_audio_max_live_handle_count = sorr_ios_audio_live_handle_count;
    }
}

static void sorr_ios_audio_note_handle_store(int kind)
{
    sorr_ios_audio_live_handle_count++;
    if (kind == SORR_IOS_AUDIO_KIND_WAV)
    {
        sorr_ios_audio_live_wav_count++;
    }
    else if (kind == SORR_IOS_AUDIO_KIND_INERT_WAV)
    {
        sorr_ios_audio_live_inert_wav_count++;
    }
    else if (kind == SORR_IOS_AUDIO_KIND_MUSIC)
    {
        sorr_ios_audio_live_music_count++;
    }
    sorr_ios_audio_update_max_live();
}

static void sorr_ios_audio_note_handle_release(int kind)
{
    if (sorr_ios_audio_live_handle_count > 0)
    {
        sorr_ios_audio_live_handle_count--;
    }

    if (kind == SORR_IOS_AUDIO_KIND_WAV && sorr_ios_audio_live_wav_count > 0)
    {
        sorr_ios_audio_live_wav_count--;
    }
    else if (kind == SORR_IOS_AUDIO_KIND_INERT_WAV && sorr_ios_audio_live_inert_wav_count > 0)
    {
        sorr_ios_audio_live_inert_wav_count--;
    }
    else if (kind == SORR_IOS_AUDIO_KIND_MUSIC && sorr_ios_audio_live_music_count > 0)
    {
        sorr_ios_audio_live_music_count--;
    }
}

static void sorr_ios_audio_note_music_path(const char *path, const char *status)
{
    snprintf(sorr_ios_audio_last_music_path,
             sizeof(sorr_ios_audio_last_music_path),
             "%s",
             path ? path : "(null)");
    snprintf(sorr_ios_audio_last_music_status,
             sizeof(sorr_ios_audio_last_music_status),
             "%s",
             status ? status : "(null)");
}

static const char *sorr_ios_audio_music_type_name(Mix_MusicType type)
{
    switch (type)
    {
        case MUS_NONE:
            return "none";
        case MUS_CMD:
            return "cmd";
        case MUS_WAV:
            return "wav";
        case MUS_MOD:
            return "mod";
        case MUS_MID:
            return "mid";
        case MUS_OGG:
            return "ogg";
        case MUS_MP3:
            return "mp3";
        case MUS_FLAC:
            return "flac";
        default:
            return "unknown";
    }
}

static int sorr_ios_audio_should_log_count(unsigned int count)
{
    return count <= 8 || count == 16 || count == 32 || count == 64 ||
           count == 128 || count == 256 || count == 512 ||
           (count % 1000u) == 0u;
}

static int sorr_ios_sound_zero_named(const char *op, sorr_ios_audio_zero_category category)
{
    unsigned int category_count = 0;

    sorr_ios_sound_stub_zero_count++;

    switch (category)
    {
        case SORR_IOS_AUDIO_ZERO_MUSIC_PLAY:
            category_count = ++sorr_ios_audio_zero_music_play_count;
            break;
        case SORR_IOS_AUDIO_ZERO_MUSIC_CONTROL:
            category_count = ++sorr_ios_audio_zero_music_control_count;
            break;
        case SORR_IOS_AUDIO_ZERO_MUSIC_QUERY:
            category_count = ++sorr_ios_audio_zero_music_query_count;
            break;
        case SORR_IOS_AUDIO_ZERO_WAV_CONTROL:
            category_count = ++sorr_ios_audio_zero_wav_control_count;
            break;
        case SORR_IOS_AUDIO_ZERO_WAV_QUERY:
            category_count = ++sorr_ios_audio_zero_wav_query_count;
            break;
        case SORR_IOS_AUDIO_ZERO_WAV_VOLUME:
            category_count = ++sorr_ios_audio_zero_wav_volume_count;
            break;
        case SORR_IOS_AUDIO_ZERO_CHANNEL_EFFECT:
            category_count = ++sorr_ios_audio_zero_channel_effect_count;
            break;
        case SORR_IOS_AUDIO_ZERO_PLAY_WAV_GUARD:
            category_count = ++sorr_ios_audio_zero_play_wav_guard_count;
            break;
        default:
            category_count = sorr_ios_sound_stub_zero_count;
            break;
    }

    if (sorr_ios_audio_should_log_count(category_count))
    {
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A zero op=%s category=%s category_count=%u total=%u",
                          op ? op : "(null)",
                          category >= 0 && category < SORR_IOS_AUDIO_ZERO_CATEGORY_COUNT ?
                              sorr_ios_audio_zero_category_names[category] : "unknown",
                          category_count,
                          sorr_ios_sound_stub_zero_count);
    }

    return 0;
}

static Sint64 SDLCALL sorr_ios_audio_seek_cb(SDL_RWops *context, Sint64 offset, int whence)
{
    file *fp = (file *)context->hidden.unknown.data1;

    if (!fp || offset > INT_MAX || offset < INT_MIN)
    {
        return -1;
    }

    if (file_seek(fp, (int)offset, whence) < 0)
    {
        return -1;
    }

    return (Sint64)file_pos(fp);
}

static size_t SDLCALL sorr_ios_audio_read_cb(SDL_RWops *context, void *ptr, size_t size, size_t maxnum)
{
    file *fp = (file *)context->hidden.unknown.data1;
    int requested;
    int read_count;

    if (!fp || !ptr || size == 0 || maxnum == 0)
    {
        return 0;
    }

    if (size > (size_t)INT_MAX / maxnum)
    {
        requested = INT_MAX;
    }
    else
    {
        requested = (int)(size * maxnum);
    }

    read_count = file_read(fp, ptr, requested);
    if (read_count <= 0)
    {
        return 0;
    }

    return (size_t)read_count / size;
}

static size_t SDLCALL sorr_ios_audio_write_cb(SDL_RWops *context, const void *ptr, size_t size, size_t num)
{
    (void)context;
    (void)ptr;
    (void)size;
    (void)num;
    return 0;
}

static int SDLCALL sorr_ios_audio_close_cb(SDL_RWops *context)
{
    if (context)
    {
        if (context->hidden.unknown.data1)
        {
            file_close((file *)context->hidden.unknown.data1);
        }
        SDL_FreeRW(context);
    }
    return 0;
}

static SDL_RWops *sorr_ios_audio_rw_from_file(file *fp)
{
    SDL_RWops *rwops;

    if (!fp)
    {
        return NULL;
    }

    rwops = SDL_AllocRW();
    if (!rwops)
    {
        return NULL;
    }

    rwops->seek = sorr_ios_audio_seek_cb;
    rwops->read = sorr_ios_audio_read_cb;
    rwops->write = sorr_ios_audio_write_cb;
    rwops->close = sorr_ios_audio_close_cb;
    rwops->hidden.unknown.data1 = fp;
    return rwops;
}

static int sorr_ios_audio_same_path(const char *a, const char *b)
{
    if (!a || !b)
    {
        return 0;
    }
    return strcmp(a, b) == 0;
}

static int sorr_ios_audio_find_path(const char *path, int kind)
{
    int i;

    if (!path || !path[0])
    {
        return 0;
    }

    for (i = 1; i < SORR_IOS_AUDIO_HANDLE_MAX; i++)
    {
        if (sorr_ios_audio_handles[i].kind == kind &&
            sorr_ios_audio_same_path(sorr_ios_audio_handles[i].path, path))
        {
            return i;
        }
    }

    return 0;
}

static int sorr_ios_audio_store_handle(int kind,
                                       const char *path,
                                       Mix_Chunk *chunk,
                                       Mix_Music *music,
                                       void *music_data,
                                       size_t music_data_size)
{
    int i;

    for (i = 1; i < SORR_IOS_AUDIO_HANDLE_MAX; i++)
    {
        if (sorr_ios_audio_handles[i].kind == SORR_IOS_AUDIO_KIND_EMPTY)
        {
            sorr_ios_audio_handles[i].kind = kind;
            sorr_ios_audio_handles[i].chunk = chunk;
            sorr_ios_audio_handles[i].music = music;
            sorr_ios_audio_handles[i].music_data = music_data;
            sorr_ios_audio_handles[i].music_data_size = music_data_size;
            sorr_ios_audio_handles[i].volume = SDL_MIX_MAXVOLUME;
            sorr_ios_audio_handles[i].retired = 0;
            sorr_ios_audio_handles[i].serial = sorr_ios_audio_next_serial++;
            if (sorr_ios_audio_next_serial == 0)
            {
                sorr_ios_audio_next_serial = 1;
            }
            sorr_ios_audio_handles[i].play_count = 0;
            sorr_ios_audio_handles[i].release_count = 0;
            if (path)
            {
                snprintf(sorr_ios_audio_handles[i].path,
                         sizeof(sorr_ios_audio_handles[i].path),
                         "%s",
                         path);
            }
            else
            {
                sorr_ios_audio_handles[i].path[0] = '\0';
            }
            sorr_ios_audio_note_handle_store(kind);

            if (kind == SORR_IOS_AUDIO_KIND_INERT_WAV)
            {
                sorr_ios_audio_inert_handle_count++;
            }

            PORTABLE_DIAG_LOG("AUDIO",
                              "iOS D3A audio handle store kind=%d handle=%d path=%s chunk=%p music=%p",
                              kind,
                              i,
                              path ? path : "(null)",
                              (void *)chunk,
                              (void *)music);
            if (kind == SORR_IOS_AUDIO_KIND_MUSIC)
            {
                sorr_ios_audio_music_last_bytes = (unsigned long long)music_data_size;
                sorr_ios_audio_music_last_ptr = (unsigned long long)(uintptr_t)music;
                sorr_ios_audio_music_last_handle = (unsigned long long)i;
                PORTABLE_DIAG_LOG("AUDIO",
                                  "iOS D3A music handle store handle=%d music=%p bytes=%llu type=%s path=%s",
                                  i,
                                  (void *)music,
                                  (unsigned long long)music_data_size,
                                  music ? sorr_ios_audio_music_type_name(Mix_GetMusicType(music)) : "none",
                                  path ? path : "(null)");
            }
            return i;
        }
    }

    if (chunk)
    {
        Mix_FreeChunk(chunk);
    }
    if (music)
    {
        Mix_FreeMusic(music);
    }
    free(music_data);

    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A audio handle table full path=%s", path ? path : "(null)");
    return 0;
}

static void sorr_ios_audio_release_handle_ex(int handle, INSTANCE *my, const char *op, int force)
{
    int kind;
    sorr_ios_audio_handle *entry;

    if (handle <= 0 || handle >= SORR_IOS_AUDIO_HANDLE_MAX)
    {
        return;
    }

    kind = sorr_ios_audio_handles[handle].kind;
    if (kind == SORR_IOS_AUDIO_KIND_EMPTY)
    {
        return;
    }

    entry = &sorr_ios_audio_handles[handle];
    entry->release_count++;

    if (!force && (kind == SORR_IOS_AUDIO_KIND_WAV || kind == SORR_IOS_AUDIO_KIND_INERT_WAV))
    {
        entry->retired = 1;
        sorr_ios_audio_wav_release_keep_count++;
        sorr_ios_audio_diag_log(my,
                                "event=UNLOAD_WAV_KEEP handle=%d serial=%u kind=%s releases=%u plays=%u path=%s op=%s",
                                handle,
                                entry->serial,
                                sorr_ios_audio_kind_name(kind),
                                entry->release_count,
                                entry->play_count,
                                entry->path[0] ? entry->path : "(none)",
                                op ? op : "(null)");
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A UNLOAD_WAV keep handle=%d serial=%u kind=%s path=%s op=%s",
                          handle,
                          entry->serial,
                          sorr_ios_audio_kind_name(kind),
                          entry->path,
                          op ? op : "(null)");
        return;
    }

    sorr_ios_audio_diag_log(my,
                            "event=RELEASE_HANDLE handle=%d serial=%u kind=%s force=%d releases=%u plays=%u path=%s op=%s",
                            handle,
                            entry->serial,
                            sorr_ios_audio_kind_name(kind),
                            force,
                            entry->release_count,
                            entry->play_count,
                            entry->path[0] ? entry->path : "(none)",
                            op ? op : "(null)");

    if (entry->chunk)
    {
        Mix_FreeChunk(entry->chunk);
    }
    if (entry->music)
    {
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A music release begin handle=%d music=%p playing=%d path=%s",
                          handle,
                          (void *)entry->music,
                          sorr_ios_audio_initialized ? Mix_PlayingMusic() : 0,
                          entry->path);
        if (Mix_PlayingMusic())
        {
            int halt_result = Mix_HaltMusic();
            sorr_ios_audio_music_halt_count++;
            PORTABLE_DIAG_LOG("AUDIO",
                              "iOS D3A music release halt handle=%d result=%d error=%s",
                              handle,
                              halt_result,
                              Mix_GetError());
        }
        Mix_FreeMusic(entry->music);
        sorr_ios_audio_music_free_count++;
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A music release free handle=%d bytes=%llu",
                          handle,
                          (unsigned long long)entry->music_data_size);
    }
    if (entry->music_data)
    {
        free(entry->music_data);
    }

    sorr_ios_audio_note_handle_release(kind);
    memset(&sorr_ios_audio_handles[handle], 0, sizeof(sorr_ios_audio_handles[handle]));
}

static void sorr_ios_audio_release_handle(int handle)
{
    sorr_ios_audio_release_handle_ex(handle, NULL, "release", 0);
}

static int sorr_ios_audio_inert_handle(const char *path, int kind)
{
    int existing = sorr_ios_audio_find_path(path, kind);

    if (existing)
    {
        return existing;
    }

    return sorr_ios_audio_store_handle(kind, path, NULL, NULL, NULL, 0);
}

static int sorr_ios_audio_read_file_to_memory_ex(const char *filename,
                                                 void **out_data,
                                                 size_t *out_size,
                                                 const char *kind,
                                                 int update_music_stats)
{
    file *fp;
    int size;
    int total = 0;
    unsigned char *data;

    if (!filename || !out_data || !out_size)
    {
        return 0;
    }

    *out_data = NULL;
    *out_size = 0;

    fp = file_open(filename, "rb0");
    if (!fp)
    {
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A %s memory open failed path=%s", kind ? kind : "audio", filename);
        return 0;
    }

    size = file_size(fp);
    if (size <= 0)
    {
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A %s memory size invalid path=%s size=%d", kind ? kind : "audio", filename, size);
        file_close(fp);
        return 0;
    }

    data = (unsigned char *)malloc((size_t)size);
    if (!data)
    {
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A %s memory malloc failed path=%s size=%d", kind ? kind : "audio", filename, size);
        file_close(fp);
        return 0;
    }

    while (total < size)
    {
        int got = file_read(fp, data + total, size - total);
        if (got <= 0)
        {
            break;
        }
        total += got;
    }

    file_close(fp);

    if (total != size)
    {
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A %s memory read short path=%s expected=%d got=%d",
                          kind ? kind : "audio",
                          filename,
                          size,
                          total);
        free(data);
        return 0;
    }

    *out_data = data;
    *out_size = (size_t)size;
    if (update_music_stats)
    {
        sorr_ios_audio_music_last_bytes = (unsigned long long)*out_size;
        sorr_ios_audio_music_total_bytes += (unsigned long long)*out_size;
    }
    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A %s memory read ok path=%s bytes=%llu",
                      kind ? kind : "audio",
                      filename,
                      (unsigned long long)*out_size);
    return 1;
}

static int sorr_ios_audio_read_file_to_memory(const char *filename, void **out_data, size_t *out_size)
{
    return sorr_ios_audio_read_file_to_memory_ex(filename, out_data, out_size, "music", 1);
}

static int sorr_ios_audio_init_device(void)
{
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16SYS;
    int audio_channels = 2;
    int audio_buffers = 1024;
    int mix_channels = 8;
    int mix_init_flags;

    if (sorr_ios_audio_initialized)
    {
        return 0;
    }

    if (sorr_ios_audio_open_attempted && !sorr_ios_audio_initialized)
    {
        return 0;
    }

    sorr_ios_audio_open_attempted = 1;
    sorr_ios_audio_init_attempt_count++;

    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        {
            sorr_ios_audio_init_fail_count++;
            PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SDL_InitSubSystem audio failed error=%s", SDL_GetError());
            return 0;
        }
    }

    if (GLOEXISTS(mod_sound, SOUND_FREQ))
    {
        audio_rate = (int)GLODWORD(mod_sound, SOUND_FREQ);
    }

    if (audio_rate > 22050)
    {
        audio_rate = 44100;
    }
    else if (audio_rate > 11025)
    {
        audio_rate = 22050;
    }
    else
    {
        audio_rate = 11025;
    }

    if (GLOEXISTS(mod_sound, SOUND_MODE))
    {
        audio_channels = (int)GLODWORD(mod_sound, SOUND_MODE) + 1;
    }
    if (audio_channels < 1 || audio_channels > 2)
    {
        audio_channels = 2;
    }

    audio_buffers = 1024 * audio_rate / 22050;
    if (audio_buffers < 512)
    {
        audio_buffers = 512;
    }

    mix_init_flags = Mix_Init(MIX_INIT_OGG);
    if ((mix_init_flags & MIX_INIT_OGG) == 0)
    {
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A Mix_Init OGG unavailable flags=%d error=%s",
                          mix_init_flags,
                          Mix_GetError());
    }

    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A Mix_OpenAudio begin driver=%s freq=%d format=%u channels=%d buffers=%d",
                      SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "(none)",
                      audio_rate,
                      (unsigned int)audio_format,
                      audio_channels,
                      audio_buffers);

    if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) < 0)
    {
        sorr_ios_audio_init_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A Mix_OpenAudio failed error=%s", Mix_GetError());
        Mix_Quit();
        return 0;
    }

    if (GLOEXISTS(mod_sound, SOUND_CHANNELS))
    {
        mix_channels = (int)GLODWORD(mod_sound, SOUND_CHANNELS);
    }
    if (mix_channels < 1)
    {
        mix_channels = 8;
    }
    if (mix_channels > SORR_IOS_AUDIO_CHANNEL_LIMIT)
    {
        mix_channels = SORR_IOS_AUDIO_CHANNEL_LIMIT;
    }
    Mix_AllocateChannels(mix_channels);
    Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);

    SDL_zero(sorr_ios_audio_have);
    sorr_ios_audio_have.freq = audio_rate;
    sorr_ios_audio_have.format = audio_format;
    sorr_ios_audio_have.channels = (Uint8)audio_channels;
    sorr_ios_audio_have.samples = (Uint16)audio_buffers;

    sorr_ios_audio_initialized = 1;
    sorr_ios_audio_init_ok_count++;
    if (GLOEXISTS(mod_sound, SOUND_CHANNELS))
    {
        GLODWORD(mod_sound, SOUND_CHANNELS) = Mix_AllocateChannels(-1);
    }

    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A Mix_OpenAudio ok driver=%s freq=%d format=%u channels=%d mix_channels=%d ogg_flags=%d",
                      SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "(none)",
                      audio_rate,
                      (unsigned int)audio_format,
                      audio_channels,
                      Mix_AllocateChannels(-1),
                      mix_init_flags);
    return 0;
}

static void sorr_ios_audio_close_device(void)
{
    int i;

    for (i = 1; i < SORR_IOS_AUDIO_HANDLE_MAX; i++)
    {
        sorr_ios_audio_release_handle_ex(i, NULL, "audio-close", 1);
    }

    if (sorr_ios_audio_initialized)
    {
        Mix_CloseAudio();
        Mix_Quit();
    }

    sorr_ios_audio_initialized = 0;
    sorr_ios_audio_open_attempted = 0;
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SDL_mixer audio closed");
}

static int sorr_ios_audio_load_wav_path(INSTANCE *my, const char *filename)
{
    int existing;
    void *wav_data = NULL;
    size_t wav_data_size = 0;
    SDL_RWops *rwops;
    Mix_Chunk *chunk;

    if (!filename || !filename[0])
    {
        sorr_ios_audio_note_wav_path(filename, "empty-path", 0);
        return 0;
    }

    sorr_ios_audio_note_wav_path(filename, "enter", 0);
    sorr_ios_audio_runtime_log(my,
                               "event=LOAD_WAV_ENTER path=%s live_handles=%u live_wav=%u wav_ok=%u wav_fail=%u",
                               filename,
                               sorr_ios_audio_live_handle_count,
                               sorr_ios_audio_live_wav_count,
                               sorr_ios_audio_wav_load_ok_count,
                               sorr_ios_audio_wav_load_fail_count);

    existing = sorr_ios_audio_find_path(filename, SORR_IOS_AUDIO_KIND_WAV);
    if (existing)
    {
        int retired_before = sorr_ios_audio_handles[existing].retired;
        sorr_ios_audio_handles[existing].retired = 0;
        sorr_ios_audio_wav_reuse_count++;
        sorr_ios_audio_diag_log(my,
                                "event=LOAD_WAV_REUSE handle=%d serial=%u kind=wav retired_before=%d reuse_count=%u path=%s",
                                existing,
                                sorr_ios_audio_handles[existing].serial,
                                retired_before,
                                sorr_ios_audio_wav_reuse_count,
                                filename);
        sorr_ios_audio_note_wav_path(filename, "reuse-wav", 0);
        sorr_ios_audio_runtime_log(my,
                                   "event=LOAD_WAV_REUSE handle=%d kind=wav path=%s",
                                   existing,
                                   filename);
        return existing;
    }
    existing = sorr_ios_audio_find_path(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    if (existing)
    {
        int retired_before = sorr_ios_audio_handles[existing].retired;
        sorr_ios_audio_handles[existing].retired = 0;
        sorr_ios_audio_wav_reuse_count++;
        sorr_ios_audio_diag_log(my,
                                "event=LOAD_WAV_REUSE handle=%d serial=%u kind=inert-wav retired_before=%d reuse_count=%u path=%s",
                                existing,
                                sorr_ios_audio_handles[existing].serial,
                                retired_before,
                                sorr_ios_audio_wav_reuse_count,
                                filename);
        sorr_ios_audio_note_wav_path(filename, "reuse-inert-wav", 0);
        sorr_ios_audio_runtime_log(my,
                                   "event=LOAD_WAV_REUSE handle=%d kind=inert-wav path=%s",
                                   existing,
                                   filename);
        return existing;
    }

    sorr_ios_audio_init_device();
    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_WAV mixer unavailable path=%s", filename);
        sorr_ios_audio_diag_log(my, "event=LOAD_WAV_FAIL reason=mixer-unavailable path=%s", filename);
        sorr_ios_audio_note_wav_path(filename, "fail-mixer-unavailable", 0);
        sorr_ios_audio_runtime_log(my, "event=LOAD_WAV_FAIL reason=mixer-unavailable path=%s", filename);
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    if (!sorr_ios_audio_read_file_to_memory_ex(filename, &wav_data, &wav_data_size, "wav", 0))
    {
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_WAV memory read failed path=%s", filename);
        sorr_ios_audio_diag_log(my, "event=LOAD_WAV_FAIL reason=memory-read path=%s", filename);
        sorr_ios_audio_note_wav_path(filename, "fail-memory-read", 0);
        sorr_ios_audio_runtime_log(my, "event=LOAD_WAV_FAIL reason=memory-read path=%s", filename);
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    if (wav_data_size > (size_t)INT_MAX)
    {
        free(wav_data);
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_WAV too large path=%s bytes=%llu", filename, (unsigned long long)wav_data_size);
        sorr_ios_audio_diag_log(my, "event=LOAD_WAV_FAIL reason=too-large bytes=%llu path=%s", (unsigned long long)wav_data_size, filename);
        sorr_ios_audio_note_wav_path(filename, "fail-too-large", wav_data_size);
        sorr_ios_audio_runtime_log(my, "event=LOAD_WAV_FAIL reason=too-large bytes=%llu path=%s", (unsigned long long)wav_data_size, filename);
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    sorr_ios_audio_note_wav_path(filename, "memory-read-ok", wav_data_size);
    sorr_ios_audio_runtime_log(my,
                               "event=LOAD_WAV_MEMORY_OK bytes=%llu path=%s",
                               (unsigned long long)wav_data_size,
                               filename);

    rwops = SDL_RWFromConstMem(wav_data, (int)wav_data_size);
    if (!rwops)
    {
        free(wav_data);
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_WAV SDL_RWops failed path=%s", filename);
        sorr_ios_audio_diag_log(my, "event=LOAD_WAV_FAIL reason=rwops path=%s", filename);
        sorr_ios_audio_note_wav_path(filename, "fail-rwops", wav_data_size);
        sorr_ios_audio_runtime_log(my,
                                   "event=LOAD_WAV_FAIL reason=rwops bytes=%llu path=%s",
                                   (unsigned long long)wav_data_size,
                                   filename);
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    chunk = Mix_LoadWAV_RW(rwops, 1);
    free(wav_data);
    if (!chunk)
    {
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A LOAD_WAV Mix_LoadWAV_RW failed path=%s error=%s",
                          filename,
                          Mix_GetError());
        sorr_ios_audio_diag_log(my,
                                "event=LOAD_WAV_FAIL reason=decode path=%s error=%s",
                                filename,
                                Mix_GetError());
        sorr_ios_audio_note_wav_path(filename, "fail-decode", wav_data_size);
        sorr_ios_audio_runtime_log(my,
                                   "event=LOAD_WAV_FAIL reason=decode bytes=%llu path=%s error=%s",
                                   (unsigned long long)wav_data_size,
                                   filename,
                                   Mix_GetError());
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    existing = sorr_ios_audio_store_handle(SORR_IOS_AUDIO_KIND_WAV, filename, chunk, NULL, NULL, 0);
    if (existing)
    {
        sorr_ios_audio_wav_load_ok_count++;
        sorr_ios_audio_diag_log(my,
                                "event=LOAD_WAV_OK handle=%d serial=%u chunk=%p path=%s live_wav=%u max_live=%u",
                                existing,
                                sorr_ios_audio_handles[existing].serial,
                                (void *)sorr_ios_audio_handles[existing].chunk,
                                filename,
                                sorr_ios_audio_live_wav_count,
                                sorr_ios_audio_max_live_handle_count);
        sorr_ios_audio_note_wav_path(filename, "ok", wav_data_size);
        sorr_ios_audio_runtime_log(my,
                                   "event=LOAD_WAV_OK handle=%d serial=%u bytes=%llu chunk=%p path=%s live_wav=%u max_live=%u",
                                   existing,
                                   sorr_ios_audio_handles[existing].serial,
                                   (unsigned long long)wav_data_size,
                                   (void *)sorr_ios_audio_handles[existing].chunk,
                                   filename,
                                   sorr_ios_audio_live_wav_count,
                                   sorr_ios_audio_max_live_handle_count);
    }
    else
    {
        Mix_FreeChunk(chunk);
        sorr_ios_audio_wav_load_fail_count++;
        sorr_ios_audio_note_wav_path(filename, "fail-handle-table", wav_data_size);
        sorr_ios_audio_runtime_log(my,
                                   "event=LOAD_WAV_FAIL reason=handle-table bytes=%llu path=%s",
                                   (unsigned long long)wav_data_size,
                                   filename);
    }
    return existing;
}

static int sorr_ios_audio_load_song_path(const char *filename)
{
    int existing;
    SDL_RWops *rwops;
    Mix_Music *music;
    void *music_data = NULL;
    size_t music_data_size = 0;

    if (!filename || !filename[0])
    {
        return 0;
    }

    existing = sorr_ios_audio_find_path(filename, SORR_IOS_AUDIO_KIND_MUSIC);
    if (existing)
    {
        return existing;
    }

    sorr_ios_audio_music_load_attempt_count++;
    sorr_ios_audio_init_device();
    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_music_open_fail_count++;
        sorr_ios_audio_note_music_path(filename, "mixer-init-fail");
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_SONG mixer unavailable path=%s", filename);
        return 0;
    }

    sorr_ios_audio_note_music_path(filename, "mem-load-start");
    if (!sorr_ios_audio_read_file_to_memory(filename, &music_data, &music_data_size))
    {
        sorr_ios_audio_music_mem_load_fail_count++;
        sorr_ios_audio_music_open_fail_count++;
        sorr_ios_audio_note_music_path(filename, "mem-load-fail");
        return 0;
    }
    sorr_ios_audio_music_mem_load_ok_count++;

    rwops = SDL_RWFromConstMem(music_data, (int)music_data_size);
    if (!rwops)
    {
        free(music_data);
        sorr_ios_audio_music_open_fail_count++;
        sorr_ios_audio_note_music_path(filename, "mem-rwops-fail");
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_SONG SDL_RWFromConstMem failed path=%s bytes=%llu error=%s",
                          filename,
                          (unsigned long long)music_data_size,
                          SDL_GetError());
        return 0;
    }

    sorr_ios_audio_note_music_path(filename, "mix-load-start");
    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A LOAD_SONG Mix_LoadMUS_RW begin path=%s bytes=%llu rwops=%p",
                      filename,
                      (unsigned long long)music_data_size,
                      (void *)rwops);
    music = Mix_LoadMUS_RW(rwops, 1);
    if (!music)
    {
        free(music_data);
        sorr_ios_audio_music_open_fail_count++;
        sorr_ios_audio_note_music_path(filename, "decode-fail");
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A LOAD_SONG Mix_LoadMUS_RW failed path=%s error=%s",
                          filename,
                          Mix_GetError());
        return 0;
    }

    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A LOAD_SONG Mix_LoadMUS_RW ok path=%s music=%p type=%s bytes=%llu",
                      filename,
                      (void *)music,
                      sorr_ios_audio_music_type_name(Mix_GetMusicType(music)),
                      (unsigned long long)music_data_size);

    existing = sorr_ios_audio_store_handle(SORR_IOS_AUDIO_KIND_MUSIC,
                                           filename,
                                           NULL,
                                           music,
                                           music_data,
                                           music_data_size);
    if (existing)
    {
        sorr_ios_audio_music_open_ok_count++;
        sorr_ios_audio_note_music_path(filename, "load-ok");
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_SONG ok handle=%d path=%s", existing, filename);
        return existing;
    }

    sorr_ios_audio_music_open_fail_count++;
    sorr_ios_audio_note_music_path(filename, "handle-table-full");
    return 0;
}

static int sorr_ios_audio_play_wav_handle(INSTANCE *my, int handle, int loops, int channel)
{
    sorr_ios_audio_handle *entry;
    int retired_before;
    int result;

    if (handle <= 0 || handle >= SORR_IOS_AUDIO_HANDLE_MAX)
    {
        sorr_ios_sound_zero_named("PLAY_WAV invalid-handle", SORR_IOS_AUDIO_ZERO_PLAY_WAV_GUARD);
        sorr_ios_audio_diag_log(my,
                                "event=PLAY_WAV_GUARD reason=invalid-handle handle=%d loops=%d channel_arg=%d",
                                handle,
                                loops,
                                channel);
        return 0;
    }

    entry = &sorr_ios_audio_handles[handle];
    if (entry->kind == SORR_IOS_AUDIO_KIND_INERT_WAV)
    {
        sorr_ios_sound_zero_named("PLAY_WAV inert-handle", SORR_IOS_AUDIO_ZERO_PLAY_WAV_GUARD);
        sorr_ios_audio_diag_log(my,
                                "event=PLAY_WAV_GUARD reason=inert-handle handle=%d serial=%u retired=%d loops=%d channel_arg=%d path=%s",
                                handle,
                                entry->serial,
                                entry->retired,
                                loops,
                                channel,
                                entry->path[0] ? entry->path : "(none)");
        return 0;
    }

    if (entry->kind != SORR_IOS_AUDIO_KIND_WAV || !entry->chunk)
    {
        sorr_ios_sound_zero_named("PLAY_WAV missing-data", SORR_IOS_AUDIO_ZERO_PLAY_WAV_GUARD);
        sorr_ios_audio_diag_log(my,
                                "event=PLAY_WAV_GUARD reason=missing-data handle=%d serial=%u kind=%s retired=%d loops=%d channel_arg=%d path=%s",
                                handle,
                                entry->serial,
                                sorr_ios_audio_kind_name(entry->kind),
                                entry->retired,
                                loops,
                                channel,
                                entry->path[0] ? entry->path : "(none)");
        return 0;
    }

    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_init_device();
    }
    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_sound_zero_named("PLAY_WAV no-mixer", SORR_IOS_AUDIO_ZERO_PLAY_WAV_GUARD);
        sorr_ios_audio_diag_log(my,
                                "event=PLAY_WAV_GUARD reason=no-mixer handle=%d serial=%u loops=%d channel_arg=%d path=%s",
                                handle,
                                entry->serial,
                                loops,
                                channel,
                                entry->path[0] ? entry->path : "(none)");
        return 0;
    }

    retired_before = entry->retired;
    if (retired_before)
    {
        entry->retired = 0;
        sorr_ios_audio_wav_retired_play_count++;
        sorr_ios_audio_diag_log(my,
                                "event=PLAY_WAV_RETIRED_HANDLE handle=%d serial=%u retired_play_count=%u loops=%d channel_arg=%d path=%s",
                                handle,
                                entry->serial,
                                sorr_ios_audio_wav_retired_play_count,
                                loops,
                                channel,
                                entry->path[0] ? entry->path : "(none)");
    }

    result = Mix_PlayChannel(channel, entry->chunk, loops);
    if (result < 0)
    {
        sorr_ios_sound_zero_named("PLAY_WAV mix-failed", SORR_IOS_AUDIO_ZERO_PLAY_WAV_GUARD);
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A Mix_PlayChannel failed handle=%d error=%s", handle, Mix_GetError());
        sorr_ios_audio_diag_log(my,
                                "event=PLAY_WAV_FAIL reason=mix-failed handle=%d serial=%u retired_before=%d loops=%d channel_arg=%d path=%s error=%s",
                                handle,
                                entry->serial,
                                retired_before,
                                loops,
                                channel,
                                entry->path[0] ? entry->path : "(none)",
                                Mix_GetError());
        return 0;
    }

    sorr_ios_audio_wav_play_count++;
    entry->play_count++;
    sorr_ios_audio_diag_log(my,
                            "event=PLAY_WAV handle=%d serial=%u kind=%s retired_before=%d plays=%u channel_arg=%d channel_result=%d loops=%d path=%s wav_play_total=%u reuse_count=%u keep_count=%u",
                            handle,
                            entry->serial,
                            sorr_ios_audio_kind_name(entry->kind),
                            retired_before,
                            entry->play_count,
                            channel,
                            result,
                            loops,
                            entry->path[0] ? entry->path : "(none)",
                            sorr_ios_audio_wav_play_count,
                            sorr_ios_audio_wav_reuse_count,
                            sorr_ios_audio_wav_release_keep_count);
    return result;
}

static int sorr_ios_sound_init(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    return sorr_ios_audio_init_device();
}

static int sorr_ios_sound_close(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    sorr_ios_audio_close_device();
    return 0;
}

static int sorr_ios_sound_load_wav(INSTANCE *my, int *params)
{
    const char *filename;
    int handle;

    filename = string_get(params[0]);
    if (!filename)
    {
        return 0;
    }

    handle = sorr_ios_audio_load_wav_path(my, filename);
    string_discard(params[0]);
    return handle;
}

static int sorr_ios_sound_bgload_wav(INSTANCE *my, int *params)
{
    const char *filename;
    int handle;
    int *target;

    filename = string_get(params[0]);
    if (!filename)
    {
        return 0;
    }

    handle = sorr_ios_audio_load_wav_path(my, filename);
    target = (int *)sorr_ios_audio_pointer_param(&params[1]);
    if (target)
    {
        *target = handle;
    }
    else
    {
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_WAV SP target pointer missing path=%s", filename);
    }
    string_discard(params[0]);
    return 0;
}

static int sorr_ios_sound_unload_wav(INSTANCE *my, int *params)
{
    sorr_ios_audio_release_handle_ex(params[0], my, "UNLOAD_WAV", 0);
    return 0;
}

static int sorr_ios_sound_unload_wav_ptr(INSTANCE *my, int *params)
{
    int *target;

    target = (int *)sorr_ios_audio_pointer_param(&params[0]);
    if (target)
    {
        sorr_ios_audio_release_handle_ex(*target, my, "UNLOAD_WAV_PTR", 0);
        *target = 0;
    }
    return 0;
}

static int sorr_ios_sound_play_wav(INSTANCE *my, int *params)
{
    return sorr_ios_audio_play_wav_handle(my, params[0], params[1], -1);
}

static int sorr_ios_sound_play_wav_channel(INSTANCE *my, int *params)
{
    return sorr_ios_audio_play_wav_handle(my, params[0], params[1], params[2]);
}

static int sorr_ios_sound_load_song(INSTANCE *my, int *params)
{
    const char *filename;
    int handle;

    (void)my;
    filename = string_get(params[0]);
    if (!filename)
    {
        return 0;
    }

    handle = sorr_ios_audio_load_song_path(filename);
    string_discard(params[0]);
    return handle;
}

static int sorr_ios_sound_bgload_song(INSTANCE *my, int *params)
{
    const char *filename;
    int handle;
    int *target;

    (void)my;
    filename = string_get(params[0]);
    if (!filename)
    {
        return 0;
    }

    handle = sorr_ios_audio_load_song_path(filename);
    target = (int *)sorr_ios_audio_pointer_param(&params[1]);
    if (target)
    {
        *target = handle;
    }
    else
    {
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_SONG SP target pointer missing path=%s", filename);
    }
    string_discard(params[0]);
    return 0;
}

static int sorr_ios_sound_unload_song(INSTANCE *my, int *params)
{
    sorr_ios_audio_release_handle_ex(params[0], my, "UNLOAD_SONG", 1);
    return 0;
}

static int sorr_ios_sound_unload_song_ptr(INSTANCE *my, int *params)
{
    int *target;

    target = (int *)sorr_ios_audio_pointer_param(&params[0]);
    if (target)
    {
        sorr_ios_audio_release_handle_ex(*target, my, "UNLOAD_SONG_PTR", 1);
        *target = 0;
    }
    return 0;
}

static int sorr_ios_sound_play_song(INSTANCE *my, int *params)
{
    sorr_ios_audio_handle *entry;
    int result;

    (void)my;

    if (params[0] <= 0 || params[0] >= SORR_IOS_AUDIO_HANDLE_MAX)
    {
        sorr_ios_audio_music_play_attempt_count++;
        sorr_ios_audio_music_play_fail_count++;
        sorr_ios_audio_note_music_path("(invalid-handle)", "play-invalid");
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A PLAY_SONG invalid handle=%d", params[0]);
        return -1;
    }

    entry = &sorr_ios_audio_handles[params[0]];
    if (entry->kind != SORR_IOS_AUDIO_KIND_MUSIC || !entry->music)
    {
        sorr_ios_audio_music_play_attempt_count++;
        sorr_ios_audio_music_play_fail_count++;
        sorr_ios_audio_note_music_path(entry->path, "play-missing-music");
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A PLAY_SONG missing music handle=%d kind=%d", params[0], entry->kind);
        return -1;
    }

    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_init_device();
    }
    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_music_play_attempt_count++;
        sorr_ios_audio_music_play_fail_count++;
        sorr_ios_audio_note_music_path(entry->path, "play-no-mixer");
        return -1;
    }

    sorr_ios_audio_music_play_attempt_count++;
    sorr_ios_audio_music_last_handle = (unsigned long long)params[0];
    sorr_ios_audio_music_last_ptr = (unsigned long long)(uintptr_t)entry->music;
    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A PLAY_SONG begin handle=%d loops=%d music=%p type=%s bytes=%llu playing_before=%d path=%s",
                      params[0],
                      params[1],
                      (void *)entry->music,
                      sorr_ios_audio_music_type_name(Mix_GetMusicType(entry->music)),
                      (unsigned long long)entry->music_data_size,
                      Mix_PlayingMusic(),
                      entry->path);
    result = Mix_PlayMusic(entry->music, params[1]);
    sorr_ios_audio_music_last_playing = (unsigned int)Mix_PlayingMusic();
    if (result == 0)
    {
        sorr_ios_audio_music_play_ok_count++;
        sorr_ios_audio_note_music_path(entry->path, "play-ok");
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A PLAY_SONG ok handle=%d loops=%d playing_after=%u path=%s",
                          params[0],
                          params[1],
                          sorr_ios_audio_music_last_playing,
                          entry->path);
    }
    else
    {
        sorr_ios_audio_music_play_fail_count++;
        sorr_ios_audio_note_music_path(entry->path, "play-fail");
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A PLAY_SONG failed handle=%d loops=%d error=%s", params[0], params[1], Mix_GetError());
    }
    return result;
}

static int sorr_ios_sound_fade_music_in(INSTANCE *my, int *params)
{
    sorr_ios_audio_handle *entry;
    int result;

    (void)my;

    if (params[0] <= 0 || params[0] >= SORR_IOS_AUDIO_HANDLE_MAX)
    {
        sorr_ios_audio_music_control_count++;
        return -1;
    }

    entry = &sorr_ios_audio_handles[params[0]];
    if (entry->kind != SORR_IOS_AUDIO_KIND_MUSIC || !entry->music)
    {
        sorr_ios_audio_music_control_count++;
        return -1;
    }

    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_init_device();
    }
    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_music_control_count++;
        return -1;
    }

    sorr_ios_audio_music_control_count++;
    sorr_ios_audio_note_music_path(entry->path, "fade-in-start");
    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A FADE_MUSIC_IN begin handle=%d loops=%d ms=%d music=%p playing_before=%d path=%s",
                      params[0],
                      params[1],
                      params[2],
                      (void *)entry->music,
                      Mix_PlayingMusic(),
                      entry->path);
    result = Mix_FadeInMusic(entry->music, params[1], params[2]);
    sorr_ios_audio_music_last_playing = (unsigned int)Mix_PlayingMusic();
    if (result == 0)
    {
        sorr_ios_audio_note_music_path(entry->path, "fade-in-ok");
    }
    else
    {
        sorr_ios_audio_note_music_path(entry->path, "fade-in-fail");
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A FADE_MUSIC_IN failed handle=%d error=%s", params[0], Mix_GetError());
    }
    return result;
}

static int sorr_ios_sound_stop_song(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    if (sorr_ios_audio_initialized)
    {
        int result;
        sorr_ios_audio_music_control_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A STOP_SONG begin playing_before=%d", Mix_PlayingMusic());
        result = Mix_HaltMusic();
        sorr_ios_audio_music_halt_count++;
        sorr_ios_audio_music_last_playing = (unsigned int)Mix_PlayingMusic();
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A STOP_SONG end result=%d playing_after=%u error=%s",
                          result,
                          sorr_ios_audio_music_last_playing,
                          Mix_GetError());
        return result;
    }
    return 0;
}

static int sorr_ios_sound_pause_song(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    if (sorr_ios_audio_initialized)
    {
        sorr_ios_audio_music_control_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A PAUSE_SONG playing_before=%d", Mix_PlayingMusic());
        Mix_PauseMusic();
        return 0;
    }
    return -1;
}

static int sorr_ios_sound_resume_song(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    if (sorr_ios_audio_initialized)
    {
        sorr_ios_audio_music_control_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A RESUME_SONG playing_before=%d", Mix_PlayingMusic());
        Mix_ResumeMusic();
        return 0;
    }
    return -1;
}

static int sorr_ios_sound_set_song_volume(INSTANCE *my, int *params)
{
    int volume;

    (void)my;
    volume = params[0];
    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_init_device();
    }
    if (!sorr_ios_audio_initialized)
    {
        return -1;
    }
    sorr_ios_audio_music_control_count++;
    if (volume < 0) volume = 0;
    if (volume > 128) volume = 128;
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SET_SONG_VOLUME volume=%d", volume);
    Mix_VolumeMusic(volume);
    return 0;
}

static int sorr_ios_sound_is_playing_song(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    if (!sorr_ios_audio_initialized)
    {
        return 0;
    }
    sorr_ios_audio_music_query_count++;
    sorr_ios_audio_music_last_playing = (unsigned int)Mix_PlayingMusic();
    if (sorr_ios_audio_should_log_count(sorr_ios_audio_music_query_count))
    {
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A IS_PLAYING_SONG result=%u query_count=%u error=%s",
                          sorr_ios_audio_music_last_playing,
                          sorr_ios_audio_music_query_count,
                          Mix_GetError());
    }
    return (int)sorr_ios_audio_music_last_playing;
}

static int sorr_ios_sound_fade_music_off(INSTANCE *my, int *params)
{
    (void)my;
    if (!sorr_ios_audio_initialized)
    {
        return 0;
    }
    sorr_ios_audio_music_control_count++;
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A FADE_MUSIC_OFF ms=%d playing_before=%d", params[0], Mix_PlayingMusic());
    return Mix_FadeOutMusic(params[0]);
}

static int sorr_ios_sound_set_music_position(INSTANCE *my, int *params)
{
    (void)my;
    if (!sorr_ios_audio_initialized)
    {
        return -1;
    }
    sorr_ios_audio_music_control_count++;
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SET_MUSIC_POSITION raw=0x%08x", (unsigned int)params[0]);
    return Mix_SetMusicPosition((double)*(float *)&params[0]);
}

static int sorr_ios_sound_stop_wav(INSTANCE *my, int *params)
{
    (void)my;
    if (sorr_ios_audio_initialized && Mix_Playing(params[0]))
    {
        return Mix_HaltChannel(params[0]);
    }
    return -1;
}

static int sorr_ios_sound_pause_wav(INSTANCE *my, int *params)
{
    (void)my;
    if (sorr_ios_audio_initialized && Mix_Playing(params[0]))
    {
        Mix_Pause(params[0]);
        return 0;
    }
    return -1;
}

static int sorr_ios_sound_resume_wav(INSTANCE *my, int *params)
{
    (void)my;
    if (sorr_ios_audio_initialized && Mix_Playing(params[0]))
    {
        Mix_Resume(params[0]);
        return 0;
    }
    return -1;
}

static int sorr_ios_sound_is_playing_wav(INSTANCE *my, int *params)
{
    (void)my;
    if (sorr_ios_audio_initialized)
    {
        return Mix_Playing(params[0]);
    }
    return 0;
}

static int sorr_ios_sound_set_wav_volume(INSTANCE *my, int *params)
{
    sorr_ios_audio_handle *entry;
    int volume;

    (void)my;
    if (params[0] <= 0 || params[0] >= SORR_IOS_AUDIO_HANDLE_MAX)
    {
        return -1;
    }
    entry = &sorr_ios_audio_handles[params[0]];
    if (entry->kind != SORR_IOS_AUDIO_KIND_WAV || !entry->chunk)
    {
        return -1;
    }
    volume = params[1];
    if (volume < 0) volume = 0;
    if (volume > 128) volume = 128;
    return Mix_VolumeChunk(entry->chunk, volume);
}

static int sorr_ios_sound_set_channel_volume(INSTANCE *my, int *params)
{
    int volume;

    (void)my;
    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_init_device();
    }
    if (!sorr_ios_audio_initialized)
    {
        return -1;
    }
    volume = params[1];
    if (volume < 0) volume = 0;
    if (volume > 128) volume = 128;
    return Mix_Volume(params[0], volume);
}

static int sorr_ios_sound_reserve_channels(INSTANCE *my, int *params)
{
    (void)my;
    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_init_device();
    }
    if (!sorr_ios_audio_initialized)
    {
        return -1;
    }
    return Mix_ReserveChannels(params[0]);
}

static int sorr_ios_sound_set_panning(INSTANCE *my, int *params)
{
    (void)my;
    if (sorr_ios_audio_initialized && Mix_Playing(params[0]))
    {
        return Mix_SetPanning(params[0], (Uint8)params[1], (Uint8)params[2]) ? 0 : -1;
    }
    return -1;
}

static int sorr_ios_sound_set_position(INSTANCE *my, int *params)
{
    (void)my;
    if (sorr_ios_audio_initialized && Mix_Playing(params[0]))
    {
        return Mix_SetPosition(params[0], (Sint16)params[1], (Uint8)params[2]) ? 0 : -1;
    }
    return -1;
}

static int sorr_ios_sound_set_distance(INSTANCE *my, int *params)
{
    (void)my;
    if (sorr_ios_audio_initialized && Mix_Playing(params[0]))
    {
        return Mix_SetDistance(params[0], (Uint8)params[1]) ? 0 : -1;
    }
    return -1;
}

static int sorr_ios_sound_reverse_stereo(INSTANCE *my, int *params)
{
    (void)my;
    if (sorr_ios_audio_initialized && Mix_Playing(params[0]))
    {
        return Mix_SetReverseStereo(params[0], params[1]) ? 0 : -1;
    }
    return -1;
}

DLCONSTANT __bgdexport(mod_sound, constants_def)[] =
{
    { "MODE_MONO", TYPE_INT, 0 },
    { "MODE_STEREO", TYPE_INT, 1 },
    { "ALL_SOUND", TYPE_INT, -1 },
    { NULL, 0, 0 }
};

char * __bgdexport(mod_sound, globals_def) =
    "   sound_freq = 22050 ;\n"
    "   sound_mode = MODE_STEREO ;\n"
    "   sound_channels = 8 ;\n";

DLVARFIXUP __bgdexport(mod_sound, globals_fixup)[] =
{
    { "sound_freq", NULL, -1, -1 },
    { "sound_mode", NULL, -1, -1 },
    { "sound_channels", NULL, -1, -1 },
    { NULL, NULL, -1, -1 }
};

DLSYSFUNCS __bgdexport(mod_sound, functions_exports)[] =
{
    { "SOUND_INIT", "", TYPE_INT, sorr_ios_sound_init },
    { "SOUND_CLOSE", "", TYPE_INT, sorr_ios_sound_close },
    { "LOAD_SONG", "S", TYPE_INT, sorr_ios_sound_load_song },
    { "LOAD_SONG", "SP", TYPE_INT, sorr_ios_sound_bgload_song },
    { "PLAY_SONG", "II", TYPE_INT, sorr_ios_sound_play_song },
    { "UNLOAD_SONG", "I", TYPE_INT, sorr_ios_sound_unload_song },
    { "UNLOAD_SONG", "P", TYPE_INT, sorr_ios_sound_unload_song_ptr },
    { "STOP_SONG", "", TYPE_INT, sorr_ios_sound_stop_song },
    { "PAUSE_SONG", "", TYPE_INT, sorr_ios_sound_pause_song },
    { "RESUME_SONG", "", TYPE_INT, sorr_ios_sound_resume_song },
    { "SET_SONG_VOLUME", "I", TYPE_INT, sorr_ios_sound_set_song_volume },
    { "IS_PLAYING_SONG", "", TYPE_INT, sorr_ios_sound_is_playing_song },
    { "LOAD_WAV", "S", TYPE_INT, sorr_ios_sound_load_wav },
    { "LOAD_WAV", "SP", TYPE_INT, sorr_ios_sound_bgload_wav },
    { "UNLOAD_WAV", "I", TYPE_INT, sorr_ios_sound_unload_wav },
    { "UNLOAD_WAV", "P", TYPE_INT, sorr_ios_sound_unload_wav_ptr },
    { "PLAY_WAV", "II", TYPE_INT, sorr_ios_sound_play_wav },
    { "PLAY_WAV", "III", TYPE_INT, sorr_ios_sound_play_wav_channel },
    { "STOP_WAV", "I", TYPE_INT, sorr_ios_sound_stop_wav },
    { "PAUSE_WAV", "I", TYPE_INT, sorr_ios_sound_pause_wav },
    { "RESUME_WAV", "I", TYPE_INT, sorr_ios_sound_resume_wav },
    { "IS_PLAYING_WAV", "I", TYPE_INT, sorr_ios_sound_is_playing_wav },
    { "FADE_MUSIC_IN", "III", TYPE_INT, sorr_ios_sound_fade_music_in },
    { "FADE_MUSIC_OFF", "I", TYPE_INT, sorr_ios_sound_fade_music_off },
    { "SET_WAV_VOLUME", "II", TYPE_INT, sorr_ios_sound_set_wav_volume },
    { "SET_CHANNEL_VOLUME", "II", TYPE_INT, sorr_ios_sound_set_channel_volume },
    { "RESERVE_CHANNELS", "I", TYPE_INT, sorr_ios_sound_reserve_channels },
    { "SET_PANNING", "III", TYPE_INT, sorr_ios_sound_set_panning },
    { "SET_POSITION", "III", TYPE_INT, sorr_ios_sound_set_position },
    { "SET_DISTANCE", "II", TYPE_INT, sorr_ios_sound_set_distance },
    { "REVERSE_STEREO", "II", TYPE_INT, sorr_ios_sound_reverse_stereo },
    { "SET_MUSIC_POSITION", "F", TYPE_INT, sorr_ios_sound_set_music_position },
    { 0, 0, 0, 0 }
};

void __bgdexport(mod_sound, module_initialize)()
{
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SDL_mixer audio backend initialize");
    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        {
            PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SDL audio subsystem init failed error=%s", SDL_GetError());
        }
    }
}

void __bgdexport(mod_sound, module_finalize)()
{
    sorr_ios_audio_close_device();
    if (SDL_WasInit(SDL_INIT_AUDIO))
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SDL_mixer audio backend finalize");
}
