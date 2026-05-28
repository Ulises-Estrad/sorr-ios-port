#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <errno.h>
#define SORR_MKDIR(path) _mkdir(path)
#else
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#define SORR_MKDIR(path) mkdir((path), 0755)
#endif

#include "SDL.h"

#ifdef SORR_IOS_D3_FIRST_RENDER
#if defined(__APPLE__)
#include <mach/mach.h>
#endif
#include "bgdrtm.h"
#include "files.h"
#include "g_frame.h"
#include "instance.h"
#include "xctype.h"
#include "xstrings.h"
#else
#include "offsets.h"
#include "sysprocs_st.h"
#endif

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

#ifndef SORR_IOS_D3_FIRST_RENDER
FN_HOOK *module_finalize_list = NULL;
int module_finalize_allocated = 0;
int module_finalize_count = 0;
void *globaldata = NULL;
#endif

void bgdrtm_entry(int argc, char *argv[]);

#ifdef SORR_IOS_D3_FIRST_RENDER
static char *sorr_ios_strdup(const char *text)
{
    size_t len;
    char *copy;

    if (!text)
    {
        return NULL;
    }

    len = strlen(text);
    copy = (char *)malloc(len + 1);
    if (!copy)
    {
        return NULL;
    }

    memcpy(copy, text, len + 1);
    return copy;
}
#endif

typedef struct sorr_ios_data_layout
{
    char bundle_root[1024];
    char documents_root[1024];
    char documents_import_dir[1024];
    char documents_diagnostics_dir[1024];
    char support_root[1024];
    char savegame_dir[1024];
    char xbox_dir[1024];
    char logs_dir[1024];
    char sorr_dat_path[1024];
    char required_file_path[1024];
    char d2_probe_path[1024];
    char d3_probe_path[1024];
    char d3_stability_path[1024];
    char d3_visible_stability_path[1024];
    bool d2_data_ready;
    bool d2_import_seen;
    bool d2_import_failed;
} sorr_ios_data_layout;

#define SORR_IOS_STATUS_MAX_LINES 16
#define SORR_IOS_STATUS_LINE_LEN 96

typedef enum sorr_ios_import_layout
{
    SORR_IOS_IMPORT_NONE = 0,
    SORR_IOS_IMPORT_DIRECT,
    SORR_IOS_IMPORT_NESTED_ONE_FOLDER,
    SORR_IOS_IMPORT_INVALID_MULTIPLE,
    SORR_IOS_IMPORT_INVALID_NESTED,
    SORR_IOS_IMPORT_INVALID_MISSING_DAT
} sorr_ios_import_layout;

typedef struct sorr_ios_import_probe
{
    sorr_ios_import_layout layout;
    char root[1024];
} sorr_ios_import_probe;

static char sorr_ios_status_lines[SORR_IOS_STATUS_MAX_LINES][SORR_IOS_STATUS_LINE_LEN];
static int sorr_ios_status_line_count = 0;
static SDL_Color sorr_ios_status_color = {74, 130, 190, 255};

static void sorr_ios_status_clear(void)
{
    memset(sorr_ios_status_lines, 0, sizeof(sorr_ios_status_lines));
    sorr_ios_status_line_count = 0;
}

static void sorr_ios_status_add(const char *line)
{
    if (!line || sorr_ios_status_line_count >= SORR_IOS_STATUS_MAX_LINES)
    {
        return;
    }

    snprintf(sorr_ios_status_lines[sorr_ios_status_line_count],
             sizeof(sorr_ios_status_lines[sorr_ios_status_line_count]),
             "%s",
             line);
    sorr_ios_status_line_count++;
}

static void sorr_ios_status_set_waiting(void)
{
    sorr_ios_status_color.r = 74;
    sorr_ios_status_color.g = 130;
    sorr_ios_status_color.b = 190;
}

static void sorr_ios_status_set_ready(void)
{
    sorr_ios_status_color.r = 57;
    sorr_ios_status_color.g = 150;
    sorr_ios_status_color.b = 96;
}

static void sorr_ios_status_set_error(void)
{
    sorr_ios_status_color.r = 178;
    sorr_ios_status_color.g = 72;
    sorr_ios_status_color.b = 72;
}

static const unsigned char *sorr_ios_font_rows(char input)
{
    static const unsigned char blank[7] = {0, 0, 0, 0, 0, 0, 0};
    static const unsigned char slash[7] = {1, 2, 2, 4, 8, 8, 16};
    static const unsigned char dot[7] = {0, 0, 0, 0, 0, 12, 12};
    static const unsigned char dash[7] = {0, 0, 0, 31, 0, 0, 0};
    static const unsigned char colon[7] = {0, 12, 12, 0, 12, 12, 0};
    static const unsigned char chars[36][7] = {
        {14, 17, 17, 31, 17, 17, 17}, /* A */
        {30, 17, 17, 30, 17, 17, 30}, /* B */
        {14, 17, 16, 16, 16, 17, 14}, /* C */
        {30, 17, 17, 17, 17, 17, 30}, /* D */
        {31, 16, 16, 30, 16, 16, 31}, /* E */
        {31, 16, 16, 30, 16, 16, 16}, /* F */
        {14, 17, 16, 23, 17, 17, 14}, /* G */
        {17, 17, 17, 31, 17, 17, 17}, /* H */
        {14, 4, 4, 4, 4, 4, 14},     /* I */
        {7, 2, 2, 2, 18, 18, 12},     /* J */
        {17, 18, 20, 24, 20, 18, 17}, /* K */
        {16, 16, 16, 16, 16, 16, 31}, /* L */
        {17, 27, 21, 21, 17, 17, 17}, /* M */
        {17, 25, 21, 19, 17, 17, 17}, /* N */
        {14, 17, 17, 17, 17, 17, 14}, /* O */
        {30, 17, 17, 30, 16, 16, 16}, /* P */
        {14, 17, 17, 17, 21, 18, 13}, /* Q */
        {30, 17, 17, 30, 20, 18, 17}, /* R */
        {15, 16, 16, 14, 1, 1, 30},   /* S */
        {31, 4, 4, 4, 4, 4, 4},       /* T */
        {17, 17, 17, 17, 17, 17, 14}, /* U */
        {17, 17, 17, 17, 17, 10, 4},  /* V */
        {17, 17, 17, 21, 21, 21, 10}, /* W */
        {17, 17, 10, 4, 10, 17, 17},  /* X */
        {17, 17, 10, 4, 4, 4, 4},     /* Y */
        {31, 1, 2, 4, 8, 16, 31},     /* Z */
        {14, 17, 19, 21, 25, 17, 14}, /* 0 */
        {4, 12, 4, 4, 4, 4, 14},      /* 1 */
        {14, 17, 1, 2, 4, 8, 31},     /* 2 */
        {30, 1, 1, 14, 1, 1, 30},     /* 3 */
        {2, 6, 10, 18, 31, 2, 2},     /* 4 */
        {31, 16, 16, 30, 1, 1, 30},   /* 5 */
        {14, 16, 16, 30, 17, 17, 14}, /* 6 */
        {31, 1, 2, 4, 8, 8, 8},       /* 7 */
        {14, 17, 17, 14, 17, 17, 14}, /* 8 */
        {14, 17, 17, 15, 1, 1, 14}    /* 9 */
    };

    char c = input;
    if (c >= 'a' && c <= 'z')
    {
        c = (char)(c - 'a' + 'A');
    }
    if (c >= 'A' && c <= 'Z')
    {
        return chars[c - 'A'];
    }
    if (c >= '0' && c <= '9')
    {
        return chars[26 + (c - '0')];
    }
    if (c == '/')
    {
        return slash;
    }
    if (c == '.')
    {
        return dot;
    }
    if (c == '-')
    {
        return dash;
    }
    if (c == ':')
    {
        return colon;
    }
    return blank;
}

