#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "bgddl.h"
#include "dlvaracc.h"
#include "files.h"
#include "xstrings.h"

#include "SDL.h"

#ifdef PORTABLE_RUNTIME_DIAG
#include "portable_diag.h"
#endif

#ifndef PORTABLE_DIAG_LOG
#define PORTABLE_DIAG_LOG(tag, fmt, ...) ((void)0)
#endif

#define SOUND_FREQ 0
#define SOUND_MODE 1
#define SOUND_CHANNELS 2

#define SORR_IOS_AUDIO_HANDLE_MAX 4096
#define SORR_IOS_AUDIO_PATH_MAX 512
#define SORR_IOS_AUDIO_KIND_EMPTY 0
#define SORR_IOS_AUDIO_KIND_WAV 1
#define SORR_IOS_AUDIO_KIND_INERT_WAV 2
#define SORR_IOS_AUDIO_KIND_INERT_MUSIC 3
#define SORR_IOS_AUDIO_MAX_QUEUED_MS 2500u

typedef struct sorr_ios_audio_handle
{
    int kind;
    Uint8 *data;
    Uint32 length;
    int volume;
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

static int sorr_ios_audio_initialized = 0;
static int sorr_ios_audio_open_attempted = 0;
static SDL_AudioDeviceID sorr_ios_audio_device = 0;
static SDL_AudioSpec sorr_ios_audio_have;
static sorr_ios_audio_handle sorr_ios_audio_handles[SORR_IOS_AUDIO_HANDLE_MAX];

#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
extern void *portable_x64_sysproc_pointer_param(int *cell);
#define sorr_ios_audio_pointer_param(cell) portable_x64_sysproc_pointer_param(cell)
#else
#define sorr_ios_audio_pointer_param(cell) ((void *)(*(cell)))
#endif

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

static int sorr_ios_audio_store_handle(int kind, const char *path, Uint8 *data, Uint32 length)
{
    int i;

    for (i = 1; i < SORR_IOS_AUDIO_HANDLE_MAX; i++)
    {
        if (sorr_ios_audio_handles[i].kind == SORR_IOS_AUDIO_KIND_EMPTY)
        {
            sorr_ios_audio_handles[i].kind = kind;
            sorr_ios_audio_handles[i].data = data;
            sorr_ios_audio_handles[i].length = length;
            sorr_ios_audio_handles[i].volume = SDL_MIX_MAXVOLUME;
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

            if (kind == SORR_IOS_AUDIO_KIND_INERT_WAV || kind == SORR_IOS_AUDIO_KIND_INERT_MUSIC)
            {
                sorr_ios_audio_inert_handle_count++;
            }

            PORTABLE_DIAG_LOG("AUDIO",
                              "iOS D3A audio handle store kind=%d handle=%d path=%s length=%u",
                              kind,
                              i,
                              path ? path : "(null)",
                              (unsigned int)length);
            return i;
        }
    }

    if (data)
    {
        SDL_free(data);
    }

    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A audio handle table full path=%s", path ? path : "(null)");
    return 0;
}

static void sorr_ios_audio_release_handle(int handle)
{
    if (handle <= 0 || handle >= SORR_IOS_AUDIO_HANDLE_MAX)
    {
        return;
    }

    if (sorr_ios_audio_handles[handle].data)
    {
        SDL_free(sorr_ios_audio_handles[handle].data);
    }

    memset(&sorr_ios_audio_handles[handle], 0, sizeof(sorr_ios_audio_handles[handle]));
}

static int sorr_ios_audio_inert_handle(const char *path, int kind)
{
    int existing = sorr_ios_audio_find_path(path, kind);

    if (existing)
    {
        return existing;
    }

    return sorr_ios_audio_store_handle(kind, path, NULL, 0);
}

static int sorr_ios_audio_init_device(void)
{
    SDL_AudioSpec want;
    int audio_rate = 22050;
    int audio_channels = 2;
    int samples = 1024;

    if (sorr_ios_audio_initialized)
    {
        return 0;
    }

    if (sorr_ios_audio_open_attempted && !sorr_ios_audio_device)
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

    samples = 1024 * audio_rate / 22050;
    if (samples < 512)
    {
        samples = 512;
    }

    SDL_zero(want);
    want.freq = audio_rate;
    want.format = AUDIO_S16SYS;
    want.channels = (Uint8)audio_channels;
    want.samples = (Uint16)samples;

    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A SDL_OpenAudioDevice begin driver=%s freq=%d channels=%d samples=%d",
                      SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "(none)",
                      want.freq,
                      want.channels,
                      want.samples);

    sorr_ios_audio_device = SDL_OpenAudioDevice(NULL,
                                                0,
                                                &want,
                                                &sorr_ios_audio_have,
                                                SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                                                    SDL_AUDIO_ALLOW_FORMAT_CHANGE |
                                                    SDL_AUDIO_ALLOW_CHANNELS_CHANGE |
                                                    SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (!sorr_ios_audio_device)
    {
        sorr_ios_audio_init_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SDL_OpenAudioDevice failed error=%s", SDL_GetError());
        return 0;
    }

    SDL_PauseAudioDevice(sorr_ios_audio_device, 0);
    sorr_ios_audio_initialized = 1;
    sorr_ios_audio_init_ok_count++;
    if (GLOEXISTS(mod_sound, SOUND_CHANNELS))
    {
        GLODWORD(mod_sound, SOUND_CHANNELS) = 8;
    }

    PORTABLE_DIAG_LOG("AUDIO",
                      "iOS D3A SDL_OpenAudioDevice ok device=%u driver=%s freq=%d format=%u channels=%d samples=%d",
                      (unsigned int)sorr_ios_audio_device,
                      SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "(none)",
                      sorr_ios_audio_have.freq,
                      (unsigned int)sorr_ios_audio_have.format,
                      (int)sorr_ios_audio_have.channels,
                      (int)sorr_ios_audio_have.samples);
    return 0;
}

static void sorr_ios_audio_close_device(void)
{
    int i;

    for (i = 1; i < SORR_IOS_AUDIO_HANDLE_MAX; i++)
    {
        sorr_ios_audio_release_handle(i);
    }

    if (sorr_ios_audio_device)
    {
        SDL_ClearQueuedAudio(sorr_ios_audio_device);
        SDL_CloseAudioDevice(sorr_ios_audio_device);
        sorr_ios_audio_device = 0;
    }

    sorr_ios_audio_initialized = 0;
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A audio closed");
}

static Uint32 sorr_ios_audio_max_queued_bytes(void)
{
    Uint32 bytes_per_second;

    if (!sorr_ios_audio_initialized || !sorr_ios_audio_have.freq || !sorr_ios_audio_have.channels)
    {
        return 0;
    }

    bytes_per_second = (Uint32)sorr_ios_audio_have.freq *
                       (Uint32)sorr_ios_audio_have.channels *
                       (Uint32)(SDL_AUDIO_BITSIZE(sorr_ios_audio_have.format) / 8);
    return (bytes_per_second * SORR_IOS_AUDIO_MAX_QUEUED_MS) / 1000u;
}

static int sorr_ios_audio_load_wav_path(const char *filename)
{
    int existing;
    file *fp;
    SDL_RWops *rwops;
    SDL_AudioSpec source_spec;
    Uint8 *source_buffer = NULL;
    Uint32 source_length = 0;
    Uint8 *stored_buffer = NULL;
    Uint32 stored_length = 0;
    SDL_AudioCVT cvt;

    if (!filename || !filename[0])
    {
        return 0;
    }

    existing = sorr_ios_audio_find_path(filename, SORR_IOS_AUDIO_KIND_WAV);
    if (existing)
    {
        return existing;
    }
    existing = sorr_ios_audio_find_path(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    if (existing)
    {
        return existing;
    }

    sorr_ios_audio_init_device();

    fp = file_open(filename, "rb0");
    if (!fp)
    {
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_WAV file open failed path=%s", filename);
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    rwops = sorr_ios_audio_rw_from_file(fp);
    if (!rwops)
    {
        file_close(fp);
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_WAV SDL_RWops failed path=%s", filename);
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    if (!SDL_LoadWAV_RW(rwops, 1, &source_spec, &source_buffer, &source_length))
    {
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A LOAD_WAV decode failed path=%s error=%s",
                          filename,
                          SDL_GetError());
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    if (sorr_ios_audio_initialized &&
        SDL_BuildAudioCVT(&cvt,
                          source_spec.format,
                          source_spec.channels,
                          source_spec.freq,
                          sorr_ios_audio_have.format,
                          sorr_ios_audio_have.channels,
                          sorr_ios_audio_have.freq) >= 0 &&
        cvt.needed)
    {
        cvt.len = (int)source_length;
        cvt.buf = (Uint8 *)SDL_malloc((size_t)cvt.len * (size_t)cvt.len_mult);
        if (cvt.buf)
        {
            memcpy(cvt.buf, source_buffer, source_length);
            if (SDL_ConvertAudio(&cvt) == 0)
            {
                stored_buffer = cvt.buf;
                stored_length = (Uint32)cvt.len_cvt;
            }
            else
            {
                SDL_free(cvt.buf);
            }
        }
        SDL_FreeWAV(source_buffer);
    }
    else
    {
        stored_buffer = source_buffer;
        stored_length = source_length;
    }

    if (!stored_buffer || !stored_length)
    {
        if (stored_buffer)
        {
            SDL_free(stored_buffer);
        }
        sorr_ios_audio_wav_load_fail_count++;
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_WAV conversion failed path=%s", filename);
        return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_WAV);
    }

    existing = sorr_ios_audio_store_handle(SORR_IOS_AUDIO_KIND_WAV, filename, stored_buffer, stored_length);
    if (existing)
    {
        sorr_ios_audio_wav_load_ok_count++;
    }
    return existing;
}

static int sorr_ios_audio_load_song_path(const char *filename)
{
    file *fp;

    if (!filename || !filename[0])
    {
        return 0;
    }

    fp = file_open(filename, "rb0");
    if (fp)
    {
        file_close(fp);
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_SONG staged inert music handle path=%s", filename);
    }
    else
    {
        PORTABLE_DIAG_LOG("AUDIO", "iOS D3A LOAD_SONG file open failed; using inert handle path=%s", filename);
    }

    sorr_ios_audio_init_device();
    return sorr_ios_audio_inert_handle(filename, SORR_IOS_AUDIO_KIND_INERT_MUSIC);
}

static int sorr_ios_audio_play_wav_handle(int handle, int loops, int channel)
{
    sorr_ios_audio_handle *entry;
    Uint32 max_queued;
    Uint32 queued;
    int play_count = 1;
    int i;

    (void)channel;

    if (handle <= 0 || handle >= SORR_IOS_AUDIO_HANDLE_MAX)
    {
        sorr_ios_sound_stub_zero_count++;
        return 0;
    }

    entry = &sorr_ios_audio_handles[handle];
    if (entry->kind == SORR_IOS_AUDIO_KIND_INERT_WAV || entry->kind == SORR_IOS_AUDIO_KIND_INERT_MUSIC)
    {
        sorr_ios_sound_stub_zero_count++;
        return 0;
    }

    if (entry->kind != SORR_IOS_AUDIO_KIND_WAV || !entry->data || !entry->length)
    {
        sorr_ios_sound_stub_zero_count++;
        return 0;
    }

    if (!sorr_ios_audio_initialized)
    {
        sorr_ios_audio_init_device();
    }
    if (!sorr_ios_audio_device)
    {
        sorr_ios_sound_stub_zero_count++;
        return 0;
    }

    max_queued = sorr_ios_audio_max_queued_bytes();
    queued = SDL_GetQueuedAudioSize(sorr_ios_audio_device);
    if (max_queued > 0 && queued > max_queued)
    {
        SDL_ClearQueuedAudio(sorr_ios_audio_device);
        sorr_ios_audio_queue_clear_count++;
        PORTABLE_DIAG_LOG("AUDIO",
                          "iOS D3A cleared queued audio bytes=%u max=%u",
                          (unsigned int)queued,
                          (unsigned int)max_queued);
    }

    if (loops > 0)
    {
        play_count = loops + 1;
        if (play_count > 4)
        {
            play_count = 4;
        }
    }

    for (i = 0; i < play_count; i++)
    {
        if (SDL_QueueAudio(sorr_ios_audio_device, entry->data, entry->length) != 0)
        {
            sorr_ios_sound_stub_zero_count++;
            PORTABLE_DIAG_LOG("AUDIO", "iOS D3A SDL_QueueAudio failed handle=%d error=%s", handle, SDL_GetError());
            return 0;
        }
    }

    sorr_ios_audio_wav_play_count++;
    return channel >= 0 ? channel : 0;
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

    (void)my;
    filename = string_get(params[0]);
    if (!filename)
    {
        return 0;
    }

    handle = sorr_ios_audio_load_wav_path(filename);
    string_discard(params[0]);
    return handle;
}

static int sorr_ios_sound_bgload_wav(INSTANCE *my, int *params)
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

    handle = sorr_ios_audio_load_wav_path(filename);
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
    (void)my;
    sorr_ios_audio_release_handle(params[0]);
    return 0;
}

static int sorr_ios_sound_unload_wav_ptr(INSTANCE *my, int *params)
{
    int *target;

    (void)my;
    target = (int *)sorr_ios_audio_pointer_param(&params[0]);
    if (target)
    {
        sorr_ios_audio_release_handle(*target);
        *target = 0;
    }
    return 0;
}

static int sorr_ios_sound_play_wav(INSTANCE *my, int *params)
{
    (void)my;
    return sorr_ios_audio_play_wav_handle(params[0], params[1], -1);
}

static int sorr_ios_sound_play_wav_channel(INSTANCE *my, int *params)
{
    (void)my;
    return sorr_ios_audio_play_wav_handle(params[0], params[1], params[2]);
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
    (void)my;
    sorr_ios_audio_release_handle(params[0]);
    return 0;
}

static int sorr_ios_sound_unload_song_ptr(INSTANCE *my, int *params)
{
    int *target;

    (void)my;
    target = (int *)sorr_ios_audio_pointer_param(&params[0]);
    if (target)
    {
        sorr_ios_audio_release_handle(*target);
        *target = 0;
    }
    return 0;
}

static int sorr_ios_sound_zero(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    sorr_ios_sound_stub_zero_count++;
    return 0;
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
    { "PLAY_SONG", "II", TYPE_INT, sorr_ios_sound_zero },
    { "UNLOAD_SONG", "I", TYPE_INT, sorr_ios_sound_unload_song },
    { "UNLOAD_SONG", "P", TYPE_INT, sorr_ios_sound_unload_song_ptr },
    { "STOP_SONG", "", TYPE_INT, sorr_ios_sound_zero },
    { "PAUSE_SONG", "", TYPE_INT, sorr_ios_sound_zero },
    { "RESUME_SONG", "", TYPE_INT, sorr_ios_sound_zero },
    { "SET_SONG_VOLUME", "I", TYPE_INT, sorr_ios_sound_zero },
    { "IS_PLAYING_SONG", "", TYPE_INT, sorr_ios_sound_zero },
    { "LOAD_WAV", "S", TYPE_INT, sorr_ios_sound_load_wav },
    { "LOAD_WAV", "SP", TYPE_INT, sorr_ios_sound_bgload_wav },
    { "UNLOAD_WAV", "I", TYPE_INT, sorr_ios_sound_unload_wav },
    { "UNLOAD_WAV", "P", TYPE_INT, sorr_ios_sound_unload_wav_ptr },
    { "PLAY_WAV", "II", TYPE_INT, sorr_ios_sound_play_wav },
    { "PLAY_WAV", "III", TYPE_INT, sorr_ios_sound_play_wav_channel },
    { "STOP_WAV", "I", TYPE_INT, sorr_ios_sound_zero },
    { "PAUSE_WAV", "I", TYPE_INT, sorr_ios_sound_zero },
    { "RESUME_WAV", "I", TYPE_INT, sorr_ios_sound_zero },
    { "IS_PLAYING_WAV", "I", TYPE_INT, sorr_ios_sound_zero },
    { "FADE_MUSIC_IN", "III", TYPE_INT, sorr_ios_sound_zero },
    { "FADE_MUSIC_OFF", "I", TYPE_INT, sorr_ios_sound_zero },
    { "SET_WAV_VOLUME", "II", TYPE_INT, sorr_ios_sound_zero },
    { "SET_CHANNEL_VOLUME", "II", TYPE_INT, sorr_ios_sound_zero },
    { "RESERVE_CHANNELS", "I", TYPE_INT, sorr_ios_sound_zero },
    { "SET_PANNING", "III", TYPE_INT, sorr_ios_sound_zero },
    { "SET_POSITION", "III", TYPE_INT, sorr_ios_sound_zero },
    { "SET_DISTANCE", "II", TYPE_INT, sorr_ios_sound_zero },
    { "REVERSE_STEREO", "II", TYPE_INT, sorr_ios_sound_zero },
    { "SET_MUSIC_POSITION", "F", TYPE_INT, sorr_ios_sound_zero },
    { 0, 0, 0, 0 }
};

void __bgdexport(mod_sound, module_initialize)()
{
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A minimal SDL audio backend initialize");
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
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3A minimal SDL audio backend finalize");
}