static void sorr_ios_draw_text(SDL_Renderer *renderer, int x, int y, int scale, const char *text)
{
    SDL_Rect pixel;
    SDL_Color color = {238, 242, 248, 255};

    if (!renderer || !text || scale <= 0)
    {
        return;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    pixel.w = scale;
    pixel.h = scale;

    while (*text)
    {
        const unsigned char *rows = sorr_ios_font_rows(*text);
        int row;
        int col;

        for (row = 0; row < 7; row++)
        {
            for (col = 0; col < 5; col++)
            {
                if (rows[row] & (1 << (4 - col)))
                {
                    pixel.x = x + col * scale;
                    pixel.y = y + row * scale;
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }

        x += 6 * scale;
        text++;
    }
}

static void sorr_ios_draw_status(SDL_Renderer *renderer)
{
    int i;

    if (!renderer)
    {
        return;
    }

    SDL_SetRenderDrawColor(renderer,
                           sorr_ios_status_color.r,
                           sorr_ios_status_color.g,
                           sorr_ios_status_color.b,
                           255);
    SDL_RenderClear(renderer);

    for (i = 0; i < sorr_ios_status_line_count; i++)
    {
        sorr_ios_draw_text(renderer, 24, 24 + i * 29, 3, sorr_ios_status_lines[i]);
    }
}

static const char *sorr_ios_import_layout_name(sorr_ios_import_layout layout)
{
    switch (layout)
    {
        case SORR_IOS_IMPORT_DIRECT:
            return "direct";
        case SORR_IOS_IMPORT_NESTED_ONE_FOLDER:
            return "nested-one-folder";
        case SORR_IOS_IMPORT_INVALID_MULTIPLE:
            return "invalid/multiple-folders";
        case SORR_IOS_IMPORT_INVALID_NESTED:
            return "invalid/nested-too-deep";
        case SORR_IOS_IMPORT_INVALID_MISSING_DAT:
            return "invalid/SorR.dat-missing";
        case SORR_IOS_IMPORT_NONE:
        default:
            return "none";
    }
}

static const char *sorr_ios_import_layout_status(sorr_ios_import_layout layout)
{
    switch (layout)
    {
        case SORR_IOS_IMPORT_DIRECT:
            return "LAYOUT DIRECT";
        case SORR_IOS_IMPORT_NESTED_ONE_FOLDER:
            return "LAYOUT NESTED ONE FOLDER";
        case SORR_IOS_IMPORT_INVALID_MULTIPLE:
            return "LAYOUT INVALID MULTIPLE";
        case SORR_IOS_IMPORT_INVALID_NESTED:
            return "LAYOUT INVALID NESTED";
        case SORR_IOS_IMPORT_INVALID_MISSING_DAT:
            return "LAYOUT INVALID NO DAT";
        case SORR_IOS_IMPORT_NONE:
        default:
            return "LAYOUT NONE";
    }
}

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

static int sorr_ios_file_size(const char *path, long *out_size)
{
    struct stat st;

    if (!path || stat(path, &st) != 0 || S_ISDIR(st.st_mode))
    {
        return 0;
    }

    if (out_size)
    {
        *out_size = (long)st.st_size;
    }
    return 1;
}

static int sorr_ios_open_file_probe(const char *label, const char *path, long *out_size)
{
    FILE *fp;
    long size = 0;

    if (!sorr_ios_file_size(path, &size))
    {
        SDL_Log("SORR iOS shell: %s missing path=%s", label, path ? path : "(null)");
        return 0;
    }

    fp = fopen(path, "rb");
    if (!fp)
    {
        SDL_Log("SORR iOS shell: %s open failed path=%s errno=%d", label, path, errno);
        return 0;
    }
    fclose(fp);

    if (out_size)
    {
        *out_size = size;
    }
    SDL_Log("SORR iOS shell: %s found/opened path=%s size=%ld", label, path, size);
    return 1;
}

static int sorr_ios_write_text_file(const char *path, const char *text)
{
    FILE *fp;
    size_t len;

    if (!path || !text)
    {
        return 0;
    }

    fp = fopen(path, "wb");
    if (!fp)
    {
        return 0;
    }

    len = strlen(text);
    if (fwrite(text, 1, len, fp) != len)
    {
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

static int sorr_ios_copy_file_contents(const char *src, const char *dst)
{
    FILE *in;
    FILE *out;
    char buffer[4096];
    size_t read_count;

    if (!src || !dst || !src[0] || !dst[0])
    {
        return 0;
    }

    in = fopen(src, "rb");
    if (!in)
    {
        return 0;
    }

    out = fopen(dst, "wb");
    if (!out)
    {
        fclose(in);
        return 0;
    }

    while ((read_count = fread(buffer, 1, sizeof(buffer), in)) > 0)
    {
        if (fwrite(buffer, 1, read_count, out) != read_count)
        {
            fclose(out);
            fclose(in);
            return 0;
        }
    }

    if (ferror(in))
    {
        fclose(out);
        fclose(in);
        return 0;
    }

    fclose(out);
    fclose(in);
    return 1;
}

#ifdef SORR_IOS_D3_FIRST_RENDER
enum sorr_ios_d3_runtime_stage
{
    SORR_IOS_D3_STAGE_APP_LAUNCH = 1,
    SORR_IOS_D3_STAGE_PREFLIGHT,
    SORR_IOS_D3_STAGE_RUNTIME_PATHS,
    SORR_IOS_D3_STAGE_DCB_LOAD,
    SORR_IOS_D3_STAGE_SYSPROC,
    SORR_IOS_D3_STAGE_RUNTIME_HANDOFF,
    SORR_IOS_D3_STAGE_RUNTIME_LOOP,
    SORR_IOS_D3_STAGE_RUNTIME_RETURNED
};

static volatile int sorr_ios_d3_stage = 0;
static volatile unsigned int sorr_ios_d3_heartbeat_count = 0;
static volatile Uint32 sorr_ios_d3_runtime_loop_start_ticks = 0;
static volatile int sorr_ios_d3_dense_window_marker = 0;
static volatile int sorr_ios_d3_first_frame_marker = 0;

extern int x_files_count;
extern int max_x_files;
extern volatile int sorr_ios_d3_live_instance_count;
extern volatile int sorr_ios_d3_render_object_count;
extern volatile unsigned int sorr_ios_d3_render_instance_object_created_count;
extern volatile unsigned int sorr_ios_d3_render_instance_object_destroyed_count;
extern volatile unsigned int sorr_ios_d3_render_invalid_callback_count;
extern volatile unsigned int sorr_ios_d3_instance_go_loop_count;
extern volatile unsigned int sorr_ios_d3_frame_complete_count;
extern volatile unsigned int sorr_ios_d3_instance_run_count;
extern volatile unsigned int sorr_ios_d3_instance_created_count;
extern volatile unsigned int sorr_ios_d3_instance_destroyed_count;
extern volatile unsigned int sorr_ios_d3_snapshot_count;
extern volatile unsigned int sorr_ios_d3_last_proc_id;
extern volatile unsigned long long sorr_ios_d3_last_proc_ptr;
extern volatile unsigned long long sorr_ios_d3_current_proc_ptr;
extern volatile int sorr_ios_d3_last_proc_status;
extern volatile int sorr_ios_d3_last_proc_frame_percent;
extern volatile int sorr_ios_d3_last_proc_code_offset;
extern char sorr_ios_d3_last_proc_name[];
extern char sorr_ios_d3_last_lifecycle_event[];
extern char sorr_ios_d3_last_family_unlink[];
extern char sorr_ios_d3_last_render_event[];
extern char sorr_ios_d3_runtime_snapshot[];
extern char sorr_ios_d3_lifecycle_events[];
extern char sorr_ios_d3_visible_event_log_path[];
extern volatile unsigned int sorr_ios_sound_stub_zero_count;
extern volatile unsigned int sorr_ios_sound_stub_minus_one_count;
extern volatile unsigned int sorr_ios_audio_init_attempt_count;
extern volatile unsigned int sorr_ios_audio_init_ok_count;
extern volatile unsigned int sorr_ios_audio_init_fail_count;
extern volatile unsigned int sorr_ios_audio_wav_load_ok_count;
extern volatile unsigned int sorr_ios_audio_wav_load_fail_count;
extern volatile unsigned int sorr_ios_audio_wav_play_count;
extern volatile unsigned int sorr_ios_audio_inert_handle_count;
extern volatile unsigned int sorr_ios_audio_queue_clear_count;
extern volatile unsigned int sorr_ios_audio_music_load_attempt_count;
extern volatile unsigned int sorr_ios_audio_music_open_ok_count;
extern volatile unsigned int sorr_ios_audio_music_open_fail_count;
extern volatile unsigned int sorr_ios_audio_live_handle_count;
extern volatile unsigned int sorr_ios_audio_live_wav_count;
extern volatile unsigned int sorr_ios_audio_live_inert_wav_count;
extern volatile unsigned int sorr_ios_audio_live_music_count;
extern volatile unsigned int sorr_ios_audio_max_live_handle_count;
extern volatile unsigned int sorr_ios_audio_zero_music_play_count;
extern volatile unsigned int sorr_ios_audio_zero_music_control_count;
extern volatile unsigned int sorr_ios_audio_zero_music_query_count;
extern volatile unsigned int sorr_ios_audio_zero_wav_control_count;
extern volatile unsigned int sorr_ios_audio_zero_wav_query_count;
extern volatile unsigned int sorr_ios_audio_zero_wav_volume_count;
extern volatile unsigned int sorr_ios_audio_zero_channel_effect_count;
extern volatile unsigned int sorr_ios_audio_zero_play_wav_guard_count;
extern volatile unsigned int sorr_ios_audio_music_mem_load_ok_count;
extern volatile unsigned int sorr_ios_audio_music_mem_load_fail_count;
extern volatile unsigned int sorr_ios_audio_music_play_attempt_count;
extern volatile unsigned int sorr_ios_audio_music_play_ok_count;
extern volatile unsigned int sorr_ios_audio_music_play_fail_count;
extern volatile unsigned int sorr_ios_audio_music_control_count;
extern volatile unsigned int sorr_ios_audio_music_query_count;
extern volatile unsigned int sorr_ios_audio_music_free_count;
extern volatile unsigned int sorr_ios_audio_music_halt_count;
extern volatile unsigned int sorr_ios_audio_music_last_playing;
extern volatile unsigned long long sorr_ios_audio_music_last_bytes;
extern volatile unsigned long long sorr_ios_audio_music_total_bytes;
extern volatile unsigned long long sorr_ios_audio_music_last_ptr;
extern volatile unsigned long long sorr_ios_audio_music_last_handle;
extern char sorr_ios_audio_last_music_path[];
extern char sorr_ios_audio_last_music_status[];

#define SORR_IOS_D3_HEARTBEAT_NORMAL_MS 10000u
#define SORR_IOS_D3_HEARTBEAT_DENSE_MS 1000u
#define SORR_IOS_D3_DENSE_START_MS 240000u
#define SORR_IOS_D3_DENSE_END_MS 330000u

static const char *sorr_ios_d3_stage_name(int stage)
{
    switch (stage)
    {
        case SORR_IOS_D3_STAGE_APP_LAUNCH:
            return "app-launch";
        case SORR_IOS_D3_STAGE_PREFLIGHT:
            return "d2-preflight";
        case SORR_IOS_D3_STAGE_RUNTIME_PATHS:
            return "runtime-paths";
        case SORR_IOS_D3_STAGE_DCB_LOAD:
            return "dcb-load";
        case SORR_IOS_D3_STAGE_SYSPROC:
            return "sysproc";
        case SORR_IOS_D3_STAGE_RUNTIME_HANDOFF:
            return "runtime-handoff";
        case SORR_IOS_D3_STAGE_RUNTIME_LOOP:
            return "runtime-loop";
        case SORR_IOS_D3_STAGE_RUNTIME_RETURNED:
            return "runtime-returned";
        default:
            return "unknown";
    }
}

#ifndef _WIN32
static char sorr_ios_d3_signal_log_path[1024];

static void sorr_ios_d3_signal_handler(int sig)
{
    int fd;

    if (sorr_ios_d3_signal_log_path[0])
    {
        char line[4096];
        int len;

        fd = open(sorr_ios_d3_signal_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd >= 0)
        {
            len = snprintf(line,
                           sizeof(line),
                           "signal=%d ticks=%u stage=%s runtime_loops=%u runtime_frames=%u runtime_runs=%u runtime_last_proc=%s#%u:s%d:f%d:o%d last_proc_ptr=0x%llx current_proc_ptr=0x%llx last_lifecycle=%s last_family=%s last_render=%s runtime_snapshot=%s runtime_lifecycle=%s\n",
                           sig,
                           SDL_GetTicks(),
                           sorr_ios_d3_stage_name(sorr_ios_d3_stage),
                           sorr_ios_d3_instance_go_loop_count,
                           sorr_ios_d3_frame_complete_count,
                           sorr_ios_d3_instance_run_count,
                           sorr_ios_d3_last_proc_name,
                           sorr_ios_d3_last_proc_id,
                           sorr_ios_d3_last_proc_status,
                           sorr_ios_d3_last_proc_frame_percent,
                           sorr_ios_d3_last_proc_code_offset,
                           sorr_ios_d3_last_proc_ptr,
                           sorr_ios_d3_current_proc_ptr,
                           sorr_ios_d3_last_lifecycle_event,
                           sorr_ios_d3_last_family_unlink,
                           sorr_ios_d3_last_render_event,
                           sorr_ios_d3_runtime_snapshot,
                           sorr_ios_d3_lifecycle_events);
            if (len > 0)
            {
                size_t write_len = (size_t)len;
                if (write_len >= sizeof(line))
                {
                    write_len = sizeof(line) - 1;
                }
                (void)write(fd, line, write_len);
            }
            close(fd);
        }
    }

    signal(sig, SIG_DFL);
    raise(sig);
}

static void sorr_ios_d3_install_signal_handlers(const char *path)
{
    if (path)
    {
        snprintf(sorr_ios_d3_signal_log_path, sizeof(sorr_ios_d3_signal_log_path), "%s", path);
    }

    signal(SIGABRT, sorr_ios_d3_signal_handler);
    signal(SIGBUS, sorr_ios_d3_signal_handler);
    signal(SIGFPE, sorr_ios_d3_signal_handler);
    signal(SIGILL, sorr_ios_d3_signal_handler);
    signal(SIGSEGV, sorr_ios_d3_signal_handler);
}
#endif

static void sorr_ios_d3_append_log_file(const char *path, const char *line)
{
    FILE *fp;

    if (!path || !path[0] || !line)
    {
        return;
    }

    fp = fopen(path, "ab");
    if (!fp)
    {
        SDL_Log("SORR iOS shell: D3 log append failed path=%s errno=%d", path, errno);
        return;
    }

    fputs(line, fp);
    fputc('\n', fp);
    fclose(fp);
}

static void sorr_ios_d3_log(const sorr_ios_data_layout *layout, const char *format, ...)
{
    char line[8192];
    va_list args;

    if (!format)
    {
        return;
    }

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    SDL_Log("SORR iOS shell: D3 %s", line);

    if (!layout)
    {
        return;
    }

    sorr_ios_d3_append_log_file(layout->d3_probe_path, line);
    sorr_ios_d3_append_log_file(layout->d3_stability_path, line);
    sorr_ios_d3_append_log_file(layout->d3_visible_stability_path, line);
}

static void sorr_ios_d3_stability_log(const sorr_ios_data_layout *layout, const char *format, ...)
{
    char line[8192];
    va_list args;

    if (!format)
    {
        return;
    }

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    SDL_Log("SORR iOS shell: D3 stability %s", line);

    if (layout)
    {
        sorr_ios_d3_append_log_file(layout->d3_stability_path, line);
        sorr_ios_d3_append_log_file(layout->d3_visible_stability_path, line);
    }
}

static void sorr_ios_d3_set_stage(const sorr_ios_data_layout *layout, int stage)
{
    sorr_ios_d3_stage = stage;
    sorr_ios_d3_log(layout, "stage=%s ticks=%u", sorr_ios_d3_stage_name(stage), SDL_GetTicks());
}

static int sorr_ios_read_last_nonempty_line(const char *path, char *out, size_t out_size)
{
    FILE *fp;
    char line[1024];
    int found = 0;

    if (!path || !out || out_size == 0)
    {
        return 0;
    }

    out[0] = '\0';
    fp = fopen(path, "rb");
    if (!fp)
    {
        return 0;
    }

    while (fgets(line, sizeof(line), fp))
    {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[--len] = '\0';
        }

        if (len > 0)
        {
            snprintf(out, out_size, "%s", line);
            found = 1;
        }
    }

    fclose(fp);
    return found;
}

static unsigned long long sorr_ios_d3_resident_memory_bytes(void)
{
#if defined(__APPLE__)
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
    {
        return (unsigned long long)info.resident_size;
    }
#endif
    return 0;
}

static Uint32 sorr_ios_d3_heartbeat_timer(Uint32 interval, void *param)
{
    const sorr_ios_data_layout *layout = (const sorr_ios_data_layout *)param;
    unsigned int heartbeat = ++sorr_ios_d3_heartbeat_count;
    Uint32 ticks = SDL_GetTicks();
    Uint32 loop_start = sorr_ios_d3_runtime_loop_start_ticks;
    Uint32 runtime_ms = loop_start ? ticks - loop_start : 0;
    Uint32 next_interval = SORR_IOS_D3_HEARTBEAT_NORMAL_MS;
    unsigned long long rss = sorr_ios_d3_resident_memory_bytes();

    (void)interval;

    if (runtime_ms >= SORR_IOS_D3_DENSE_START_MS && runtime_ms <= SORR_IOS_D3_DENSE_END_MS)
    {
        next_interval = SORR_IOS_D3_HEARTBEAT_DENSE_MS;
        if (!sorr_ios_d3_dense_window_marker)
        {
            sorr_ios_d3_dense_window_marker = 1;
            sorr_ios_d3_stability_log(layout,
                                      "dense_window_start ticks=%u runtime_ms=%u stage=%s",
                                      ticks,
                                      runtime_ms,
                                      sorr_ios_d3_stage_name(sorr_ios_d3_stage));
        }
    }
    else if (runtime_ms > SORR_IOS_D3_DENSE_END_MS && sorr_ios_d3_dense_window_marker == 1)
    {
        sorr_ios_d3_dense_window_marker = 2;
        sorr_ios_d3_stability_log(layout,
                                  "dense_window_end ticks=%u runtime_ms=%u stage=%s",
                                  ticks,
                                  runtime_ms,
                                  sorr_ios_d3_stage_name(sorr_ios_d3_stage));
    }

    if (!sorr_ios_d3_first_frame_marker && frame_count > 0)
    {
        sorr_ios_d3_first_frame_marker = 1;
        sorr_ios_d3_stability_log(layout,
                                  "first_frame_detected ticks=%u runtime_ms=%u frame_count=%u last_frame_ticks=%d",
                                  ticks,
                                  runtime_ms,
                                  frame_count,
                                  last_frame_ticks);
    }

    sorr_ios_d3_stability_log(layout,
                              "heartbeat=%u ticks=%u runtime_ms=%u interval_next_ms=%u stage=%s rss_bytes=%llu frame_count=%u last_frame_ticks=%d frame_ms=%.3f fps_count=%d fps_init=%d max_jump=%d jump=%d instances=%d render_objects=%d render_object_creates=%u render_object_destroys=%u render_invalid_callbacks=%u opened_files=%d x_files=%d max_x_files=%d runtime_loops=%u runtime_frames=%u runtime_runs=%u runtime_created=%u runtime_destroyed=%u runtime_snapshots=%u runtime_last_proc=%s#%u:s%d:f%d:o%d audio_stub_zero=%u audio_stub_minus_one=%u audio_init_attempts=%u audio_init_ok=%u audio_init_fail=%u audio_wav_load_ok=%u audio_wav_load_fail=%u audio_wav_play=%u audio_inert_handles=%u audio_queue_clears=%u audio_music_load_attempts=%u audio_music_open_ok=%u audio_music_open_fail=%u audio_music_mem_ok=%u audio_music_mem_fail=%u audio_music_play_attempts=%u audio_music_play_ok=%u audio_music_play_fail=%u audio_music_controls=%u audio_music_queries=%u audio_music_free=%u audio_music_halt=%u audio_music_playing=%u audio_music_last_handle=%llu audio_music_last_ptr=0x%llx audio_music_last_bytes=%llu audio_music_total_bytes=%llu audio_live_handles=%u audio_live_wav=%u audio_live_inert_wav=%u audio_live_music=%u audio_max_live_handles=%u audio_zero_music_play=%u audio_zero_music_control=%u audio_zero_music_query=%u audio_zero_wav_control=%u audio_zero_wav_query=%u audio_zero_wav_volume=%u audio_zero_channel_effect=%u audio_zero_play_wav_guard=%u audio_music_last_status=%s audio_music_last_path=%s runtime_snapshot=%s runtime_lifecycle=%s",
                              heartbeat,
                              ticks,
                              runtime_ms,
                              next_interval,
                              sorr_ios_d3_stage_name(sorr_ios_d3_stage),
                              rss,
                              frame_count,
                              last_frame_ticks,
                              frame_ms,
                              FPS_count,
                              FPS_init,
                              max_jump,
                              jump,
                              sorr_ios_d3_live_instance_count,
                              sorr_ios_d3_render_object_count,
                              sorr_ios_d3_render_instance_object_created_count,
                              sorr_ios_d3_render_instance_object_destroyed_count,
                              sorr_ios_d3_render_invalid_callback_count,
                              opened_files,
                              x_files_count,
                              max_x_files,
                              sorr_ios_d3_instance_go_loop_count,
                              sorr_ios_d3_frame_complete_count,
                              sorr_ios_d3_instance_run_count,
                              sorr_ios_d3_instance_created_count,
                              sorr_ios_d3_instance_destroyed_count,
                              sorr_ios_d3_snapshot_count,
                              sorr_ios_d3_last_proc_name,
                              sorr_ios_d3_last_proc_id,
                              sorr_ios_d3_last_proc_status,
                              sorr_ios_d3_last_proc_frame_percent,
                              sorr_ios_d3_last_proc_code_offset,
                              sorr_ios_sound_stub_zero_count,
                              sorr_ios_sound_stub_minus_one_count,
                              sorr_ios_audio_init_attempt_count,
                              sorr_ios_audio_init_ok_count,
                              sorr_ios_audio_init_fail_count,
                              sorr_ios_audio_wav_load_ok_count,
                              sorr_ios_audio_wav_load_fail_count,
                              sorr_ios_audio_wav_play_count,
                              sorr_ios_audio_inert_handle_count,
                              sorr_ios_audio_queue_clear_count,
                              sorr_ios_audio_music_load_attempt_count,
                              sorr_ios_audio_music_open_ok_count,
                              sorr_ios_audio_music_open_fail_count,
                              sorr_ios_audio_music_mem_load_ok_count,
                              sorr_ios_audio_music_mem_load_fail_count,
                              sorr_ios_audio_music_play_attempt_count,
                              sorr_ios_audio_music_play_ok_count,
                              sorr_ios_audio_music_play_fail_count,
                              sorr_ios_audio_music_control_count,
                              sorr_ios_audio_music_query_count,
                              sorr_ios_audio_music_free_count,
                              sorr_ios_audio_music_halt_count,
                              sorr_ios_audio_music_last_playing,
                              sorr_ios_audio_music_last_handle,
                              sorr_ios_audio_music_last_ptr,
                              sorr_ios_audio_music_last_bytes,
                              sorr_ios_audio_music_total_bytes,
                              sorr_ios_audio_live_handle_count,
                              sorr_ios_audio_live_wav_count,
                              sorr_ios_audio_live_inert_wav_count,
                              sorr_ios_audio_live_music_count,
                              sorr_ios_audio_max_live_handle_count,
                              sorr_ios_audio_zero_music_play_count,
                              sorr_ios_audio_zero_music_control_count,
                              sorr_ios_audio_zero_music_query_count,
                              sorr_ios_audio_zero_wav_control_count,
                              sorr_ios_audio_zero_wav_query_count,
                              sorr_ios_audio_zero_wav_volume_count,
                              sorr_ios_audio_zero_channel_effect_count,
                              sorr_ios_audio_zero_play_wav_guard_count,
                              sorr_ios_audio_last_music_status,
                              sorr_ios_audio_last_music_path,
                              sorr_ios_d3_runtime_snapshot,
                              sorr_ios_d3_lifecycle_events);
    return next_interval;
}

static const char *sorr_ios_d3_event_name(Uint32 type)
{
    switch (type)
    {
        case SDL_QUIT:
            return "SDL_QUIT";
        case SDL_APP_TERMINATING:
            return "SDL_APP_TERMINATING";
        case SDL_APP_LOWMEMORY:
            return "SDL_APP_LOWMEMORY";
        case SDL_APP_WILLENTERBACKGROUND:
            return "SDL_APP_WILLENTERBACKGROUND";
        case SDL_APP_DIDENTERBACKGROUND:
            return "SDL_APP_DIDENTERBACKGROUND";
        case SDL_APP_WILLENTERFOREGROUND:
            return "SDL_APP_WILLENTERFOREGROUND";
        case SDL_APP_DIDENTERFOREGROUND:
            return "SDL_APP_DIDENTERFOREGROUND";
        default:
            return "SDL_EVENT";
    }
}

static int sorr_ios_d3_event_watch(void *userdata, SDL_Event *event)
{
    const sorr_ios_data_layout *layout = (const sorr_ios_data_layout *)userdata;

    if (!event)
    {
        return 0;
    }

    switch (event->type)
    {
        case SDL_QUIT:
        case SDL_APP_TERMINATING:
        case SDL_APP_LOWMEMORY:
        case SDL_APP_WILLENTERBACKGROUND:
        case SDL_APP_DIDENTERBACKGROUND:
        case SDL_APP_WILLENTERFOREGROUND:
        case SDL_APP_DIDENTERFOREGROUND:
            sorr_ios_d3_stability_log(layout,
                                      "event=%s ticks=%u stage=%s runtime_loops=%u runtime_frames=%u runtime_runs=%u runtime_last_proc=%s#%u:s%d:f%d:o%d audio_music_last_status=%s audio_music_last_path=%s audio_music_play_attempts=%u audio_music_play_ok=%u audio_music_play_fail=%u audio_music_playing=%u audio_music_last_handle=%llu audio_music_last_ptr=0x%llx audio_music_last_bytes=%llu runtime_snapshot=%s runtime_lifecycle=%s",
                                      sorr_ios_d3_event_name(event->type),
                                      SDL_GetTicks(),
                                      sorr_ios_d3_stage_name(sorr_ios_d3_stage),
                                      sorr_ios_d3_instance_go_loop_count,
                                      sorr_ios_d3_frame_complete_count,
                                      sorr_ios_d3_instance_run_count,
                                      sorr_ios_d3_last_proc_name,
                                      sorr_ios_d3_last_proc_id,
                                      sorr_ios_d3_last_proc_status,
                                      sorr_ios_d3_last_proc_frame_percent,
                                      sorr_ios_d3_last_proc_code_offset,
                                      sorr_ios_audio_last_music_status,
                                      sorr_ios_audio_last_music_path,
                                      sorr_ios_audio_music_play_attempt_count,
                                      sorr_ios_audio_music_play_ok_count,
                                      sorr_ios_audio_music_play_fail_count,
                                      sorr_ios_audio_music_last_playing,
                                      sorr_ios_audio_music_last_handle,
                                      sorr_ios_audio_music_last_ptr,
                                      sorr_ios_audio_music_last_bytes,
                                      sorr_ios_d3_runtime_snapshot,
                                      sorr_ios_d3_lifecycle_events);
            break;

        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_CLOSE ||
                event->window.event == SDL_WINDOWEVENT_MINIMIZED ||
                event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            {
                sorr_ios_d3_stability_log(layout,
                                          "event=SDL_WINDOWEVENT code=%u ticks=%u stage=%s",
                                          (unsigned int)event->window.event,
                                          SDL_GetTicks(),
                                          sorr_ios_d3_stage_name(sorr_ios_d3_stage));
            }
            break;

        default:
            break;
    }

    return 0;
}

static void sorr_ios_set_d3_missing_data_status(const sorr_ios_data_layout *layout)
{
    sorr_ios_status_clear();
    sorr_ios_status_set_error();
    sorr_ios_status_add("D3 FIRST RENDER PROBE");
    sorr_ios_status_add("D2 DATA REQUIRED");
    sorr_ios_status_add("COMPLETE D2 IMPORT FIRST");
    sorr_ios_status_add("DATA APP SUPPORT/SORR");
    sorr_ios_status_add(layout && layout->d2_data_ready ? "D2 DATA READY" : "D2 DATA MISSING");
    sorr_ios_status_add("SORR.DAT NOT OPENED");
    sorr_ios_status_add("NO GAME EXECUTION");
    sorr_ios_status_add("NO GAME RENDERING");
}

static int sorr_ios_prepare_runtime_app_paths(const sorr_ios_data_layout *layout)
{
    char runtime_fullpath[1024];

    if (!layout)
    {
        return 0;
    }

    free(appexename);
    free(appexepath);
    free(appexefullpath);
    free(appname);

    appexename = sorr_ios_strdup("SorrIOSShell");
    appexepath = sorr_ios_strdup(layout->support_root);
    appname = sorr_ios_strdup("SorR.dat");

    if (!appexename || !appexepath || !appname)
    {
        return 0;
    }

    if (!sorr_ios_join_path(runtime_fullpath, sizeof(runtime_fullpath), layout->support_root, appexename))
    {
        return 0;
    }

    appexefullpath = sorr_ios_strdup(runtime_fullpath);
    return appexefullpath != NULL;
}

static int sorr_ios_run_d3_first_render(sorr_ios_data_layout *layout,
                                        SDL_Window **window_ref,
                                        SDL_Renderer **renderer_ref)
{
    char *runtime_argv[1];
    INSTANCE *mainproc_running;
    int ret;
    long sorr_dat_size = 0;
    long required_size = 0;
    SDL_TimerID heartbeat_timer = 0;
    char previous_stability_line[256] = "";
    char previous_status_line[SORR_IOS_STATUS_LINE_LEN];

    if (!layout)
    {
        return 1;
    }

    sorr_ios_copy_file_contents(layout->d3_stability_path, layout->d3_visible_stability_path);
    if (!sorr_ios_read_last_nonempty_line(layout->d3_stability_path,
                                          previous_stability_line,
                                          sizeof(previous_stability_line)))
    {
        sorr_ios_read_last_nonempty_line(layout->d3_visible_stability_path,
                                         previous_stability_line,
                                         sizeof(previous_stability_line));
    }

    remove(layout->d3_probe_path);
    sorr_ios_d3_set_stage(layout, SORR_IOS_D3_STAGE_APP_LAUNCH);
    sorr_ios_d3_log(layout, "probe log path=%s", layout->d3_probe_path);
    sorr_ios_d3_log(layout, "stability log path=%s", layout->d3_stability_path);
    sorr_ios_d3_log(layout, "visible stability log path=%s", layout->d3_visible_stability_path);
    snprintf(sorr_ios_d3_visible_event_log_path,
             1024,
             "%s",
             layout->d3_visible_stability_path);
#ifndef _WIN32
    sorr_ios_d3_install_signal_handlers(layout->d3_visible_stability_path);
#endif
    sorr_ios_d3_log(layout, "app support path=%s", layout->support_root);
    sorr_ios_d3_log(layout, "SorR.dat path=%s", layout->sorr_dat_path);
    if (previous_stability_line[0])
    {
        sorr_ios_d3_log(layout, "previous stability last marker=%s", previous_stability_line);
    }
    else
    {
        sorr_ios_d3_log(layout, "previous stability log unavailable");
    }

    sorr_ios_status_clear();
    sorr_ios_status_set_waiting();
    sorr_ios_status_add("D3 FIRST RENDER PROBE");
    sorr_ios_status_add("DATA APP SUPPORT/SORR");
    sorr_ios_status_add("DIAG FILES SORR_DIAGNOSTICS");
    if (previous_stability_line[0])
    {
        sorr_ios_status_add("PREV STABILITY LOG FOUND");
        snprintf(previous_status_line, sizeof(previous_status_line), "PREV %.88s", previous_stability_line);
        sorr_ios_status_add(previous_status_line);
    }

    sorr_ios_d3_set_stage(layout, SORR_IOS_D3_STAGE_PREFLIGHT);
    sorr_ios_d3_log(layout,
                    "D2 data preflight ready=%d import_seen=%d import_failed=%d",
                    layout->d2_data_ready ? 1 : 0,
                    layout->d2_import_seen ? 1 : 0,
                    layout->d2_import_failed ? 1 : 0);

    if (!layout->d2_data_ready)
    {
        sorr_ios_d3_log(layout, "D2 data not ready; refusing runtime execution");
        sorr_ios_set_d3_missing_data_status(layout);
        return 1;
    }

    if (!sorr_ios_open_file_probe("D3 SorR.dat", layout->sorr_dat_path, &sorr_dat_size))
    {
        sorr_ios_d3_log(layout, "SorR.dat open failed before runtime");
        sorr_ios_status_set_error();
        sorr_ios_status_add("SORR.DAT OPEN FAILED");
        return 1;
    }
    sorr_ios_status_add("SORR.DAT FOUND OPENED");

    if (!sorr_ios_open_file_probe("D3 required data file mod/system.txt",
                                  layout->required_file_path,
                                  &required_size))
    {
        sorr_ios_d3_log(layout, "mod/system.txt open failed before runtime");
        sorr_ios_status_set_error();
        sorr_ios_status_add("MOD/SYSTEM.TXT MISSING");
        return 1;
    }
    sorr_ios_status_add("MOD/SYSTEM.TXT FOUND");

    if (!sorr_ios_write_text_file(layout->d3_probe_path, "d3-first-render-probe-start\n"))
    {
        SDL_Log("SORR iOS shell: D3 probe initial write failed path=%s errno=%d",
                layout->d3_probe_path,
                errno);
        sorr_ios_status_set_error();
        sorr_ios_status_add("D3 PROBE LOG FAILED");
        return 1;
    }
    sorr_ios_d3_log(layout, "probe log write ok");

    SDL_AddEventWatch(sorr_ios_d3_event_watch, layout);
    heartbeat_timer = SDL_AddTimer(SORR_IOS_D3_HEARTBEAT_NORMAL_MS, sorr_ios_d3_heartbeat_timer, layout);
    if (heartbeat_timer)
    {
        sorr_ios_d3_log(layout,
                        "heartbeat timer started interval_ms=%u dense_start_ms=%u dense_end_ms=%u dense_interval_ms=%u",
                        SORR_IOS_D3_HEARTBEAT_NORMAL_MS,
                        SORR_IOS_D3_DENSE_START_MS,
                        SORR_IOS_D3_DENSE_END_MS,
                        SORR_IOS_D3_HEARTBEAT_DENSE_MS);
    }
    else
    {
        sorr_ios_d3_log(layout, "heartbeat timer start failed error=%s", SDL_GetError());
    }

    SDL_setenv("OS_ID", "0", 1);
    SDL_setenv("SORR_PORTABLE_PUMP_EVENTS", "1", 1);
    SDL_setenv("SORR_PORTABLE_DIAG", "1", 1);
    SDL_setenv("SORR_PORTABLE_DIAG_FILES", "1", 1);
    SDL_setenv("SORR_PORTABLE_DIAG_LOOP", "1", 1);
    SDL_setenv("SORR_PORTABLE_DIAG_RENDER", "1", 1);
    SDL_setenv("SORR_PORTABLE_DIAG_VIDEO", "1", 1);
    SDL_setenv("SDL_RENDER_DRIVER", "opengles2", 0);

    if (chdir(layout->support_root) != 0)
    {
        sorr_ios_d3_log(layout, "chdir failed path=%s errno=%d", layout->support_root, errno);
        sorr_ios_status_set_error();
        sorr_ios_status_add("CHDIR FAILED");
        return 1;
    }
    sorr_ios_d3_log(layout, "chdir ok path=%s", layout->support_root);

    sorr_ios_d3_set_stage(layout, SORR_IOS_D3_STAGE_RUNTIME_PATHS);
    if (!sorr_ios_prepare_runtime_app_paths(layout))
    {
        sorr_ios_d3_log(layout, "runtime app path setup failed");
        sorr_ios_status_set_error();
        sorr_ios_status_add("RUNTIME PATH SETUP FAILED");
        return 1;
    }

    file_addp(layout->support_root);
    file_addp(".");
    sorr_ios_d3_log(layout, "runtime file search paths added");

    sorr_ios_status_add("RUNTIME INIT START");
    sorr_ios_d3_log(layout, "runtime init start");
    string_init();
    init_c_type();

    sorr_ios_d3_set_stage(layout, SORR_IOS_D3_STAGE_DCB_LOAD);
    if (!dcb_load("SorR.dat"))
    {
        sorr_ios_d3_log(layout, "dcb_load failed for SorR.dat");
        sorr_ios_status_set_error();
        sorr_ios_status_add("DCB LOAD FAILED");
        return 1;
    }
    sorr_ios_d3_log(layout, "dcb_load ok");

    sorr_ios_d3_set_stage(layout, SORR_IOS_D3_STAGE_SYSPROC);
    sysproc_init();
    sorr_ios_d3_log(layout, "sysproc_init ok");

    sorr_ios_d3_set_stage(layout, SORR_IOS_D3_STAGE_RUNTIME_HANDOFF);
    runtime_argv[0] = "SorR.dat";
    bgdrtm_entry(1, runtime_argv);
    sorr_ios_d3_log(layout, "runtime init end mainproc=%p", mainproc);

    if (!mainproc)
    {
        sorr_ios_status_set_error();
        sorr_ios_status_add("MAINPROC MISSING");
        return 1;
    }

    sorr_ios_status_add("RUNTIME INIT OK");
    sorr_ios_status_add("HANDOFF TO RENDER");
    sorr_ios_d3_log(layout, "first script execution start mainproc=%s", mainproc->name ? mainproc->name : "(unnamed)");
    mainproc_running = instance_new(mainproc, NULL);
    sorr_ios_d3_log(layout, "instance_new returned %p", mainproc_running);
    if (!mainproc_running)
    {
        sorr_ios_status_set_error();
        sorr_ios_status_add("MAIN INSTANCE FAILED");
        return 1;
    }

    if (renderer_ref && *renderer_ref)
    {
        sorr_ios_draw_status(*renderer_ref);
        SDL_RenderPresent(*renderer_ref);
        SDL_Delay(1000);
        SDL_DestroyRenderer(*renderer_ref);
        *renderer_ref = NULL;
    }

    if (window_ref && *window_ref)
    {
        SDL_DestroyWindow(*window_ref);
        *window_ref = NULL;
    }

    sorr_ios_d3_log(layout, "first frame/render loop handoff begin");
    sorr_ios_d3_set_stage(layout, SORR_IOS_D3_STAGE_RUNTIME_LOOP);
    sorr_ios_d3_runtime_loop_start_ticks = SDL_GetTicks();
    sorr_ios_d3_log(layout,
                    "runtime loop start ticks=%u dense_start_ms=%u dense_end_ms=%u",
                    sorr_ios_d3_runtime_loop_start_ticks,
                    SORR_IOS_D3_DENSE_START_MS,
                    SORR_IOS_D3_DENSE_END_MS);
    ret = instance_go_all();
    sorr_ios_d3_set_stage(layout, SORR_IOS_D3_STAGE_RUNTIME_RETURNED);
    sorr_ios_d3_log(layout, "instance_go_all returned ret=%d", ret);
    if (heartbeat_timer)
    {
        SDL_RemoveTimer(heartbeat_timer);
    }
    SDL_DelEventWatch(sorr_ios_d3_event_watch, layout);
    bgdrtm_exit(ret);
    return ret;
}
#endif

#ifndef _WIN32
static int sorr_ios_copy_file_if_needed(const char *src, const char *dst)
{
    FILE *in;
    FILE *out;
    char *buffer;
    size_t read_count;
    long src_size = 0;
    long dst_size = 0;

    if (!sorr_ios_file_size(src, &src_size))
    {
        SDL_Log("SORR iOS shell: D2 copy source file unavailable path=%s", src ? src : "(null)");
        return 0;
    }

    if (sorr_ios_file_size(dst, &dst_size) && dst_size == src_size)
    {
        return 1;
    }

    in = fopen(src, "rb");
    if (!in)
    {
        SDL_Log("SORR iOS shell: D2 copy source open failed path=%s errno=%d", src, errno);
        return 0;
    }

    out = fopen(dst, "wb");
    if (!out)
    {
        SDL_Log("SORR iOS shell: D2 copy destination open failed path=%s errno=%d", dst, errno);
        fclose(in);
        return 0;
    }

    buffer = (char *)malloc(65536);
    if (!buffer)
    {
        fclose(out);
        fclose(in);
        return 0;
    }

    while ((read_count = fread(buffer, 1, 65536, in)) > 0)
    {
        if (fwrite(buffer, 1, read_count, out) != read_count)
        {
            SDL_Log("SORR iOS shell: D2 copy write failed path=%s errno=%d", dst, errno);
            free(buffer);
            fclose(out);
            fclose(in);
            return 0;
        }
    }

    free(buffer);
    if (ferror(in))
    {
        SDL_Log("SORR iOS shell: D2 copy read failed path=%s errno=%d", src, errno);
        fclose(out);
        fclose(in);
        return 0;
    }

    fclose(out);
    fclose(in);
    return 1;
}

static int sorr_ios_copy_tree(const char *src_dir, const char *dst_dir)
{
    DIR *dir;
    struct dirent *entry;

    if (!sorr_ios_mkdir_if_needed(dst_dir))
    {
        SDL_Log("SORR iOS shell: D2 copy destination dir failed path=%s errno=%d", dst_dir, errno);
        return 0;
    }

    dir = opendir(src_dir);
    if (!dir)
    {
        SDL_Log("SORR iOS shell: D2 import source open failed path=%s errno=%d", src_dir, errno);
        return 0;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char src_path[1024];
        char dst_path[1024];
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        if (strcmp(entry->d_name, "README_D2_IMPORT.txt") == 0)
        {
            continue;
        }

        if (!sorr_ios_join_path(src_path, sizeof(src_path), src_dir, entry->d_name) ||
            !sorr_ios_join_path(dst_path, sizeof(dst_path), dst_dir, entry->d_name))
        {
            closedir(dir);
            return 0;
        }

        if (stat(src_path, &st) != 0)
        {
            SDL_Log("SORR iOS shell: D2 import stat failed path=%s errno=%d", src_path, errno);
            closedir(dir);
            return 0;
        }

        if (S_ISDIR(st.st_mode))
        {
            if (!sorr_ios_copy_tree(src_path, dst_path))
            {
                closedir(dir);
                return 0;
            }
        }
        else if (!sorr_ios_copy_file_if_needed(src_path, dst_path))
        {
            closedir(dir);
            return 0;
        }
    }

    closedir(dir);
    return 1;
}
#else
static int sorr_ios_copy_tree(const char *src_dir, const char *dst_dir)
{
    (void)src_dir;
    (void)dst_dir;
    SDL_Log("SORR iOS shell: D2 import copy is not implemented for this host preflight");
    return 0;
}
#endif

static int sorr_ios_prepare_data_layout(sorr_ios_data_layout *layout)
{
    char *base_path;
    const char *home;
    char documents_dir[1024];
    char library_dir[1024];
    char app_support_dir[1024];
    char diagnostics_readme_path[1024];

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

    if (!sorr_ios_join_path(documents_dir, sizeof(documents_dir), home, "Documents") ||
        !sorr_ios_join_path(layout->documents_root, sizeof(layout->documents_root), home, "Documents") ||
        !sorr_ios_join_path(library_dir, sizeof(library_dir), home, "Library") ||
        !sorr_ios_join_path(app_support_dir, sizeof(app_support_dir), library_dir, "Application Support") ||
        !sorr_ios_join_path(layout->support_root, sizeof(layout->support_root), app_support_dir, "SORR") ||
        !sorr_ios_join_path(layout->savegame_dir, sizeof(layout->savegame_dir), layout->support_root, "savegame") ||
        !sorr_ios_join_path(layout->xbox_dir, sizeof(layout->xbox_dir), layout->support_root, "xbox") ||
        !sorr_ios_join_path(layout->logs_dir, sizeof(layout->logs_dir), layout->support_root, "logs") ||
        !sorr_ios_join_path(layout->documents_import_dir, sizeof(layout->documents_import_dir), layout->documents_root, "SORR_IMPORT") ||
        !sorr_ios_join_path(layout->documents_diagnostics_dir, sizeof(layout->documents_diagnostics_dir), layout->documents_root, "SORR_DIAGNOSTICS") ||
        !sorr_ios_join_path(layout->sorr_dat_path, sizeof(layout->sorr_dat_path), layout->support_root, "SorR.dat") ||
        !sorr_ios_join_path(layout->required_file_path, sizeof(layout->required_file_path), layout->support_root, "mod/system.txt") ||
        !sorr_ios_join_path(layout->d2_probe_path, sizeof(layout->d2_probe_path), layout->logs_dir, "ios_d2_data_import_probe.txt") ||
        !sorr_ios_join_path(layout->d3_probe_path, sizeof(layout->d3_probe_path), layout->logs_dir, "ios_d3_first_render_probe.txt") ||
        !sorr_ios_join_path(layout->d3_stability_path, sizeof(layout->d3_stability_path), layout->logs_dir, "ios_d3_runtime_stability_probe.txt") ||
        !sorr_ios_join_path(layout->d3_visible_stability_path, sizeof(layout->d3_visible_stability_path), layout->documents_diagnostics_dir, "ios_d3_runtime_stability_probe.txt"))
    {
        SDL_Log("SORR iOS shell: data layout path construction failed");
        return 0;
    }

    if (!sorr_ios_mkdir_if_needed(documents_dir) ||
        !sorr_ios_mkdir_if_needed(layout->documents_root) ||
        !sorr_ios_mkdir_if_needed(layout->documents_import_dir) ||
        !sorr_ios_mkdir_if_needed(layout->documents_diagnostics_dir) ||
        !sorr_ios_mkdir_if_needed(library_dir) ||
        !sorr_ios_mkdir_if_needed(app_support_dir) ||
        !sorr_ios_mkdir_if_needed(layout->support_root))
    {
        SDL_Log("SORR iOS shell: support root create failed path=%s errno=%d",
                layout->support_root,
                errno);
        return 0;
    }

    SDL_Log("SORR iOS shell: support root path=%s", layout->support_root);
    SDL_Log("SORR iOS shell: D2 import inbox path=%s", layout->documents_import_dir);
    SDL_Log("SORR iOS shell: D3 visible diagnostics path=%s", layout->documents_diagnostics_dir);
    SDL_Log("SORR iOS shell: D2 canonical data path=%s", layout->support_root);

    if (sorr_ios_join_path(diagnostics_readme_path,
                           sizeof(diagnostics_readme_path),
                           layout->documents_diagnostics_dir,
                           "README_D3S_DIAGNOSTICS.txt"))
    {
        sorr_ios_write_text_file(diagnostics_readme_path,
                                 "D3S diagnostics are mirrored here for Files access.\n"
                                 "After an idle crash, reopen SorrIOSShell once, then copy the last lines of ios_d3_runtime_stability_probe.txt.\n"
                                 "For the five-minute idle issue, include dense_window_start through dense_window_end, runtime_snapshot, runtime_lifecycle, runtime_family_unlink, runtime_render_event, destroy_begin, last_lifecycle, last_family, last_render, and signal= lines when present.\n");
    }

    if (!sorr_ios_create_dir_marker("savegame", layout->savegame_dir) ||
        !sorr_ios_create_dir_marker("xbox", layout->xbox_dir) ||
        !sorr_ios_create_dir_marker("logs", layout->logs_dir))
    {
        return 0;
    }

    return sorr_ios_write_read_probe(layout->logs_dir);
}

static void sorr_ios_write_import_readme(const sorr_ios_data_layout *layout)
{
    char readme_path[1024];
    const char *readme_text =
        "SoRR iOS D2 SORR_IMPORT inbox\n"
        "\n"
        "Copy prepared SoRR data here. This folder is only an import inbox.\n"
        "\n"
        "Accepted direct layout:\n"
        "- SorR.dat\n"
        "- mod/system.txt\n"
        "- savegame/\n"
        "- xbox/\n"
        "\n"
        "Also accepted: one top-level folder that contains SorR.dat.\n"
        "Wrong: multiple top-level folders without SorR.dat, or nested two folders deep.\n"
        "\n"
        "The shell copies detected data into Library/Application Support/SORR.\n"
        "No game runtime is executed during D2.\n";

    if (!layout ||
        !sorr_ios_join_path(readme_path, sizeof(readme_path), layout->documents_import_dir, "README_D2_IMPORT.txt"))
    {
        return;
    }

    if (sorr_ios_write_text_file(readme_path, readme_text))
    {
        SDL_Log("SORR iOS shell: D2 import README available path=%s", readme_path);
    }
}

static int sorr_ios_dir_contains_sorr_dat(const char *dir)
{
    char candidate[1024];

    return dir &&
           sorr_ios_join_path(candidate, sizeof(candidate), dir, "SorR.dat") &&
           sorr_ios_file_size(candidate, NULL);
}

static sorr_ios_import_probe sorr_ios_detect_import_root(const sorr_ios_data_layout *layout)
{
    sorr_ios_import_probe probe;

    memset(&probe, 0, sizeof(probe));
    probe.layout = SORR_IOS_IMPORT_NONE;

    if (!layout)
    {
        return probe;
    }

    if (sorr_ios_dir_contains_sorr_dat(layout->documents_import_dir))
    {
        probe.layout = SORR_IOS_IMPORT_DIRECT;
        snprintf(probe.root, sizeof(probe.root), "%s", layout->documents_import_dir);
        return probe;
    }

#ifndef _WIN32
    {
        DIR *dir = opendir(layout->documents_import_dir);
        struct dirent *entry;
        char only_dir[1024] = "";
        int dir_count = 0;
        int file_count = 0;
        int nested_dat_seen = 0;

        if (!dir)
        {
            return probe;
        }

        while ((entry = readdir(dir)) != NULL)
        {
            char path[1024];
            struct stat st;

            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0 ||
                strcmp(entry->d_name, "README_D2_IMPORT.txt") == 0)
            {
                continue;
            }

            if (!sorr_ios_join_path(path, sizeof(path), layout->documents_import_dir, entry->d_name) ||
                stat(path, &st) != 0)
            {
                continue;
            }

            if (S_ISDIR(st.st_mode))
            {
                dir_count++;
                snprintf(only_dir, sizeof(only_dir), "%s", path);
                if (sorr_ios_dir_contains_sorr_dat(path))
                {
                    nested_dat_seen = 1;
                }
            }
            else
            {
                file_count++;
            }
        }

        closedir(dir);

        if (dir_count == 1 && nested_dat_seen)
        {
            probe.layout = SORR_IOS_IMPORT_NESTED_ONE_FOLDER;
            snprintf(probe.root, sizeof(probe.root), "%s", only_dir);
        }
        else if (dir_count > 1)
        {
            probe.layout = SORR_IOS_IMPORT_INVALID_MULTIPLE;
        }
        else if (dir_count == 1)
        {
            probe.layout = SORR_IOS_IMPORT_INVALID_NESTED;
        }
        else if (file_count > 0)
        {
            probe.layout = SORR_IOS_IMPORT_INVALID_MISSING_DAT;
        }
    }
#endif

    return probe;
}

static int sorr_ios_write_d2_probe(const sorr_ios_data_layout *layout)
{
    char readback[128];
    const char *proof_text = "sorr-ios-d2-sorr-import-ok\n";
    FILE *fp;

    if (!layout)
    {
        return 0;
    }

    fp = fopen(layout->d2_probe_path, "wb");
    if (!fp)
    {
        SDL_Log("SORR iOS shell: D2 probe write open failed path=%s errno=%d",
                layout->d2_probe_path,
                errno);
        return 0;
    }
    if (fwrite(proof_text, 1, strlen(proof_text), fp) != strlen(proof_text))
    {
        fclose(fp);
        SDL_Log("SORR iOS shell: D2 probe write failed path=%s errno=%d",
                layout->d2_probe_path,
                errno);
        return 0;
    }
    fclose(fp);

    fp = fopen(layout->d2_probe_path, "rb");
    if (!fp)
    {
        SDL_Log("SORR iOS shell: D2 probe read open failed path=%s errno=%d",
                layout->d2_probe_path,
                errno);
        return 0;
    }
    memset(readback, 0, sizeof(readback));
    if (!fgets(readback, sizeof(readback), fp))
    {
        fclose(fp);
        SDL_Log("SORR iOS shell: D2 probe read failed path=%s errno=%d",
                layout->d2_probe_path,
                errno);
        return 0;
    }
    fclose(fp);

    if (strcmp(readback, proof_text) != 0)
    {
        SDL_Log("SORR iOS shell: D2 probe read mismatch path=%s", layout->d2_probe_path);
        return 0;
    }

    SDL_Log("SORR iOS shell: D2 probe log write/read ok path=%s", layout->d2_probe_path);
    return 1;
}

static int sorr_ios_dir_write_read_probe(const char *dir, const char *name)
{
    char probe_path[1024];
    char readback[32];
    const char *proof_text = "ok\n";
    FILE *fp;

    if (!dir || !name ||
        !sorr_ios_mkdir_if_needed(dir) ||
        !sorr_ios_join_path(probe_path, sizeof(probe_path), dir, name))
    {
        return 0;
    }

    fp = fopen(probe_path, "wb");
    if (!fp)
    {
        return 0;
    }
    if (fwrite(proof_text, 1, strlen(proof_text), fp) != strlen(proof_text))
    {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    fp = fopen(probe_path, "rb");
    if (!fp)
    {
        return 0;
    }
    memset(readback, 0, sizeof(readback));
    if (!fgets(readback, sizeof(readback), fp))
    {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    remove(probe_path);

    return strcmp(readback, proof_text) == 0;
}

static void sorr_ios_run_d2_probe(sorr_ios_data_layout *layout)
{
    sorr_ios_import_probe import_probe;
    const char *staging_status = "not started";
    long sorr_dat_size = 0;
    long required_size = 0;
    int sorr_dat_ok;
    int required_ok;
    int probe_ok;
    int savegame_ok;
    int xbox_ok;
    int logs_ok;

    if (!layout)
    {
        return;
    }

    sorr_ios_status_clear();
    sorr_ios_status_add("D2 SORR DATA PROBE");
    sorr_ios_status_add("INBOX DOCUMENTS/SORR_IMPORT");
    sorr_ios_status_add("DATA APP SUPPORT/SORR");
    sorr_ios_write_import_readme(layout);

    import_probe = sorr_ios_detect_import_root(layout);
    SDL_Log("SORR iOS shell: D2 import layout detected=%s inbox=%s",
            sorr_ios_import_layout_name(import_probe.layout),
            layout->documents_import_dir);
    sorr_ios_status_add(sorr_ios_import_layout_status(import_probe.layout));

    if (import_probe.layout == SORR_IOS_IMPORT_DIRECT ||
        import_probe.layout == SORR_IOS_IMPORT_NESTED_ONE_FOLDER)
    {
        layout->d2_import_seen = true;
        SDL_Log("SORR iOS shell: D2 import started source=%s destination=%s",
                import_probe.root,
                layout->support_root);
        if (sorr_ios_copy_tree(import_probe.root, layout->support_root))
        {
            SDL_Log("SORR iOS shell: D2 import completed source=%s", import_probe.root);
            staging_status = "copied";
            layout->d2_import_failed = false;
        }
        else
        {
            SDL_Log("SORR iOS shell: D2 import failed source=%s", import_probe.root);
            staging_status = "failed";
            layout->d2_import_failed = true;
        }
    }
    else if (import_probe.layout == SORR_IOS_IMPORT_NONE)
    {
        SDL_Log("SORR iOS shell: D2 waiting for data import path=%s", layout->documents_import_dir);
        layout->d2_import_failed = false;
    }
    else
    {
        SDL_Log("SORR iOS shell: D2 import invalid layout=%s path=%s",
                sorr_ios_import_layout_name(import_probe.layout),
                layout->documents_import_dir);
        staging_status = "failed";
        layout->d2_import_failed = true;
    }

    sorr_dat_ok = sorr_ios_open_file_probe("D2 SorR.dat", layout->sorr_dat_path, &sorr_dat_size);
    required_ok = sorr_ios_open_file_probe("D2 required data file mod/system.txt",
                                           layout->required_file_path,
                                           &required_size);
    savegame_ok = sorr_ios_dir_write_read_probe(layout->savegame_dir, ".ios_d2_savegame_probe");
    xbox_ok = sorr_ios_dir_write_read_probe(layout->xbox_dir, ".ios_d2_xbox_probe");
    logs_ok = sorr_ios_dir_write_read_probe(layout->logs_dir, ".ios_d2_logs_probe");
    probe_ok = sorr_ios_write_d2_probe(layout);

    if (strcmp(staging_status, "not started") == 0 && sorr_dat_ok && required_ok)
    {
        staging_status = "existing";
    }

    SDL_Log("SORR iOS shell: D2 staging result=%s", staging_status);
    if (strcmp(staging_status, "copied") == 0)
    {
        sorr_ios_status_add("STAGING COPIED");
    }
    else if (strcmp(staging_status, "existing") == 0)
    {
        sorr_ios_status_add("STAGING EXISTING OK");
    }
    else if (strcmp(staging_status, "failed") == 0)
    {
        sorr_ios_status_add("STAGING FAILED");
    }
    else
    {
        sorr_ios_status_add("STAGING NOT STARTED");
    }

    if (sorr_dat_ok)
    {
        sorr_ios_status_add("SORR.DAT FOUND OPENED");
    }
    else
    {
        sorr_ios_status_add("SORR.DAT NOT FOUND");
    }

    if (required_ok)
    {
        sorr_ios_status_add("MOD/SYSTEM.TXT FOUND");
    }
    else
    {
        sorr_ios_status_add("MOD/SYSTEM.TXT MISSING");
    }

    if (probe_ok)
    {
        sorr_ios_status_add("PROBE LOG OK");
    }
    else
    {
        sorr_ios_status_add("PROBE LOG FAILED");
    }

    sorr_ios_status_add(savegame_ok ? "SAVEGAME WRITABLE" : "SAVEGAME NOT WRITABLE");
    sorr_ios_status_add(xbox_ok ? "XBOX WRITABLE" : "XBOX NOT WRITABLE");
    sorr_ios_status_add(logs_ok ? "LOGS WRITABLE" : "LOGS NOT WRITABLE");
    sorr_ios_status_add("NO GAME EXECUTION");
    sorr_ios_status_add("NO GAME RENDERING");
    layout->d2_data_ready = (sorr_dat_ok && required_ok && probe_ok && savegame_ok && xbox_ok && logs_ok && !layout->d2_import_failed);

    if (layout->d2_data_ready)
    {
        SDL_Log("SORR iOS shell: D2 data import/storage proof ready SorR.dat=%ld required=%ld",
                sorr_dat_size,
                required_size);
        sorr_ios_status_set_ready();
    }
    else if (layout->d2_import_failed)
    {
        sorr_ios_status_set_error();
    }
    else
    {
        sorr_ios_status_set_waiting();
    }

    SDL_Log("SORR iOS shell: D2 SorR.dat intentionally not loaded or executed");
}

#ifndef SORR_IOS_D3_FIRST_RENDER
static int sorr_ios_runtime_entry_probe(int argc, char **argv)
{
    SDL_Log("SORR iOS shell: reached D2 runtime skip probe argc=%d argv0=%s",
            argc,
            (argc > 0 && argv && argv[0]) ? argv[0] : "(null)");
    (void)bgdrtm_entry;
    SDL_Log("SORR iOS shell: Bennu runtime intentionally skipped in D2");
    SDL_Log("SORR iOS shell: SorR.dat intentionally not loaded or executed in D2");
    return 0;
}
#endif

int main(int argc, char *argv[])
{
    sorr_ios_data_layout data_layout;
    Uint32 last_d2_probe_ticks = 0;

#ifdef SDL_MAIN_HANDLED
    SDL_SetMainReady();
#endif

    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    SDL_Log("SORR iOS shell: app entry");

#if defined(SDL_HINT_IDLE_TIMER_DISABLED)
    SDL_SetHint(SDL_HINT_IDLE_TIMER_DISABLED, "1");
#elif defined(SDL_HINT_IOS_IDLE_TIMER_DISABLED)
    SDL_SetHint(SDL_HINT_IOS_IDLE_TIMER_DISABLED, "1");
#endif

    if (SDL_Init(0) != 0)
    {
        SDL_Log("SORR iOS shell: SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Log("SORR iOS shell: SDL_Init ok");
#if defined(SDL_HINT_IDLE_TIMER_DISABLED) || defined(SDL_HINT_IOS_IDLE_TIMER_DISABLED)
    SDL_Log("SORR iOS shell: iOS idle timer disabled for D3 stability");
#endif

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
    sorr_ios_run_d2_probe(&data_layout);
    last_d2_probe_ticks = SDL_GetTicks();

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

#ifdef SORR_IOS_D3_FIRST_RENDER
    if (data_layout.d2_data_ready)
    {
        if (sorr_ios_run_d3_first_render(&data_layout, &window, &renderer) != 0)
        {
            SDL_Log("SORR iOS shell: D3 first render probe failed before runtime handoff");
        }
    }
    else
    {
        sorr_ios_set_d3_missing_data_status(&data_layout);
        SDL_Log("SORR iOS shell: D3 waiting for D2-staged data before runtime execution");
    }
#else
    if (sorr_ios_runtime_entry_probe(argc, argv) != 0)
    {
        SDL_Log("SORR iOS shell: runtime handoff probe failed");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
#endif

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

#ifndef SORR_IOS_D3_FIRST_RENDER
        if (SDL_GetTicks() - last_d2_probe_ticks > 2000)
        {
            sorr_ios_run_d2_probe(&data_layout);
            last_d2_probe_ticks = SDL_GetTicks();
        }
#else
        (void)last_d2_probe_ticks;
        if (!data_layout.d2_data_ready && SDL_GetTicks() - last_d2_probe_ticks > 2000)
        {
            sorr_ios_run_d2_probe(&data_layout);
            if (data_layout.d2_data_ready)
            {
                sorr_ios_status_clear();
                sorr_ios_status_set_ready();
                sorr_ios_status_add("D3 DATA READY");
                sorr_ios_status_add("RELAUNCH APP TO RENDER");
                sorr_ios_status_add("NO GAME EXECUTION");
                sorr_ios_status_add("NO GAME RENDERING");
            }
            else
            {
                sorr_ios_set_d3_missing_data_status(&data_layout);
            }
            last_d2_probe_ticks = SDL_GetTicks();
        }
#endif

        if (renderer)
        {
            sorr_ios_draw_status(renderer);
            SDL_RenderPresent(renderer);
        }
        SDL_Delay(16);
    }

    SDL_Log("SORR iOS shell: clean shutdown");
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
    }
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    free(globaldata);
    globaldata = NULL;
    return 0;
}
