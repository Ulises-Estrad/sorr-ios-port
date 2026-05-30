#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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

#ifndef SORR_IOS_BUILD_LABEL
#define SORR_IOS_BUILD_LABEL "ios-playtest-fallback-crash-report"
#endif

#ifndef SORR_IOS_ARTIFACT_LABEL
#define SORR_IOS_ARTIFACT_LABEL "ios-shell-playtest-fallback-crash-report-device-arm64"
#endif

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
    char d3_current_run_path[1024];
    char d3_previous_run_path[1024];
    char d3_latest_crash_report_path[1024];
    char d3_private_current_run_path[1024];
    char d3_private_previous_run_path[1024];
    char d3_private_latest_crash_report_path[1024];
    char d4_touch_config_path[1024];
    char audio_sfx_diagnostics_path[1024];
    bool d2_data_ready;
    bool d2_import_seen;
    bool d2_import_failed;
} sorr_ios_data_layout;

#ifdef SORR_IOS_D3_FIRST_RENDER
static void sorr_ios_d3_stability_log(const sorr_ios_data_layout *layout, const char *format, ...);
#else
static void sorr_ios_d3_stability_log(const sorr_ios_data_layout *layout, const char *format, ...)
{
    (void)layout;
    (void)format;
}
#endif

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

#ifdef TARGET_IOS
#ifdef SORR_IOS_D3_FIRST_RENDER
void sorr_ios_touch_set_bennu_key(int code, int pressed);
#else
static void sorr_ios_touch_set_bennu_key(int code, int pressed)
{
    (void)code;
    (void)pressed;
}
#endif

#define SORR_IOS_D4A_TOUCH_BUTTON_COUNT 7
#define SORR_IOS_D4A_TOUCH_FINGER_COUNT 16

#define SORR_IOS_D4A_DPAD_CENTER_X 0.18f
#define SORR_IOS_D4A_DPAD_CENTER_Y 0.76f
#define SORR_IOS_D4A_DPAD_RADIUS_X 0.105f
#define SORR_IOS_D4A_DPAD_RADIUS_Y 0.205f
#define SORR_IOS_D4A_DPAD_DEADZONE 0.28f
#define SORR_IOS_D4A_DPAD_AXIS_THRESHOLD 0.28f
#define SORR_IOS_D4A_DPAD_DIAGONAL_RATIO 1.15f
#define SORR_IOS_D4A_JOYSTICK_CAPTURE_SCALE 1.45f
#define SORR_IOS_D4A_JOYSTICK_DEADZONE 0.22f
#define SORR_IOS_D4A_JOYSTICK_AXIS_THRESHOLD 0.30f
#define SORR_IOS_D4A_JOYSTICK_DIAGONAL_RATIO 0.58f
#define SORR_IOS_D4A_CONFIG_HIT_SLOP 0.016f
#define SORR_IOS_D4A_TOUCH_MOUSE_SUPPRESS_MS 450
#define SORR_IOS_D4A_CONTROL_CONFIG_VERSION 6
#define SORR_IOS_D4A_UTILITY_BUTTON_HIT_SLOP 0.012f
#define SORR_IOS_D4A_EDIT_DRAG_THRESHOLD 0.018f

#define SORR_IOS_D4A_DPAD_UP 1
#define SORR_IOS_D4A_DPAD_DOWN 2
#define SORR_IOS_D4A_DPAD_LEFT 4
#define SORR_IOS_D4A_DPAD_RIGHT 8

#define SORR_IOS_D4A_EDIT_TARGET_NONE -1
#define SORR_IOS_D4A_EDIT_TARGET_DPAD 1000
#define SORR_IOS_D4A_MIN_BUTTON_W 0.055f
#define SORR_IOS_D4A_MIN_BUTTON_H 0.055f
#define SORR_IOS_D4A_MAX_BUTTON_W 0.28f
#define SORR_IOS_D4A_MAX_BUTTON_H 0.22f
#define SORR_IOS_D4A_MIN_DPAD_RX 0.065f
#define SORR_IOS_D4A_MAX_DPAD_RX 0.18f
#define SORR_IOS_D4A_MIN_DPAD_RY 0.12f
#define SORR_IOS_D4A_MAX_DPAD_RY 0.28f
#define SORR_IOS_D4A_BUTTON_START 4
#define SORR_IOS_D4A_BUTTON_BACK 5
#define SORR_IOS_D4A_BUTTON_BACK_ATTACK 6

typedef struct sorr_ios_d4a_touch_button
{
    const char *name;
    const char *label;
    int primary_key;
    int secondary_key;
    float x;
    float y;
    float w;
    float h;
} sorr_ios_d4a_touch_button;

typedef struct sorr_ios_d4a_touch_finger
{
    SDL_FingerID finger_id;
    int button_index;
    int is_dpad;
    int is_config;
    int edit_target;
    int active;
    int drag_started;
    float drag_dx;
    float drag_dy;
    float start_x;
    float start_y;
} sorr_ios_d4a_touch_finger;

typedef struct sorr_ios_d4a_rectf
{
    float x;
    float y;
    float w;
    float h;
} sorr_ios_d4a_rectf;

typedef struct sorr_ios_d4a_control_layout
{
    float dpad_center_x;
    float dpad_center_y;
    float dpad_radius_x;
    float dpad_radius_y;
    float opacity;
    int overlay_visible;
    int labels_visible;
    int config_version;
    int initialized;
    int loaded_from_disk;
    sorr_ios_d4a_rectf buttons[SORR_IOS_D4A_TOUCH_BUTTON_COUNT];
} sorr_ios_d4a_control_layout;

static const sorr_ios_d4a_touch_button sorr_ios_d4a_touch_buttons[SORR_IOS_D4A_TOUCH_BUTTON_COUNT] = {
    {"Attack", "ATK", 46, -1, 0.800f, 0.620f, 0.065f, 0.140f},
    {"Jump", "JUMP", 47, -1, 0.875f, 0.620f, 0.065f, 0.140f},
    {"Special", "SPC", 45, -1, 0.800f, 0.790f, 0.065f, 0.140f},
    {"Police", "POL", 48, -1, 0.875f, 0.790f, 0.065f, 0.140f},
    {"Start", "START", 28, -1, 0.921f, 0.095f, 0.055f, 0.052f},
    {"Back", "BACK", 1, 14, 0.921f, 0.220f, 0.055f, 0.052f},
    {"Back Attack", "B-ATK", 32, -1, 0.800f, 0.440f, 0.065f, 0.140f}
};

static int sorr_ios_d4a_button_press_count[SORR_IOS_D4A_TOUCH_BUTTON_COUNT];
static sorr_ios_d4a_touch_finger sorr_ios_d4a_touch_fingers[SORR_IOS_D4A_TOUCH_FINGER_COUNT];
static const sorr_ios_data_layout *sorr_ios_d4a_active_layout = NULL;
static sorr_ios_d4a_control_layout sorr_ios_d4a_controls;
static int sorr_ios_d4a_dpad_active = 0;
static SDL_FingerID sorr_ios_d4a_dpad_finger_id = 0;
static int sorr_ios_d4a_dpad_mask = 0;
static float sorr_ios_d4a_joystick_norm_x = 0.0f;
static float sorr_ios_d4a_joystick_norm_y = 0.0f;
static Uint32 sorr_ios_d4a_ignore_mouse_until_ticks = 0;
static int sorr_ios_d4a_edit_mode = 0;
static int sorr_ios_d4a_selected_target = SORR_IOS_D4A_EDIT_TARGET_NONE;

static float sorr_ios_d4a_clampf(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static void sorr_ios_d4a_touch_log(const char *format, ...)
{
    char line[384];
    va_list args;

    if (!format)
    {
        return;
    }

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    SDL_Log("SORR iOS shell: D4a %s", line);
    if (sorr_ios_d4a_active_layout)
    {
        sorr_ios_d3_stability_log(sorr_ios_d4a_active_layout, "%s", line);
    }
}

static int sorr_ios_d4a_button_is_utility(int index)
{
    return index == SORR_IOS_D4A_BUTTON_START || index == SORR_IOS_D4A_BUTTON_BACK;
}

static void sorr_ios_d4a_apply_button_default(int index)
{
    if (index < 0 || index >= SORR_IOS_D4A_TOUCH_BUTTON_COUNT)
    {
        return;
    }

    sorr_ios_d4a_controls.buttons[index].x = sorr_ios_d4a_touch_buttons[index].x;
    sorr_ios_d4a_controls.buttons[index].y = sorr_ios_d4a_touch_buttons[index].y;
    sorr_ios_d4a_controls.buttons[index].w = sorr_ios_d4a_touch_buttons[index].w;
    sorr_ios_d4a_controls.buttons[index].h = sorr_ios_d4a_touch_buttons[index].h;
}

static void sorr_ios_d4a_apply_button_shape_preserve_center(int index)
{
    float center_x;
    float center_y;

    if (index < 0 || index >= SORR_IOS_D4A_TOUCH_BUTTON_COUNT)
    {
        return;
    }

    center_x = sorr_ios_d4a_controls.buttons[index].x + sorr_ios_d4a_controls.buttons[index].w * 0.5f;
    center_y = sorr_ios_d4a_controls.buttons[index].y + sorr_ios_d4a_controls.buttons[index].h * 0.5f;
    sorr_ios_d4a_controls.buttons[index].w = sorr_ios_d4a_touch_buttons[index].w;
    sorr_ios_d4a_controls.buttons[index].h = sorr_ios_d4a_touch_buttons[index].h;
    sorr_ios_d4a_controls.buttons[index].x = center_x - sorr_ios_d4a_controls.buttons[index].w * 0.5f;
    sorr_ios_d4a_controls.buttons[index].y = center_y - sorr_ios_d4a_controls.buttons[index].h * 0.5f;
}

static void sorr_ios_d4a_apply_version5_button_defaults(void)
{
    int i;

    for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
    {
        if (!sorr_ios_d4a_button_is_utility(i) && i != SORR_IOS_D4A_BUTTON_BACK_ATTACK)
        {
            sorr_ios_d4a_apply_button_shape_preserve_center(i);
        }
    }
    sorr_ios_d4a_apply_button_default(SORR_IOS_D4A_BUTTON_BACK_ATTACK);
}

static void sorr_ios_d4a_apply_version6_button_defaults(void)
{
    sorr_ios_d4a_apply_button_default(SORR_IOS_D4A_BUTTON_BACK_ATTACK);
    sorr_ios_d4a_apply_button_default(0);
    sorr_ios_d4a_apply_button_default(1);
    sorr_ios_d4a_apply_button_default(2);
    sorr_ios_d4a_apply_button_default(3);
}

static void sorr_ios_d4a_reset_control_defaults(void)
{
    int i;

    memset(&sorr_ios_d4a_controls, 0, sizeof(sorr_ios_d4a_controls));
    sorr_ios_d4a_controls.dpad_center_x = SORR_IOS_D4A_DPAD_CENTER_X;
    sorr_ios_d4a_controls.dpad_center_y = SORR_IOS_D4A_DPAD_CENTER_Y;
    sorr_ios_d4a_controls.dpad_radius_x = SORR_IOS_D4A_DPAD_RADIUS_X;
    sorr_ios_d4a_controls.dpad_radius_y = SORR_IOS_D4A_DPAD_RADIUS_Y;
    sorr_ios_d4a_controls.opacity = 0.70f;
    sorr_ios_d4a_controls.overlay_visible = 1;
    sorr_ios_d4a_controls.labels_visible = 0;
    sorr_ios_d4a_controls.config_version = SORR_IOS_D4A_CONTROL_CONFIG_VERSION;
    sorr_ios_d4a_controls.initialized = 1;
    for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
    {
        sorr_ios_d4a_apply_button_default(i);
    }
}

static void sorr_ios_d4a_clamp_control_layout(void)
{
    int i;

    sorr_ios_d4a_controls.dpad_radius_x = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.dpad_radius_x, SORR_IOS_D4A_MIN_DPAD_RX, SORR_IOS_D4A_MAX_DPAD_RX);
    sorr_ios_d4a_controls.dpad_radius_y = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.dpad_radius_y, SORR_IOS_D4A_MIN_DPAD_RY, SORR_IOS_D4A_MAX_DPAD_RY);
    sorr_ios_d4a_controls.dpad_center_x = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.dpad_center_x, sorr_ios_d4a_controls.dpad_radius_x, 1.0f - sorr_ios_d4a_controls.dpad_radius_x);
    sorr_ios_d4a_controls.dpad_center_y = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.dpad_center_y, sorr_ios_d4a_controls.dpad_radius_y, 1.0f - sorr_ios_d4a_controls.dpad_radius_y);
    sorr_ios_d4a_controls.opacity = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.opacity, 0.20f, 1.0f);
    sorr_ios_d4a_controls.overlay_visible = 1;
    sorr_ios_d4a_controls.labels_visible = sorr_ios_d4a_controls.labels_visible ? 1 : 0;
    if (sorr_ios_d4a_controls.config_version <= 0)
    {
        sorr_ios_d4a_controls.config_version = SORR_IOS_D4A_CONTROL_CONFIG_VERSION;
    }

    for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
    {
        sorr_ios_d4a_controls.buttons[i].w = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.buttons[i].w, SORR_IOS_D4A_MIN_BUTTON_W, SORR_IOS_D4A_MAX_BUTTON_W);
        sorr_ios_d4a_controls.buttons[i].h = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.buttons[i].h, SORR_IOS_D4A_MIN_BUTTON_H, SORR_IOS_D4A_MAX_BUTTON_H);
        sorr_ios_d4a_controls.buttons[i].x = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.buttons[i].x, 0.0f, 1.0f - sorr_ios_d4a_controls.buttons[i].w);
        sorr_ios_d4a_controls.buttons[i].y = sorr_ios_d4a_clampf(sorr_ios_d4a_controls.buttons[i].y, 0.0f, 1.0f - sorr_ios_d4a_controls.buttons[i].h);
    }
}

static void sorr_ios_d4a_apply_utility_button_defaults(void)
{
    sorr_ios_d4a_apply_button_default(SORR_IOS_D4A_BUTTON_START);
    sorr_ios_d4a_apply_button_default(SORR_IOS_D4A_BUTTON_BACK);
}

static void sorr_ios_d4a_save_control_config(void)
{
    FILE *fp;
    int i;

    if (!sorr_ios_d4a_active_layout || !sorr_ios_d4a_active_layout->d4_touch_config_path[0])
    {
        return;
    }

    sorr_ios_d4a_clamp_control_layout();
    fp = fopen(sorr_ios_d4a_active_layout->d4_touch_config_path, "wb");
    if (!fp)
    {
        sorr_ios_d4a_touch_log("control config save failed path=%s errno=%d",
                               sorr_ios_d4a_active_layout->d4_touch_config_path,
                               errno);
        return;
    }

    fprintf(fp, "version=%d\n", SORR_IOS_D4A_CONTROL_CONFIG_VERSION);
    fprintf(fp, "overlay_visible=%d\n", sorr_ios_d4a_controls.overlay_visible);
    fprintf(fp, "labels_visible=%d\n", sorr_ios_d4a_controls.labels_visible);
    fprintf(fp, "opacity=%.4f\n", sorr_ios_d4a_controls.opacity);
    fprintf(fp, "dpad_center_x=%.4f\n", sorr_ios_d4a_controls.dpad_center_x);
    fprintf(fp, "dpad_center_y=%.4f\n", sorr_ios_d4a_controls.dpad_center_y);
    fprintf(fp, "dpad_radius_x=%.4f\n", sorr_ios_d4a_controls.dpad_radius_x);
    fprintf(fp, "dpad_radius_y=%.4f\n", sorr_ios_d4a_controls.dpad_radius_y);
    for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
    {
        fprintf(fp, "button%d_x=%.4f\n", i, sorr_ios_d4a_controls.buttons[i].x);
        fprintf(fp, "button%d_y=%.4f\n", i, sorr_ios_d4a_controls.buttons[i].y);
        fprintf(fp, "button%d_w=%.4f\n", i, sorr_ios_d4a_controls.buttons[i].w);
        fprintf(fp, "button%d_h=%.4f\n", i, sorr_ios_d4a_controls.buttons[i].h);
    }
    fclose(fp);
    sorr_ios_d4a_touch_log("control config saved path=%s", sorr_ios_d4a_active_layout->d4_touch_config_path);
}

static void sorr_ios_d4a_apply_config_value(const char *key, const char *value)
{
    int int_value;
    float float_value;
    int index;
    char axis;

    if (!key || !value)
    {
        return;
    }

    if (strcmp(key, "version") == 0 && sscanf(value, "%d", &int_value) == 1)
    {
        sorr_ios_d4a_controls.config_version = int_value;
    }
    else if (strcmp(key, "overlay_visible") == 0 && sscanf(value, "%d", &int_value) == 1)
    {
        sorr_ios_d4a_controls.overlay_visible = int_value ? 1 : 0;
    }
    else if (strcmp(key, "labels_visible") == 0 && sscanf(value, "%d", &int_value) == 1)
    {
        sorr_ios_d4a_controls.labels_visible = int_value ? 1 : 0;
    }
    else if (strcmp(key, "opacity") == 0 && sscanf(value, "%f", &float_value) == 1)
    {
        sorr_ios_d4a_controls.opacity = float_value;
    }
    else if (strcmp(key, "dpad_center_x") == 0 && sscanf(value, "%f", &float_value) == 1)
    {
        sorr_ios_d4a_controls.dpad_center_x = float_value;
    }
    else if (strcmp(key, "dpad_center_y") == 0 && sscanf(value, "%f", &float_value) == 1)
    {
        sorr_ios_d4a_controls.dpad_center_y = float_value;
    }
    else if (strcmp(key, "dpad_radius_x") == 0 && sscanf(value, "%f", &float_value) == 1)
    {
        sorr_ios_d4a_controls.dpad_radius_x = float_value;
    }
    else if (strcmp(key, "dpad_radius_y") == 0 && sscanf(value, "%f", &float_value) == 1)
    {
        sorr_ios_d4a_controls.dpad_radius_y = float_value;
    }
    else if (sscanf(key, "button%d_%c", &index, &axis) == 2 &&
             index >= 0 && index < SORR_IOS_D4A_TOUCH_BUTTON_COUNT &&
             sscanf(value, "%f", &float_value) == 1)
    {
        if (axis == 'x') sorr_ios_d4a_controls.buttons[index].x = float_value;
        else if (axis == 'y') sorr_ios_d4a_controls.buttons[index].y = float_value;
        else if (axis == 'w') sorr_ios_d4a_controls.buttons[index].w = float_value;
        else if (axis == 'h') sorr_ios_d4a_controls.buttons[index].h = float_value;
    }
}

static void sorr_ios_d4a_load_control_config(void)
{
    FILE *fp;
    char line[256];
    char key[96];
    char value[128];

    if (!sorr_ios_d4a_controls.initialized)
    {
        sorr_ios_d4a_reset_control_defaults();
    }
    if (sorr_ios_d4a_controls.loaded_from_disk)
    {
        return;
    }
    if (!sorr_ios_d4a_active_layout || !sorr_ios_d4a_active_layout->d4_touch_config_path[0])
    {
        return;
    }

    fp = fopen(sorr_ios_d4a_active_layout->d4_touch_config_path, "rb");
    if (!fp)
    {
        sorr_ios_d4a_controls.loaded_from_disk = 1;
        sorr_ios_d4a_save_control_config();
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        if (sscanf(line, " %95[^=]=%127s", key, value) == 2)
        {
            sorr_ios_d4a_apply_config_value(key, value);
        }
    }
    fclose(fp);
    if (sorr_ios_d4a_controls.config_version < SORR_IOS_D4A_CONTROL_CONFIG_VERSION)
    {
        int old_version = sorr_ios_d4a_controls.config_version;
        sorr_ios_d4a_controls.labels_visible = 0;
        if (sorr_ios_d4a_controls.config_version < 4)
        {
            sorr_ios_d4a_apply_utility_button_defaults();
        }
        if (sorr_ios_d4a_controls.config_version < 5)
        {
            sorr_ios_d4a_apply_version5_button_defaults();
        }
        if (sorr_ios_d4a_controls.config_version < 6)
        {
            sorr_ios_d4a_apply_version6_button_defaults();
        }
        sorr_ios_d4a_controls.config_version = SORR_IOS_D4A_CONTROL_CONFIG_VERSION;
        sorr_ios_d4a_touch_log("control config migrated old_version=%d version=%d labels_visible=0 circular_actions=1 back_attack=d action_column=aligned",
                               old_version,
                               SORR_IOS_D4A_CONTROL_CONFIG_VERSION);
    }
    sorr_ios_d4a_controls.loaded_from_disk = 1;
    sorr_ios_d4a_clamp_control_layout();
    sorr_ios_d4a_touch_log("control config loaded path=%s", sorr_ios_d4a_active_layout->d4_touch_config_path);
}

static void sorr_ios_d4a_ensure_control_config(void)
{
    if (!sorr_ios_d4a_controls.initialized)
    {
        sorr_ios_d4a_reset_control_defaults();
    }
    if (!sorr_ios_d4a_controls.loaded_from_disk)
    {
        sorr_ios_d4a_load_control_config();
    }
}

static void sorr_ios_d4a_set_active_layout(const sorr_ios_data_layout *layout)
{
    sorr_ios_d4a_active_layout = layout;
    sorr_ios_d4a_ensure_control_config();
}

static int sorr_ios_d4a_point_in_rect_slop(float x, float y, const sorr_ios_d4a_rectf *rect, float slop);

static int sorr_ios_d4a_button_for_point(float x, float y)
{
    int i;

    sorr_ios_d4a_ensure_control_config();
    for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
    {
        const sorr_ios_d4a_rectf *rect = &sorr_ios_d4a_controls.buttons[i];
        const sorr_ios_d4a_touch_button *button = &sorr_ios_d4a_touch_buttons[i];
        float slop = sorr_ios_d4a_button_is_utility(i) ? SORR_IOS_D4A_UTILITY_BUTTON_HIT_SLOP : 0.0f;
        (void)button;
        if (sorr_ios_d4a_point_in_rect_slop(x, y, rect, slop))
        {
            return i;
        }
    }

    return -1;
}

static int sorr_ios_d4a_dpad_mask_for_point_ex(float x, float y, int *inside, float *norm_x, float *norm_y);

static int sorr_ios_d4a_dpad_mask_for_point(float x, float y, int *inside)
{
    return sorr_ios_d4a_dpad_mask_for_point_ex(x, y, inside, NULL, NULL);
}

static int sorr_ios_d4a_dpad_mask_for_point_ex(float x, float y, int *inside, float *norm_x, float *norm_y)
{
    float dx;
    float dy;
    float ax;
    float ay;
    float dist_sq;
    float visual_x;
    float visual_y;
    int mask = 0;

    sorr_ios_d4a_ensure_control_config();
    dx = (x - sorr_ios_d4a_controls.dpad_center_x) / sorr_ios_d4a_controls.dpad_radius_x;
    dy = (y - sorr_ios_d4a_controls.dpad_center_y) / sorr_ios_d4a_controls.dpad_radius_y;
    ax = dx < 0.0f ? -dx : dx;
    ay = dy < 0.0f ? -dy : dy;
    dist_sq = dx * dx + dy * dy;

    visual_x = dx;
    visual_y = dy;
    if (dist_sq > 1.0f)
    {
        float inv_len = 1.0f / sqrtf(dist_sq);
        visual_x *= inv_len;
        visual_y *= inv_len;
    }
    if (norm_x) *norm_x = visual_x;
    if (norm_y) *norm_y = visual_y;

    if (inside)
    {
        float capture = SORR_IOS_D4A_JOYSTICK_CAPTURE_SCALE;
        *inside = dist_sq <= capture * capture;
    }

    if (dist_sq < SORR_IOS_D4A_JOYSTICK_DEADZONE * SORR_IOS_D4A_JOYSTICK_DEADZONE)
    {
        return 0;
    }

    if (ax >= ay)
    {
        if (ax >= SORR_IOS_D4A_JOYSTICK_AXIS_THRESHOLD)
        {
            mask |= dx < 0.0f ? SORR_IOS_D4A_DPAD_LEFT : SORR_IOS_D4A_DPAD_RIGHT;
            if (ay >= SORR_IOS_D4A_JOYSTICK_AXIS_THRESHOLD &&
                ay >= ax * SORR_IOS_D4A_JOYSTICK_DIAGONAL_RATIO)
            {
                mask |= dy < 0.0f ? SORR_IOS_D4A_DPAD_UP : SORR_IOS_D4A_DPAD_DOWN;
            }
        }
    }
    else
    {
        if (ay >= SORR_IOS_D4A_JOYSTICK_AXIS_THRESHOLD)
        {
            mask |= dy < 0.0f ? SORR_IOS_D4A_DPAD_UP : SORR_IOS_D4A_DPAD_DOWN;
            if (ax >= SORR_IOS_D4A_JOYSTICK_AXIS_THRESHOLD &&
                ax >= ay * SORR_IOS_D4A_JOYSTICK_DIAGONAL_RATIO)
            {
                mask |= dx < 0.0f ? SORR_IOS_D4A_DPAD_LEFT : SORR_IOS_D4A_DPAD_RIGHT;
            }
        }
    }

    return mask;
}

static void sorr_ios_d4a_apply_dpad_key(int old_mask, int new_mask, int bit, int key, const char *name, int pressed, const char *reason)
{
    (void)bit;
    sorr_ios_touch_set_bennu_key(key, pressed);
    sorr_ios_d4a_touch_log("joystick key=%s action=%s key=%d old_mask=%d new_mask=%d reason=%s",
                           name,
                           pressed ? "down" : "up",
                           key,
                           old_mask,
                           new_mask,
                           reason ? reason : "joystick");
}

static void sorr_ios_d4a_set_dpad_mask(int new_mask, const char *reason)
{
    int old_mask = sorr_ios_d4a_dpad_mask;

    if (old_mask == new_mask)
    {
        return;
    }

    sorr_ios_d4a_touch_log("joystick transition old_mask=%d new_mask=%d reason=%s",
                           old_mask,
                           new_mask,
                           reason ? reason : "joystick");
    if ((old_mask & SORR_IOS_D4A_DPAD_UP) && !(new_mask & SORR_IOS_D4A_DPAD_UP))
        sorr_ios_d4a_apply_dpad_key(old_mask, new_mask, SORR_IOS_D4A_DPAD_UP, 72, "up", 0, reason);
    if ((old_mask & SORR_IOS_D4A_DPAD_DOWN) && !(new_mask & SORR_IOS_D4A_DPAD_DOWN))
        sorr_ios_d4a_apply_dpad_key(old_mask, new_mask, SORR_IOS_D4A_DPAD_DOWN, 80, "down", 0, reason);
    if ((old_mask & SORR_IOS_D4A_DPAD_LEFT) && !(new_mask & SORR_IOS_D4A_DPAD_LEFT))
        sorr_ios_d4a_apply_dpad_key(old_mask, new_mask, SORR_IOS_D4A_DPAD_LEFT, 75, "left", 0, reason);
    if ((old_mask & SORR_IOS_D4A_DPAD_RIGHT) && !(new_mask & SORR_IOS_D4A_DPAD_RIGHT))
        sorr_ios_d4a_apply_dpad_key(old_mask, new_mask, SORR_IOS_D4A_DPAD_RIGHT, 77, "right", 0, reason);

    if (!(old_mask & SORR_IOS_D4A_DPAD_UP) && (new_mask & SORR_IOS_D4A_DPAD_UP))
        sorr_ios_d4a_apply_dpad_key(old_mask, new_mask, SORR_IOS_D4A_DPAD_UP, 72, "up", 1, reason);
    if (!(old_mask & SORR_IOS_D4A_DPAD_DOWN) && (new_mask & SORR_IOS_D4A_DPAD_DOWN))
        sorr_ios_d4a_apply_dpad_key(old_mask, new_mask, SORR_IOS_D4A_DPAD_DOWN, 80, "down", 1, reason);
    if (!(old_mask & SORR_IOS_D4A_DPAD_LEFT) && (new_mask & SORR_IOS_D4A_DPAD_LEFT))
        sorr_ios_d4a_apply_dpad_key(old_mask, new_mask, SORR_IOS_D4A_DPAD_LEFT, 75, "left", 1, reason);
    if (!(old_mask & SORR_IOS_D4A_DPAD_RIGHT) && (new_mask & SORR_IOS_D4A_DPAD_RIGHT))
        sorr_ios_d4a_apply_dpad_key(old_mask, new_mask, SORR_IOS_D4A_DPAD_RIGHT, 77, "right", 1, reason);
    sorr_ios_d4a_dpad_mask = new_mask;
}

static int sorr_ios_d4a_find_finger(SDL_FingerID finger_id)
{
    int i;

    for (i = 0; i < SORR_IOS_D4A_TOUCH_FINGER_COUNT; i++)
    {
        if (sorr_ios_d4a_touch_fingers[i].active &&
            sorr_ios_d4a_touch_fingers[i].finger_id == finger_id)
        {
            return i;
        }
    }

    return -1;
}

static int sorr_ios_d4a_alloc_finger(SDL_FingerID finger_id)
{
    int i;

    for (i = 0; i < SORR_IOS_D4A_TOUCH_FINGER_COUNT; i++)
    {
        if (!sorr_ios_d4a_touch_fingers[i].active)
        {
            sorr_ios_d4a_touch_fingers[i].active = 1;
            sorr_ios_d4a_touch_fingers[i].finger_id = finger_id;
            sorr_ios_d4a_touch_fingers[i].button_index = -1;
            sorr_ios_d4a_touch_fingers[i].is_dpad = 0;
            sorr_ios_d4a_touch_fingers[i].is_config = 0;
            sorr_ios_d4a_touch_fingers[i].edit_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
            sorr_ios_d4a_touch_fingers[i].drag_started = 0;
            sorr_ios_d4a_touch_fingers[i].drag_dx = 0.0f;
            sorr_ios_d4a_touch_fingers[i].drag_dy = 0.0f;
            sorr_ios_d4a_touch_fingers[i].start_x = 0.0f;
            sorr_ios_d4a_touch_fingers[i].start_y = 0.0f;
            return i;
        }
    }

    return -1;
}

static void sorr_ios_d4a_set_button(int button_index, int pressed, const char *reason)
{
    const sorr_ios_d4a_touch_button *button;
    int was_pressed;
    int now_pressed;

    if (button_index < 0 || button_index >= SORR_IOS_D4A_TOUCH_BUTTON_COUNT)
    {
        return;
    }

    button = &sorr_ios_d4a_touch_buttons[button_index];
    was_pressed = sorr_ios_d4a_button_press_count[button_index] > 0;
    if (pressed)
    {
        sorr_ios_d4a_button_press_count[button_index]++;
    }
    else if (sorr_ios_d4a_button_press_count[button_index] > 0)
    {
        sorr_ios_d4a_button_press_count[button_index]--;
    }
    now_pressed = sorr_ios_d4a_button_press_count[button_index] > 0;

    if (was_pressed == now_pressed)
    {
        return;
    }

    sorr_ios_touch_set_bennu_key(button->primary_key, now_pressed);
    if (button->secondary_key >= 0)
    {
        sorr_ios_touch_set_bennu_key(button->secondary_key, now_pressed);
    }

    sorr_ios_d4a_touch_log("touch button=%s action=%s key=%d fallback=%d path=bennu reason=%s",
                           button->name,
                           now_pressed ? "down" : "up",
                           button->primary_key,
                           button->secondary_key,
                           reason ? reason : "touch");
}

static void sorr_ios_d4a_release_all(const char *reason)
{
    int i;

    for (i = 0; i < SORR_IOS_D4A_TOUCH_FINGER_COUNT; i++)
    {
        sorr_ios_d4a_touch_fingers[i].active = 0;
        sorr_ios_d4a_touch_fingers[i].button_index = -1;
        sorr_ios_d4a_touch_fingers[i].is_dpad = 0;
        sorr_ios_d4a_touch_fingers[i].is_config = 0;
        sorr_ios_d4a_touch_fingers[i].edit_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
        sorr_ios_d4a_touch_fingers[i].drag_started = 0;
        sorr_ios_d4a_touch_fingers[i].drag_dx = 0.0f;
        sorr_ios_d4a_touch_fingers[i].drag_dy = 0.0f;
        sorr_ios_d4a_touch_fingers[i].start_x = 0.0f;
        sorr_ios_d4a_touch_fingers[i].start_y = 0.0f;
    }
    sorr_ios_d4a_set_dpad_mask(0, reason ? reason : "release-all");
    sorr_ios_d4a_dpad_active = 0;
    sorr_ios_d4a_dpad_finger_id = 0;
    sorr_ios_d4a_joystick_norm_x = 0.0f;
    sorr_ios_d4a_joystick_norm_y = 0.0f;

    for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
    {
        while (sorr_ios_d4a_button_press_count[i] > 0)
        {
            sorr_ios_d4a_set_button(i, 0, reason ? reason : "release-all");
        }
    }
}

static int sorr_ios_d4a_has_pressed_controls(void)
{
    int i;

    if (sorr_ios_d4a_dpad_mask != 0 || sorr_ios_d4a_dpad_active)
    {
        return 1;
    }

    for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
    {
        if (sorr_ios_d4a_button_press_count[i] > 0)
        {
            return 1;
        }
    }

    return 0;
}

typedef enum sorr_ios_d4a_config_button
{
    SORR_IOS_D4A_CONFIG_NONE = -1,
    SORR_IOS_D4A_CONFIG_TOGGLE = 0,
    SORR_IOS_D4A_CONFIG_DONE,
    SORR_IOS_D4A_CONFIG_RESET,
    SORR_IOS_D4A_CONFIG_OPACITY,
    SORR_IOS_D4A_CONFIG_LABELS,
    SORR_IOS_D4A_CONFIG_BIGGER,
    SORR_IOS_D4A_CONFIG_SMALLER
} sorr_ios_d4a_config_button;

static sorr_ios_d4a_rectf sorr_ios_d4a_config_button_rect(sorr_ios_d4a_config_button button)
{
    sorr_ios_d4a_rectf rect = {0.024f, 0.095f, 0.055f, 0.052f};

    if (!sorr_ios_d4a_edit_mode)
    {
        return rect;
    }

    rect.x = 0.022f;
    rect.w = 0.065f;
    rect.h = 0.070f;
    switch (button)
    {
        case SORR_IOS_D4A_CONFIG_DONE:
            rect.y = 0.095f;
            break;
        case SORR_IOS_D4A_CONFIG_RESET:
            rect.y = 0.180f;
            break;
        case SORR_IOS_D4A_CONFIG_OPACITY:
            rect.y = 0.265f;
            break;
        case SORR_IOS_D4A_CONFIG_LABELS:
            rect.y = 0.350f;
            break;
        case SORR_IOS_D4A_CONFIG_BIGGER:
            rect.y = 0.435f;
            break;
        case SORR_IOS_D4A_CONFIG_SMALLER:
            rect.y = 0.520f;
            break;
        case SORR_IOS_D4A_CONFIG_TOGGLE:
        default:
            rect.y = 0.095f;
            break;
    }
    return rect;
}

static int sorr_ios_d4a_point_in_rect(float x, float y, const sorr_ios_d4a_rectf *rect)
{
    return rect &&
           x >= rect->x && x <= rect->x + rect->w &&
           y >= rect->y && y <= rect->y + rect->h;
}

static int sorr_ios_d4a_point_in_rect_slop(float x, float y, const sorr_ios_d4a_rectf *rect, float slop)
{
    return rect &&
           x >= rect->x - slop && x <= rect->x + rect->w + slop &&
           y >= rect->y - slop && y <= rect->y + rect->h + slop;
}

static sorr_ios_d4a_config_button sorr_ios_d4a_config_button_for_point(float x, float y)
{
    if (!sorr_ios_d4a_edit_mode)
    {
        sorr_ios_d4a_rectf cfg = sorr_ios_d4a_config_button_rect(SORR_IOS_D4A_CONFIG_TOGGLE);
        return sorr_ios_d4a_point_in_rect_slop(x, y, &cfg, SORR_IOS_D4A_CONFIG_HIT_SLOP) ? SORR_IOS_D4A_CONFIG_TOGGLE : SORR_IOS_D4A_CONFIG_NONE;
    }

    {
        sorr_ios_d4a_config_button buttons[] = {
            SORR_IOS_D4A_CONFIG_DONE,
            SORR_IOS_D4A_CONFIG_RESET,
            SORR_IOS_D4A_CONFIG_OPACITY,
            SORR_IOS_D4A_CONFIG_LABELS,
            SORR_IOS_D4A_CONFIG_BIGGER,
            SORR_IOS_D4A_CONFIG_SMALLER
        };
        int i;
        for (i = 0; i < (int)(sizeof(buttons) / sizeof(buttons[0])); i++)
        {
            sorr_ios_d4a_rectf rect = sorr_ios_d4a_config_button_rect(buttons[i]);
            if (sorr_ios_d4a_point_in_rect_slop(x, y, &rect, SORR_IOS_D4A_CONFIG_HIT_SLOP))
            {
                return buttons[i];
            }
        }
    }

    return SORR_IOS_D4A_CONFIG_NONE;
}

static int sorr_ios_d4a_edit_target_for_point(float x, float y, float *drag_dx, float *drag_dy)
{
    int i;
    int dpad_inside = 0;

    (void)sorr_ios_d4a_dpad_mask_for_point(x, y, &dpad_inside);
    if (dpad_inside)
    {
        if (drag_dx) *drag_dx = x - sorr_ios_d4a_controls.dpad_center_x;
        if (drag_dy) *drag_dy = y - sorr_ios_d4a_controls.dpad_center_y;
        return SORR_IOS_D4A_EDIT_TARGET_DPAD;
    }

    for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
    {
        float slop = sorr_ios_d4a_button_is_utility(i) ? SORR_IOS_D4A_UTILITY_BUTTON_HIT_SLOP : 0.0f;
        if (sorr_ios_d4a_point_in_rect_slop(x, y, &sorr_ios_d4a_controls.buttons[i], slop))
        {
            if (drag_dx) *drag_dx = x - sorr_ios_d4a_controls.buttons[i].x;
            if (drag_dy) *drag_dy = y - sorr_ios_d4a_controls.buttons[i].y;
            return i;
        }
    }

    return SORR_IOS_D4A_EDIT_TARGET_NONE;
}

static void sorr_ios_d4a_resize_selected(float factor)
{
    if (sorr_ios_d4a_selected_target == SORR_IOS_D4A_EDIT_TARGET_DPAD)
    {
        sorr_ios_d4a_controls.dpad_radius_x *= factor;
        sorr_ios_d4a_controls.dpad_radius_y *= factor;
    }
    else if (sorr_ios_d4a_selected_target >= 0 &&
             sorr_ios_d4a_selected_target < SORR_IOS_D4A_TOUCH_BUTTON_COUNT)
    {
        sorr_ios_d4a_controls.buttons[sorr_ios_d4a_selected_target].w *= factor;
        sorr_ios_d4a_controls.buttons[sorr_ios_d4a_selected_target].h *= factor;
    }
    else
    {
        sorr_ios_d4a_touch_log("config resize ignored no selection factor=%.2f", factor);
        return;
    }
    sorr_ios_d4a_clamp_control_layout();
    sorr_ios_d4a_save_control_config();
    sorr_ios_d4a_touch_log("config resize selected=%d factor=%.2f", sorr_ios_d4a_selected_target, factor);
}

static void sorr_ios_d4a_cycle_opacity(void)
{
    if (sorr_ios_d4a_controls.opacity < 0.45f)
    {
        sorr_ios_d4a_controls.opacity = 0.55f;
    }
    else if (sorr_ios_d4a_controls.opacity < 0.70f)
    {
        sorr_ios_d4a_controls.opacity = 0.80f;
    }
    else if (sorr_ios_d4a_controls.opacity < 0.90f)
    {
        sorr_ios_d4a_controls.opacity = 1.0f;
    }
    else
    {
        sorr_ios_d4a_controls.opacity = 0.35f;
    }
    sorr_ios_d4a_save_control_config();
    sorr_ios_d4a_touch_log("config opacity=%.2f", sorr_ios_d4a_controls.opacity);
}

static void sorr_ios_d4a_handle_config_button(sorr_ios_d4a_config_button button)
{
    switch (button)
    {
        case SORR_IOS_D4A_CONFIG_TOGGLE:
            sorr_ios_d4a_release_all("config-enter");
            sorr_ios_d4a_edit_mode = 1;
            sorr_ios_d4a_selected_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
            sorr_ios_d4a_touch_log("config edit_mode=1");
            break;
        case SORR_IOS_D4A_CONFIG_DONE:
            sorr_ios_d4a_edit_mode = 0;
            sorr_ios_d4a_release_all("config-done");
            sorr_ios_d4a_save_control_config();
            sorr_ios_d4a_touch_log("config edit_mode=0 saved");
            break;
        case SORR_IOS_D4A_CONFIG_RESET:
            sorr_ios_d4a_release_all("config-reset");
            sorr_ios_d4a_reset_control_defaults();
            sorr_ios_d4a_controls.loaded_from_disk = 1;
            sorr_ios_d4a_selected_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
            sorr_ios_d4a_save_control_config();
            sorr_ios_d4a_touch_log("config reset defaults");
            break;
        case SORR_IOS_D4A_CONFIG_OPACITY:
            sorr_ios_d4a_cycle_opacity();
            break;
        case SORR_IOS_D4A_CONFIG_LABELS:
            sorr_ios_d4a_controls.labels_visible = !sorr_ios_d4a_controls.labels_visible;
            sorr_ios_d4a_save_control_config();
            sorr_ios_d4a_touch_log("config labels_visible=%d", sorr_ios_d4a_controls.labels_visible);
            break;
        case SORR_IOS_D4A_CONFIG_BIGGER:
            sorr_ios_d4a_resize_selected(1.12f);
            break;
        case SORR_IOS_D4A_CONFIG_SMALLER:
            sorr_ios_d4a_resize_selected(0.88f);
            break;
        default:
            break;
    }
}

static void sorr_ios_d4a_update_finger(SDL_FingerID finger_id, float x, float y, int is_down, const char *reason)
{
    int finger_slot = sorr_ios_d4a_find_finger(finger_id);
    int dpad_inside = 0;
    float joystick_norm_x = 0.0f;
    float joystick_norm_y = 0.0f;
    int new_dpad_mask = is_down ? sorr_ios_d4a_dpad_mask_for_point_ex(x, y, &dpad_inside, &joystick_norm_x, &joystick_norm_y) : 0;
    int new_button = -1;
    int old_button = -1;
    int is_motion = reason && strcmp(reason, "move") == 0;
    sorr_ios_d4a_config_button config_button = SORR_IOS_D4A_CONFIG_NONE;

    sorr_ios_d4a_ensure_control_config();
    if (finger_slot < 0 && is_down)
    {
        finger_slot = sorr_ios_d4a_alloc_finger(finger_id);
    }
    if (finger_slot < 0)
    {
        if (!is_down && finger_id == (SDL_FingerID)-1 && sorr_ios_d4a_has_pressed_controls())
        {
            sorr_ios_d4a_release_all("mouse-up-no-slot");
        }
        return;
    }

    if (is_down && !is_motion && !sorr_ios_d4a_touch_fingers[finger_slot].is_config)
    {
        config_button = sorr_ios_d4a_config_button_for_point(x, y);
        if (config_button != SORR_IOS_D4A_CONFIG_NONE)
        {
            sorr_ios_d4a_touch_fingers[finger_slot].is_config = 1;
            sorr_ios_d4a_touch_fingers[finger_slot].edit_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
            sorr_ios_d4a_handle_config_button(config_button);
            return;
        }
    }

    if (sorr_ios_d4a_edit_mode || sorr_ios_d4a_touch_fingers[finger_slot].is_config)
    {
        if (!is_down)
        {
            if (sorr_ios_d4a_touch_fingers[finger_slot].is_config &&
                sorr_ios_d4a_touch_fingers[finger_slot].edit_target != SORR_IOS_D4A_EDIT_TARGET_NONE)
            {
                sorr_ios_d4a_save_control_config();
            }
            sorr_ios_d4a_touch_fingers[finger_slot].active = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].button_index = -1;
            sorr_ios_d4a_touch_fingers[finger_slot].is_dpad = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].is_config = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].edit_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_started = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_dx = 0.0f;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_dy = 0.0f;
            sorr_ios_d4a_touch_fingers[finger_slot].start_x = 0.0f;
            sorr_ios_d4a_touch_fingers[finger_slot].start_y = 0.0f;
            return;
        }

        if (sorr_ios_d4a_touch_fingers[finger_slot].is_config)
        {
            int target = sorr_ios_d4a_touch_fingers[finger_slot].edit_target;
            float move_dx = x - sorr_ios_d4a_touch_fingers[finger_slot].start_x;
            float move_dy = y - sorr_ios_d4a_touch_fingers[finger_slot].start_y;
            float move_dist_sq = move_dx * move_dx + move_dy * move_dy;
            float threshold_sq = SORR_IOS_D4A_EDIT_DRAG_THRESHOLD * SORR_IOS_D4A_EDIT_DRAG_THRESHOLD;
            if (target != SORR_IOS_D4A_EDIT_TARGET_NONE &&
                !sorr_ios_d4a_touch_fingers[finger_slot].drag_started)
            {
                if (move_dist_sq < threshold_sq)
                {
                    return;
                }
                sorr_ios_d4a_touch_fingers[finger_slot].drag_started = 1;
                sorr_ios_d4a_touch_log("config drag_start selected=%d", target);
            }
            if (target == SORR_IOS_D4A_EDIT_TARGET_DPAD)
            {
                sorr_ios_d4a_controls.dpad_center_x = x - sorr_ios_d4a_touch_fingers[finger_slot].drag_dx;
                sorr_ios_d4a_controls.dpad_center_y = y - sorr_ios_d4a_touch_fingers[finger_slot].drag_dy;
                sorr_ios_d4a_clamp_control_layout();
            }
            else if (target >= 0 && target < SORR_IOS_D4A_TOUCH_BUTTON_COUNT)
            {
                sorr_ios_d4a_controls.buttons[target].x = x - sorr_ios_d4a_touch_fingers[finger_slot].drag_dx;
                sorr_ios_d4a_controls.buttons[target].y = y - sorr_ios_d4a_touch_fingers[finger_slot].drag_dy;
                sorr_ios_d4a_clamp_control_layout();
            }
            return;
        }

        {
            float drag_dx = 0.0f;
            float drag_dy = 0.0f;
            int target = sorr_ios_d4a_edit_target_for_point(x, y, &drag_dx, &drag_dy);
            if (!is_motion && target != SORR_IOS_D4A_EDIT_TARGET_NONE &&
                target == sorr_ios_d4a_selected_target)
            {
                sorr_ios_d4a_selected_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
                sorr_ios_d4a_touch_fingers[finger_slot].is_config = 1;
                sorr_ios_d4a_touch_fingers[finger_slot].edit_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
                sorr_ios_d4a_touch_fingers[finger_slot].drag_started = 0;
                sorr_ios_d4a_touch_fingers[finger_slot].start_x = x;
                sorr_ios_d4a_touch_fingers[finger_slot].start_y = y;
                sorr_ios_d4a_touch_log("config deselected=%d", target);
                return;
            }
            sorr_ios_d4a_touch_fingers[finger_slot].is_config = 1;
            sorr_ios_d4a_touch_fingers[finger_slot].edit_target = target;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_started = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_dx = drag_dx;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_dy = drag_dy;
            sorr_ios_d4a_touch_fingers[finger_slot].start_x = x;
            sorr_ios_d4a_touch_fingers[finger_slot].start_y = y;
            if (target != SORR_IOS_D4A_EDIT_TARGET_NONE)
            {
                sorr_ios_d4a_selected_target = target;
                sorr_ios_d4a_touch_log("config selected=%d", target);
            }
            return;
        }
    }

    if (is_down && dpad_inside && (!sorr_ios_d4a_dpad_active || sorr_ios_d4a_dpad_finger_id == finger_id) &&
        sorr_ios_d4a_touch_fingers[finger_slot].button_index < 0)
    {
        sorr_ios_d4a_touch_fingers[finger_slot].is_dpad = 1;
        sorr_ios_d4a_dpad_active = 1;
        sorr_ios_d4a_dpad_finger_id = finger_id;
    }

    if (sorr_ios_d4a_touch_fingers[finger_slot].is_dpad)
    {
        if (is_down)
        {
            sorr_ios_d4a_joystick_norm_x = joystick_norm_x;
            sorr_ios_d4a_joystick_norm_y = joystick_norm_y;
            sorr_ios_d4a_set_dpad_mask(new_dpad_mask, reason ? reason : "joystick");
        }
        else
        {
            sorr_ios_d4a_set_dpad_mask(0, reason ? reason : "joystick-release");
            sorr_ios_d4a_dpad_active = 0;
            sorr_ios_d4a_dpad_finger_id = 0;
            sorr_ios_d4a_joystick_norm_x = 0.0f;
            sorr_ios_d4a_joystick_norm_y = 0.0f;
            sorr_ios_d4a_touch_fingers[finger_slot].active = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].button_index = -1;
            sorr_ios_d4a_touch_fingers[finger_slot].is_dpad = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].is_config = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].edit_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_started = 0;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_dx = 0.0f;
            sorr_ios_d4a_touch_fingers[finger_slot].drag_dy = 0.0f;
            sorr_ios_d4a_touch_fingers[finger_slot].start_x = 0.0f;
            sorr_ios_d4a_touch_fingers[finger_slot].start_y = 0.0f;
        }
        return;
    }

    new_button = is_down ? sorr_ios_d4a_button_for_point(x, y) : -1;
    old_button = sorr_ios_d4a_touch_fingers[finger_slot].button_index;
    if (old_button != new_button)
    {
        if (old_button >= 0)
        {
            sorr_ios_d4a_set_button(old_button, 0, reason);
        }
        if (new_button >= 0)
        {
            sorr_ios_d4a_set_button(new_button, 1, reason);
        }
        sorr_ios_d4a_touch_fingers[finger_slot].button_index = new_button;
    }

    if (!is_down)
    {
        sorr_ios_d4a_touch_fingers[finger_slot].active = 0;
        sorr_ios_d4a_touch_fingers[finger_slot].button_index = -1;
        sorr_ios_d4a_touch_fingers[finger_slot].is_dpad = 0;
        sorr_ios_d4a_touch_fingers[finger_slot].is_config = 0;
        sorr_ios_d4a_touch_fingers[finger_slot].edit_target = SORR_IOS_D4A_EDIT_TARGET_NONE;
        sorr_ios_d4a_touch_fingers[finger_slot].drag_started = 0;
        sorr_ios_d4a_touch_fingers[finger_slot].drag_dx = 0.0f;
        sorr_ios_d4a_touch_fingers[finger_slot].drag_dy = 0.0f;
        sorr_ios_d4a_touch_fingers[finger_slot].start_x = 0.0f;
        sorr_ios_d4a_touch_fingers[finger_slot].start_y = 0.0f;
    }
}

void sorr_ios_d4a_process_sdl_event(const SDL_Event *event)
{
    float x = 0.0f;
    float y = 0.0f;

    if (!event)
    {
        return;
    }

    switch (event->type)
    {
        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION:
        case SDL_FINGERUP:
            x = event->tfinger.x;
            y = event->tfinger.y;
            sorr_ios_d4a_ignore_mouse_until_ticks = SDL_GetTicks() + SORR_IOS_D4A_TOUCH_MOUSE_SUPPRESS_MS;
            sorr_ios_d4a_update_finger(event->tfinger.fingerId,
                                       x,
                                       y,
                                       event->type != SDL_FINGERUP,
                                       event->type == SDL_FINGERMOTION ? "move" : "touch");
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            SDL_Window *focus = SDL_GetMouseFocus();
            int width = 1;
            int height = 1;
            if ((Sint32)(SDL_GetTicks() - sorr_ios_d4a_ignore_mouse_until_ticks) < 0)
            {
                if (event->type == SDL_MOUSEBUTTONUP && sorr_ios_d4a_has_pressed_controls())
                {
                    sorr_ios_d4a_release_all("mouse-up-shadow");
                }
                break;
            }
            if (focus)
            {
                SDL_GetWindowSize(focus, &width, &height);
            }
            if (width <= 0) width = 1;
            if (height <= 0) height = 1;
            x = (float)event->button.x / (float)width;
            y = (float)event->button.y / (float)height;
            sorr_ios_d4a_update_finger((SDL_FingerID)-1,
                                       x,
                                       y,
                                       event->type == SDL_MOUSEBUTTONDOWN,
                                       "mouse");
            break;
        }
        case SDL_MOUSEMOTION:
        {
            SDL_Window *focus = SDL_GetMouseFocus();
            int width = 1;
            int height = 1;
            if ((Sint32)(SDL_GetTicks() - sorr_ios_d4a_ignore_mouse_until_ticks) < 0)
            {
                break;
            }
            if (!(event->motion.state & SDL_BUTTON_LMASK))
            {
                break;
            }
            if (focus)
            {
                SDL_GetWindowSize(focus, &width, &height);
            }
            if (width <= 0) width = 1;
            if (height <= 0) height = 1;
            x = (float)event->motion.x / (float)width;
            y = (float)event->motion.y / (float)height;
            sorr_ios_d4a_update_finger((SDL_FingerID)-1,
                                       x,
                                       y,
                                       1,
                                       "move");
            break;
        }
        case SDL_APP_WILLENTERBACKGROUND:
        case SDL_APP_DIDENTERBACKGROUND:
        case SDL_APP_TERMINATING:
        case SDL_QUIT:
            sorr_ios_d4a_release_all("lifecycle");
            break;
        default:
            break;
    }
}

static Uint8 sorr_ios_d4a_alpha(int base)
{
    float value = (float)base * sorr_ios_d4a_controls.opacity;
    if (value < 20.0f)
    {
        value = 20.0f;
    }
    if (value > 255.0f)
    {
        value = 255.0f;
    }
    return (Uint8)value;
}

static void sorr_ios_d4a_draw_labeled_rect(SDL_Renderer *renderer,
                                           const SDL_Rect *rect,
                                           const char *label,
                                           int label_scale,
                                           int selected,
                                           int pressed,
                                           int use_config_alpha)
{
    SDL_Rect inner;
    int label_len;
    int text_w;
    int text_h;
    Uint8 fill_alpha = use_config_alpha ? sorr_ios_d4a_alpha(pressed ? 215 : 150) : (Uint8)(pressed ? 255 : 235);
    Uint8 line_alpha = use_config_alpha ? sorr_ios_d4a_alpha(250) : 250;

    if (!renderer || !rect)
    {
        return;
    }

    inner = *rect;
    inner.x += 2;
    inner.y += 2;
    inner.w -= 4;
    inner.h -= 4;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, fill_alpha);
    SDL_RenderFillRect(renderer, rect);
    SDL_SetRenderDrawColor(renderer,
                           (pressed || selected) ? 84 : 255,
                           (pressed || selected) ? 190 : 255,
                           255,
                           line_alpha);
    SDL_RenderDrawRect(renderer, rect);
    SDL_RenderDrawRect(renderer, &inner);

    if (label && (sorr_ios_d4a_controls.labels_visible || !use_config_alpha || sorr_ios_d4a_edit_mode || selected))
    {
        label_len = (int)strlen(label);
        text_w = label_len * 6 * label_scale;
        text_h = 7 * label_scale;
        sorr_ios_draw_text(renderer,
                           rect->x + (rect->w - text_w) / 2,
                           rect->y + (rect->h - text_h) / 2,
                           label_scale,
                           label);
    }
}

static void sorr_ios_d4a_draw_ellipse(SDL_Renderer *renderer,
                                      int cx,
                                      int cy,
                                      int rx,
                                      int ry,
                                      Uint8 r,
                                      Uint8 g,
                                      Uint8 b,
                                      Uint8 a,
                                      int filled)
{
    int y;

    if (!renderer || rx <= 0 || ry <= 0)
    {
        return;
    }

    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    for (y = -ry; y <= ry; y++)
    {
        float yf = (float)y / (float)ry;
        int x_span = (int)((float)rx * sqrtf(1.0f - yf * yf));
        if (filled)
        {
            SDL_RenderDrawLine(renderer, cx - x_span, cy + y, cx + x_span, cy + y);
        }
        else
        {
            SDL_RenderDrawPoint(renderer, cx - x_span, cy + y);
            SDL_RenderDrawPoint(renderer, cx + x_span, cy + y);
        }
    }
}

static void sorr_ios_d4a_draw_ellipse_outline(SDL_Renderer *renderer,
                                              int cx,
                                              int cy,
                                              int rx,
                                              int ry,
                                              int thickness,
                                              Uint8 r,
                                              Uint8 g,
                                              Uint8 b,
                                              Uint8 a)
{
    int i;

    if (thickness < 1)
    {
        thickness = 1;
    }
    for (i = 0; i < thickness; i++)
    {
        sorr_ios_d4a_draw_ellipse(renderer, cx, cy, rx - i, ry - i, r, g, b, a, 0);
    }
}

static void sorr_ios_d4a_draw_labeled_ellipse(SDL_Renderer *renderer,
                                              const SDL_Rect *rect,
                                              const char *label,
                                              int label_scale,
                                              int selected,
                                              int pressed,
                                              int use_config_alpha)
{
    int label_len;
    int text_w;
    int text_h;
    int cx;
    int cy;
    int rx;
    int ry;
    Uint8 fill_alpha = use_config_alpha ? sorr_ios_d4a_alpha(pressed ? 215 : 150) : (Uint8)(pressed ? 255 : 235);
    Uint8 line_alpha = use_config_alpha ? sorr_ios_d4a_alpha(250) : 250;

    if (!renderer || !rect)
    {
        return;
    }

    cx = rect->x + rect->w / 2;
    cy = rect->y + rect->h / 2;
    rx = rect->w / 2;
    ry = rect->h / 2;
    if (rx < 1 || ry < 1)
    {
        return;
    }

    sorr_ios_d4a_draw_ellipse(renderer, cx, cy, rx, ry, 0, 0, 0, fill_alpha, 1);
    sorr_ios_d4a_draw_ellipse_outline(renderer,
                                      cx,
                                      cy,
                                      rx,
                                      ry,
                                      selected ? 4 : 3,
                                      (pressed || selected) ? 84 : 255,
                                      (pressed || selected) ? 190 : 255,
                                      255,
                                      line_alpha);
    sorr_ios_d4a_draw_ellipse_outline(renderer,
                                      cx,
                                      cy,
                                      rx - 5,
                                      ry - 5,
                                      1,
                                      255,
                                      255,
                                      255,
                                      use_config_alpha ? sorr_ios_d4a_alpha(110) : 110);

    if (label && (sorr_ios_d4a_controls.labels_visible || !use_config_alpha || sorr_ios_d4a_edit_mode || selected))
    {
        label_len = (int)strlen(label);
        text_w = label_len * 6 * label_scale;
        while (label_scale > 1 && text_w > rect->w - 6)
        {
            label_scale--;
            text_w = label_len * 6 * label_scale;
        }
        text_h = 7 * label_scale;
        sorr_ios_draw_text(renderer,
                           rect->x + (rect->w - text_w) / 2,
                           rect->y + (rect->h - text_h) / 2,
                           label_scale,
                           label);
    }
}

static void sorr_ios_d4a_draw_config_button(SDL_Renderer *renderer,
                                            int width,
                                            int height,
                                            sorr_ios_d4a_config_button button,
                                            const char *label,
                                            int label_scale)
{
    sorr_ios_d4a_rectf nrect = sorr_ios_d4a_config_button_rect(button);
    SDL_Rect rect;

    rect.x = (int)(nrect.x * (float)width);
    rect.y = (int)(nrect.y * (float)height);
    rect.w = (int)(nrect.w * (float)width);
    rect.h = (int)(nrect.h * (float)height);
    if (rect.w < 52) rect.w = 52;
    if (rect.h < 36) rect.h = 36;
    sorr_ios_d4a_draw_labeled_rect(renderer, &rect, label, label_scale, 0, 0, 0);
}

void sorr_ios_d4a_draw_touch_overlay(SDL_Renderer *renderer)
{
    int width = 0;
    int height = 0;
    int old_logical_w = 0;
    int old_logical_h = 0;
    float old_scale_x = 1.0f;
    float old_scale_y = 1.0f;
    unsigned char old_r = 0;
    unsigned char old_g = 0;
    unsigned char old_b = 0;
    unsigned char old_a = 255;
    SDL_Rect old_viewport;
    SDL_Rect old_clip;
    SDL_bool old_clip_enabled = SDL_FALSE;
    int i;
    SDL_BlendMode old_blend = SDL_BLENDMODE_NONE;
    int margin;
    int label_scale;

    if (!renderer)
    {
        return;
    }

    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0 || width <= 0 || height <= 0)
    {
        return;
    }

    SDL_GetRenderDrawBlendMode(renderer, &old_blend);
    SDL_GetRenderDrawColor(renderer, &old_r, &old_g, &old_b, &old_a);
    SDL_RenderGetLogicalSize(renderer, &old_logical_w, &old_logical_h);
    SDL_RenderGetViewport(renderer, &old_viewport);
    old_clip_enabled = SDL_RenderIsClipEnabled(renderer);
    SDL_RenderGetClipRect(renderer, &old_clip);
    SDL_RenderGetScale(renderer, &old_scale_x, &old_scale_y);

    SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_RenderSetViewport(renderer, NULL);
    SDL_RenderSetClipRect(renderer, NULL);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    margin = height / 48;
    if (margin < 10)
    {
        margin = 10;
    }
    label_scale = height >= 1000 ? 4 : (height >= 700 ? 3 : 2);
    sorr_ios_d4a_ensure_control_config();

    if (sorr_ios_d4a_controls.overlay_visible || sorr_ios_d4a_edit_mode)
    {
        int cx = (int)(sorr_ios_d4a_controls.dpad_center_x * (float)width);
        int cy = (int)(sorr_ios_d4a_controls.dpad_center_y * (float)height);
        int rx = (int)(sorr_ios_d4a_controls.dpad_radius_x * (float)width);
        int ry = (int)(sorr_ios_d4a_controls.dpad_radius_y * (float)height);
        int knob_rx;
        int knob_ry;
        int knob_x;
        int knob_y;
        int dead_rx;
        int dead_ry;
        int selected = sorr_ios_d4a_edit_mode && sorr_ios_d4a_selected_target == SORR_IOS_D4A_EDIT_TARGET_DPAD;

        if (rx < 58) rx = 58;
        if (ry < 58) ry = 58;
        if (cx - rx < margin) cx = margin + rx;
        if (cx + rx > width - margin) cx = width - margin - rx;
        if (cy - ry < margin) cy = margin + ry;
        if (cy + ry > height - margin) cy = height - margin - ry;

        knob_rx = rx / 3;
        knob_ry = ry / 3;
        if (knob_rx < 24) knob_rx = 24;
        if (knob_ry < 24) knob_ry = 24;
        dead_rx = (int)((float)rx * SORR_IOS_D4A_JOYSTICK_DEADZONE);
        dead_ry = (int)((float)ry * SORR_IOS_D4A_JOYSTICK_DEADZONE);
        if (dead_rx < 14) dead_rx = 14;
        if (dead_ry < 14) dead_ry = 14;

        knob_x = cx + (int)(sorr_ios_d4a_joystick_norm_x * (float)(rx - knob_rx));
        knob_y = cy + (int)(sorr_ios_d4a_joystick_norm_y * (float)(ry - knob_ry));

        sorr_ios_d4a_draw_ellipse(renderer, cx, cy, rx, ry, 0, 0, 0, sorr_ios_d4a_alpha(selected ? 145 : 95), 1);
        sorr_ios_d4a_draw_ellipse_outline(renderer, cx, cy, rx, ry, selected ? 4 : 3, selected ? 84 : 255, selected ? 190 : 255, 255, sorr_ios_d4a_alpha(230));
        sorr_ios_d4a_draw_ellipse_outline(renderer, cx, cy, dead_rx, dead_ry, 2, 255, 255, 255, sorr_ios_d4a_alpha(95));
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, sorr_ios_d4a_alpha(115));
        SDL_RenderDrawLine(renderer, cx - rx, cy, cx + rx, cy);
        SDL_RenderDrawLine(renderer, cx, cy - ry, cx, cy + ry);
        sorr_ios_d4a_draw_ellipse(renderer,
                                  knob_x,
                                  knob_y,
                                  knob_rx,
                                  knob_ry,
                                  (sorr_ios_d4a_dpad_mask != 0) ? 84 : 32,
                                  (sorr_ios_d4a_dpad_mask != 0) ? 190 : 32,
                                  (sorr_ios_d4a_dpad_mask != 0) ? 255 : 255,
                                  sorr_ios_d4a_alpha((sorr_ios_d4a_dpad_mask != 0) ? 205 : 160),
                                  1);
        sorr_ios_d4a_draw_ellipse_outline(renderer, knob_x, knob_y, knob_rx, knob_ry, 3, 255, 255, 255, sorr_ios_d4a_alpha(240));
        if (sorr_ios_d4a_controls.labels_visible)
        {
            sorr_ios_draw_text(renderer, cx - 18 * label_scale / 2, cy - 7 * label_scale / 2, label_scale, "JOY");
        }
    }

    if (sorr_ios_d4a_controls.overlay_visible || sorr_ios_d4a_edit_mode)
    {
        for (i = 0; i < SORR_IOS_D4A_TOUCH_BUTTON_COUNT; i++)
        {
            const sorr_ios_d4a_touch_button *button = &sorr_ios_d4a_touch_buttons[i];
            int pressed = sorr_ios_d4a_button_press_count[i] > 0;
            int selected = sorr_ios_d4a_edit_mode && sorr_ios_d4a_selected_target == i;
            int is_utility = sorr_ios_d4a_button_is_utility(i);
            SDL_Rect rect;

            rect.x = (int)(sorr_ios_d4a_controls.buttons[i].x * (float)width);
            rect.y = (int)(sorr_ios_d4a_controls.buttons[i].y * (float)height);
            rect.w = (int)(sorr_ios_d4a_controls.buttons[i].w * (float)width);
            rect.h = (int)(sorr_ios_d4a_controls.buttons[i].h * (float)height);

            if (rect.w < (is_utility ? 52 : 58)) rect.w = is_utility ? 52 : 58;
            if (rect.h < (is_utility ? 36 : 58)) rect.h = is_utility ? 36 : 58;
            if (rect.x < margin) rect.x = margin;
            if (rect.y < margin) rect.y = margin;
            if (rect.x + rect.w > width - margin) rect.x = width - margin - rect.w;
            if (rect.y + rect.h > height - margin) rect.y = height - margin - rect.h;
            if (rect.x < 0) rect.x = 0;
            if (rect.y < 0) rect.y = 0;

            if (is_utility)
            {
                sorr_ios_d4a_draw_labeled_rect(renderer, &rect, button->label, label_scale, selected, pressed, 1);
            }
            else
            {
                sorr_ios_d4a_draw_labeled_ellipse(renderer, &rect, button->label, label_scale, selected, pressed, 1);
            }
        }
    }

    if (sorr_ios_d4a_edit_mode)
    {
        sorr_ios_d4a_draw_config_button(renderer, width, height, SORR_IOS_D4A_CONFIG_DONE, "DONE", label_scale);
        sorr_ios_d4a_draw_config_button(renderer, width, height, SORR_IOS_D4A_CONFIG_RESET, "RST", label_scale);
        sorr_ios_d4a_draw_config_button(renderer, width, height, SORR_IOS_D4A_CONFIG_OPACITY, "OPAC", label_scale);
        sorr_ios_d4a_draw_config_button(renderer, width, height, SORR_IOS_D4A_CONFIG_LABELS, sorr_ios_d4a_controls.labels_visible ? "TXT-" : "TXT+", label_scale);
        sorr_ios_d4a_draw_config_button(renderer, width, height, SORR_IOS_D4A_CONFIG_BIGGER, "BIG", label_scale);
        sorr_ios_d4a_draw_config_button(renderer, width, height, SORR_IOS_D4A_CONFIG_SMALLER, "SML", label_scale);
    }
    else
    {
        sorr_ios_d4a_draw_config_button(renderer, width, height, SORR_IOS_D4A_CONFIG_TOGGLE, "CFG", label_scale);
    }

    if (old_logical_w > 0 && old_logical_h > 0)
    {
        SDL_RenderSetLogicalSize(renderer, old_logical_w, old_logical_h);
    }
    else
    {
        SDL_RenderSetLogicalSize(renderer, 0, 0);
    }
    SDL_RenderSetViewport(renderer, &old_viewport);
    SDL_RenderSetClipRect(renderer, old_clip_enabled ? &old_clip : NULL);
    SDL_RenderSetScale(renderer, old_scale_x, old_scale_y);
    SDL_SetRenderDrawBlendMode(renderer, old_blend);
    SDL_SetRenderDrawColor(renderer, old_r, old_g, old_b, old_a);
}
#endif

#ifndef SORR_IOS_D3_FIRST_RENDER
static void sorr_ios_d4a_prepare_run_logs(const sorr_ios_data_layout *layout)
{
    (void)layout;
}
#endif

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
static volatile unsigned long long sorr_ios_d3_last_rss_bytes = 0;

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
extern volatile unsigned int sorr_ios_d3_lookup_guard_count;
extern volatile unsigned int sorr_ios_d3_last_lookup_id;
extern volatile unsigned int sorr_ios_d3_last_lookup_result_id;
extern char sorr_ios_d3_last_lookup_event[];
extern char sorr_ios_d3_last_native_call_event[];
extern char sorr_ios_d3_last_native_return_event[];
extern char sorr_ios_d3_native_call_events[];
extern char sorr_ios_d3_last_effect_water_event[];
extern char sorr_ios_d3_runtime_snapshot[];
extern char sorr_ios_d3_lifecycle_events[];
extern char sorr_ios_d3_destroyed_ring_snapshot[];
extern char sorr_ios_d3_family_events[];
extern char sorr_ios_d3_render_events[];
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

static void sorr_ios_maybe_write_previous_run_fallback_report(const sorr_ios_data_layout *layout);

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

static char sorr_ios_d4a_run_id[96] = "run-unset";
static unsigned int sorr_ios_d4a_run_number = 0;

#ifndef _WIN32
static char sorr_ios_d3_signal_log_path[1024];
static char sorr_ios_d4a_signal_current_run_path[1024];
static char sorr_ios_d4a_signal_private_current_run_path[1024];
static char sorr_ios_d4a_signal_latest_crash_path[1024];
static char sorr_ios_d4a_signal_private_latest_crash_path[1024];
#define SORR_IOS_D4A_RECENT_LOG_COUNT 80
#define SORR_IOS_D4A_RECENT_LOG_LINE 512
static char sorr_ios_d4a_recent_log_lines[SORR_IOS_D4A_RECENT_LOG_COUNT][SORR_IOS_D4A_RECENT_LOG_LINE];
static unsigned int sorr_ios_d4a_recent_log_pos = 0;
static unsigned int sorr_ios_d4a_recent_log_count = 0;

static void sorr_ios_d4a_recent_log_reset(void)
{
    memset(sorr_ios_d4a_recent_log_lines, 0, sizeof(sorr_ios_d4a_recent_log_lines));
    sorr_ios_d4a_recent_log_pos = 0;
    sorr_ios_d4a_recent_log_count = 0;
}

static void sorr_ios_d4a_recent_log_add(const char *line)
{
    if (!line)
    {
        return;
    }

    snprintf(sorr_ios_d4a_recent_log_lines[sorr_ios_d4a_recent_log_pos],
             sizeof(sorr_ios_d4a_recent_log_lines[sorr_ios_d4a_recent_log_pos]),
             "%s",
             line);
    sorr_ios_d4a_recent_log_pos = (sorr_ios_d4a_recent_log_pos + 1) % SORR_IOS_D4A_RECENT_LOG_COUNT;
    if (sorr_ios_d4a_recent_log_count < SORR_IOS_D4A_RECENT_LOG_COUNT)
    {
        sorr_ios_d4a_recent_log_count++;
    }
}

static void sorr_ios_signal_write_all(int fd, const char *text)
{
    size_t len;
    const char *ptr;

    if (fd < 0 || !text)
    {
        return;
    }

    ptr = text;
    len = strlen(text);
    while (len > 0)
    {
        ssize_t wrote = write(fd, ptr, len);
        if (wrote <= 0)
        {
            return;
        }
        ptr += wrote;
        len -= (size_t)wrote;
    }
}

static void sorr_ios_signal_write_format(int fd, const char *format, ...)
{
    char line[4096];
    va_list args;
    int len;

    if (fd < 0 || !format)
    {
        return;
    }

    va_start(args, format);
    len = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (len <= 0)
    {
        return;
    }

    if ((size_t)len >= sizeof(line))
    {
        len = (int)sizeof(line) - 1;
    }
    (void)write(fd, line, (size_t)len);
}

static void sorr_ios_d4a_write_crash_report_fd(int fd, int sig)
{
    Uint32 ticks = SDL_GetTicks();
    Uint32 loop_start = sorr_ios_d3_runtime_loop_start_ticks;
    Uint32 runtime_ms = loop_start ? ticks - loop_start : 0;
    unsigned int i;

    if (fd < 0)
    {
        return;
    }

    sorr_ios_signal_write_format(fd,
                                 "SORR IOS LATEST CRASH REPORT\n"
                                 "build=%s\n"
                                 "artifact=%s\n"
                                 "crash_report_version=3\n"
                                 "debug_focus=playtest GET_REAL_POINT/effects crash; keep this whole file when reporting\n"
                                 "run_id=%s\n"
                                 "run_number=%u\n"
                                 "signal=%d\n"
                                 "ticks=%u\n"
                                 "runtime_ms=%u\n"
                                 "stage=%s\n"
                                 "current_process=%s#%u:s%d:f%d:o%d\n"
                                 "runtime_last_proc=%s#%u:s%d:f%d:o%d\n"
                                 "current_proc_ptr=0x%llx\n"
                                 "last_proc_ptr=0x%llx\n"
                                 "last_lookup_id=%u\n"
                                 "last_lookup_result=%u\n"
                                 "lookup_guards=%u\n",
                                 SORR_IOS_BUILD_LABEL,
                                 SORR_IOS_ARTIFACT_LABEL,
                                 sorr_ios_d4a_run_id,
                                 sorr_ios_d4a_run_number,
                                 sig,
                                 ticks,
                                 runtime_ms,
                                 sorr_ios_d3_stage_name(sorr_ios_d3_stage),
                                 sorr_ios_d3_last_proc_name,
                                 sorr_ios_d3_last_proc_id,
                                 sorr_ios_d3_last_proc_status,
                                 sorr_ios_d3_last_proc_frame_percent,
                                 sorr_ios_d3_last_proc_code_offset,
                                 sorr_ios_d3_last_proc_name,
                                 sorr_ios_d3_last_proc_id,
                                 sorr_ios_d3_last_proc_status,
                                 sorr_ios_d3_last_proc_frame_percent,
                                 sorr_ios_d3_last_proc_code_offset,
                                 sorr_ios_d3_current_proc_ptr,
                                 sorr_ios_d3_last_proc_ptr,
                                 sorr_ios_d3_last_lookup_id,
                                 sorr_ios_d3_last_lookup_result_id,
                                 sorr_ios_d3_lookup_guard_count);
    sorr_ios_signal_write_format(fd, "last_lookup=%s\n", sorr_ios_d3_last_lookup_event);
    sorr_ios_signal_write_format(fd, "last_native_call=%s\n", sorr_ios_d3_last_native_call_event);
    sorr_ios_signal_write_format(fd, "last_native_return=%s\n", sorr_ios_d3_last_native_return_event);
    sorr_ios_signal_write_format(fd, "recent_native_call_ring=%s\n", sorr_ios_d3_native_call_events);
    sorr_ios_signal_write_format(fd, "last_effect_water=%s\n", sorr_ios_d3_last_effect_water_event);
    sorr_ios_signal_write_format(fd,
                                 "runtime_counters=heartbeats:%u loops:%u frames:%u runs:%u instances:%d render_objects:%d render_create:%u render_destroy:%u render_invalid:%u opened_files:%d x_files:%d max_x_files:%d rss_bytes:%llu\n",
                                 sorr_ios_d3_heartbeat_count,
                                 sorr_ios_d3_instance_go_loop_count,
                                 sorr_ios_d3_frame_complete_count,
                                 sorr_ios_d3_instance_run_count,
                                 sorr_ios_d3_live_instance_count,
                                 sorr_ios_d3_render_object_count,
                                 sorr_ios_d3_render_instance_object_created_count,
                                 sorr_ios_d3_render_instance_object_destroyed_count,
                                 sorr_ios_d3_render_invalid_callback_count,
                                 opened_files,
                                 x_files_count,
                                 max_x_files,
                                 sorr_ios_d3_last_rss_bytes);
    sorr_ios_signal_write_format(fd,
                                 "audio_counters=stub_zero:%u stub_minus_one:%u init:%u/%u/%u wav_load:%u/%u wav_play:%u music_load:%u open:%u/%u mem:%u/%u play:%u/%u/%u live_handles:%u live_wav:%u live_music:%u playing:%u last_status:%s last_path:%s\n",
                                 sorr_ios_sound_stub_zero_count,
                                 sorr_ios_sound_stub_minus_one_count,
                                 sorr_ios_audio_init_attempt_count,
                                 sorr_ios_audio_init_ok_count,
                                 sorr_ios_audio_init_fail_count,
                                 sorr_ios_audio_wav_load_ok_count,
                                 sorr_ios_audio_wav_load_fail_count,
                                 sorr_ios_audio_wav_play_count,
                                 sorr_ios_audio_music_load_attempt_count,
                                 sorr_ios_audio_music_open_ok_count,
                                 sorr_ios_audio_music_open_fail_count,
                                 sorr_ios_audio_music_mem_load_ok_count,
                                 sorr_ios_audio_music_mem_load_fail_count,
                                 sorr_ios_audio_music_play_attempt_count,
                                 sorr_ios_audio_music_play_ok_count,
                                 sorr_ios_audio_music_play_fail_count,
                                 sorr_ios_audio_live_handle_count,
                                 sorr_ios_audio_live_wav_count,
                                 sorr_ios_audio_live_music_count,
                                 sorr_ios_audio_music_last_playing,
                                 sorr_ios_audio_last_music_status,
                                 sorr_ios_audio_last_music_path);
    sorr_ios_signal_write_format(fd,
                                 "control_note=custom touch/joystick active; crash target is gameplay effect/water/projectile path, not controls unless touch lines appear immediately before signal\n");
    sorr_ios_signal_write_format(fd, "last_lifecycle=%s\n", sorr_ios_d3_last_lifecycle_event);
    sorr_ios_signal_write_format(fd, "last_family=%s\n", sorr_ios_d3_last_family_unlink);
    sorr_ios_signal_write_format(fd, "last_render=%s\n", sorr_ios_d3_last_render_event);
    sorr_ios_signal_write_format(fd, "recent_destroyed_process_ring=%s\n", sorr_ios_d3_destroyed_ring_snapshot);
    sorr_ios_signal_write_format(fd, "recent_runtime_lifecycle_ring=%s\n", sorr_ios_d3_lifecycle_events);
    sorr_ios_signal_write_format(fd, "recent_runtime_family_unlink_ring=%s\n", sorr_ios_d3_family_events);
    sorr_ios_signal_write_format(fd, "recent_runtime_render_event_ring=%s\n", sorr_ios_d3_render_events);
    sorr_ios_signal_write_format(fd, "latest_runtime_snapshot=%s\n", sorr_ios_d3_runtime_snapshot);
    sorr_ios_signal_write_all(fd, "current_launch_recent_log_tail:\n");

    for (i = 0; i < sorr_ios_d4a_recent_log_count; i++)
    {
        unsigned int slot = (sorr_ios_d4a_recent_log_pos + SORR_IOS_D4A_RECENT_LOG_COUNT - sorr_ios_d4a_recent_log_count + i) % SORR_IOS_D4A_RECENT_LOG_COUNT;
        if (sorr_ios_d4a_recent_log_lines[slot][0])
        {
            sorr_ios_signal_write_all(fd, sorr_ios_d4a_recent_log_lines[slot]);
            sorr_ios_signal_write_all(fd, "\n");
        }
    }
}

static void sorr_ios_d4a_write_latest_crash_report(const char *path, int sig)
{
    int fd;

    if (!path || !path[0])
    {
        return;
    }

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        return;
    }
    sorr_ios_d4a_write_crash_report_fd(fd, sig);
    fsync(fd);
    close(fd);
}

static void sorr_ios_d4a_append_signal_marker(const char *path, int sig)
{
    int fd;
    Uint32 ticks = SDL_GetTicks();
    Uint32 loop_start = sorr_ios_d3_runtime_loop_start_ticks;
    Uint32 runtime_ms = loop_start ? ticks - loop_start : 0;

    if (!path || !path[0])
    {
        return;
    }

    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
    {
        return;
    }
    sorr_ios_signal_write_format(fd,
                                 "signal=%d ticks=%u runtime_ms=%u stage=%s run_id=%s current=%s#%u ptr=0x%llx lookup_guards=%u last_lookup_id=%u last_lookup_result=%u latest_crash_report=ios_latest_crash_report.txt\n",
                                 sig,
                                 ticks,
                                 runtime_ms,
                                 sorr_ios_d3_stage_name(sorr_ios_d3_stage),
                                 sorr_ios_d4a_run_id,
                                 sorr_ios_d3_last_proc_name,
                                 sorr_ios_d3_last_proc_id,
                                 sorr_ios_d3_current_proc_ptr,
                                 sorr_ios_d3_lookup_guard_count,
                                 sorr_ios_d3_last_lookup_id,
                                 sorr_ios_d3_last_lookup_result_id);
    fsync(fd);
    close(fd);
}

static void sorr_ios_d3_signal_handler(int sig)
{
    sorr_ios_d4a_append_signal_marker(sorr_ios_d3_signal_log_path, sig);
    sorr_ios_d4a_append_signal_marker(sorr_ios_d4a_signal_current_run_path, sig);
    sorr_ios_d4a_append_signal_marker(sorr_ios_d4a_signal_private_current_run_path, sig);
    sorr_ios_d4a_write_latest_crash_report(sorr_ios_d4a_signal_latest_crash_path, sig);
    sorr_ios_d4a_write_latest_crash_report(sorr_ios_d4a_signal_private_latest_crash_path, sig);

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

static void sorr_ios_d4a_configure_crash_paths(const sorr_ios_data_layout *layout)
{
#ifndef _WIN32
    if (!layout)
    {
        return;
    }

    snprintf(sorr_ios_d3_signal_log_path,
             sizeof(sorr_ios_d3_signal_log_path),
             "%s",
             layout->d3_visible_stability_path);
    snprintf(sorr_ios_d4a_signal_current_run_path,
             sizeof(sorr_ios_d4a_signal_current_run_path),
             "%s",
             layout->d3_current_run_path);
    snprintf(sorr_ios_d4a_signal_private_current_run_path,
             sizeof(sorr_ios_d4a_signal_private_current_run_path),
             "%s",
             layout->d3_private_current_run_path);
    snprintf(sorr_ios_d4a_signal_latest_crash_path,
             sizeof(sorr_ios_d4a_signal_latest_crash_path),
             "%s",
             layout->d3_latest_crash_report_path);
    snprintf(sorr_ios_d4a_signal_private_latest_crash_path,
             sizeof(sorr_ios_d4a_signal_private_latest_crash_path),
             "%s",
             layout->d3_private_latest_crash_report_path);
#else
    (void)layout;
#endif
}

static unsigned int sorr_ios_d4a_next_run_number(const sorr_ios_data_layout *layout)
{
    char counter_path[1024];
    FILE *fp;
    unsigned int value = 0;

    if (!layout || !sorr_ios_join_path(counter_path,
                                       sizeof(counter_path),
                                       layout->documents_diagnostics_dir,
                                       "ios_run_counter.txt"))
    {
        return 0;
    }

    fp = fopen(counter_path, "rb");
    if (fp)
    {
        if (fscanf(fp, "%u", &value) != 1)
        {
            value = 0;
        }
        fclose(fp);
    }

    value++;
    fp = fopen(counter_path, "wb");
    if (fp)
    {
        fprintf(fp, "%u\n", value);
        fclose(fp);
    }

    return value;
}

static void sorr_ios_d4a_prepare_run_logs(const sorr_ios_data_layout *layout)
{
    char delimiter[512];
    time_t now;
    unsigned int ticks;

    if (!layout)
    {
        return;
    }

    ticks = SDL_GetTicks();
    now = time(NULL);
    sorr_ios_d4a_run_number = sorr_ios_d4a_next_run_number(layout);
    snprintf(sorr_ios_d4a_run_id,
             sizeof(sorr_ios_d4a_run_id),
             "%ld-%u-%u",
             (long)now,
             ticks,
             sorr_ios_d4a_run_number);

#ifndef _WIN32
    sorr_ios_d4a_recent_log_reset();
#endif
    sorr_ios_d4a_configure_crash_paths(layout);

    remove(layout->d3_previous_run_path);
    rename(layout->d3_current_run_path, layout->d3_previous_run_path);
    remove(layout->d3_private_previous_run_path);
    rename(layout->d3_private_current_run_path, layout->d3_private_previous_run_path);

    snprintf(delimiter,
             sizeof(delimiter),
             "===== SORR IOS RUN %s build=%s artifact=%s run_number=%u ticks=%u =====",
             sorr_ios_d4a_run_id,
             SORR_IOS_BUILD_LABEL,
             SORR_IOS_ARTIFACT_LABEL,
             sorr_ios_d4a_run_number,
             ticks);
    sorr_ios_d3_append_log_file(layout->d3_current_run_path, delimiter);
    sorr_ios_d3_append_log_file(layout->d3_private_current_run_path, delimiter);
    sorr_ios_d3_append_log_file(layout->d3_visible_stability_path, delimiter);
    sorr_ios_d3_append_log_file(layout->d3_stability_path, delimiter);
    remove(layout->audio_sfx_diagnostics_path);
    sorr_ios_d3_append_log_file(layout->audio_sfx_diagnostics_path, delimiter);
    sorr_ios_maybe_write_previous_run_fallback_report(layout);
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
    sorr_ios_d3_append_log_file(layout->d3_current_run_path, line);
    sorr_ios_d3_append_log_file(layout->d3_private_current_run_path, line);
#ifndef _WIN32
    sorr_ios_d4a_recent_log_add(line);
#endif
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
        sorr_ios_d3_append_log_file(layout->d3_current_run_path, line);
        sorr_ios_d3_append_log_file(layout->d3_private_current_run_path, line);
    }
#ifndef _WIN32
    sorr_ios_d4a_recent_log_add(line);
#endif
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

static int sorr_ios_file_contains_line_prefix(const char *path, const char *prefix)
{
    FILE *fp;
    char line[4096];
    size_t prefix_len;

    if (!path || !path[0] || !prefix || !prefix[0])
    {
        return 0;
    }

    prefix_len = strlen(prefix);
    fp = fopen(path, "rb");
    if (!fp)
    {
        return 0;
    }

    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, prefix, prefix_len) == 0)
        {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

static void sorr_ios_write_file_tail(FILE *out, const char *path, int max_lines)
{
    FILE *fp;
    char (*ring)[1024];
    char line[1024];
    int count = 0;
    int pos = 0;
    int i;

    if (!out || !path || !path[0] || max_lines <= 0)
    {
        return;
    }

    fp = fopen(path, "rb");
    if (!fp)
    {
        fprintf(out, "tail_unavailable path=%s\n", path);
        return;
    }

    ring = (char (*)[1024])calloc((size_t)max_lines, sizeof(*ring));
    if (!ring)
    {
        fclose(fp);
        fprintf(out, "tail_unavailable reason=alloc-failed path=%s\n", path);
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        snprintf(ring[pos], 1024, "%s", line);
        pos = (pos + 1) % max_lines;
        if (count < max_lines)
        {
            count++;
        }
    }

    for (i = 0; i < count; i++)
    {
        int slot = (pos + max_lines - count + i) % max_lines;
        fputs(ring[slot], out);
        if (ring[slot][0])
        {
            size_t len = strlen(ring[slot]);
            if (len > 0 && ring[slot][len - 1] != '\n')
            {
                fputc('\n', out);
            }
        }
    }

    free(ring);
    fclose(fp);
}

static void sorr_ios_write_previous_run_fallback_report(const sorr_ios_data_layout *layout,
                                                        const char *path,
                                                        const char *previous_last_marker,
                                                        const char *reason)
{
    FILE *fp;

    if (!layout || !path || !path[0])
    {
        return;
    }

    fp = fopen(path, "wb");
    if (!fp)
    {
        return;
    }

    fprintf(fp, "SORR IOS LATEST CRASH REPORT\n");
    fprintf(fp, "build=%s\n", SORR_IOS_BUILD_LABEL);
    fprintf(fp, "artifact=%s\n", SORR_IOS_ARTIFACT_LABEL);
    fprintf(fp, "crash_report_version=4\n");
    fprintf(fp, "crash_report_type=previous-run-nosignal-fallback\n");
    fprintf(fp, "reason=%s\n", reason ? reason : "previous run ended without clean shutdown marker");
    fprintf(fp, "run_id=%s\n", sorr_ios_d4a_run_id);
    fprintf(fp, "run_number=%u\n", sorr_ios_d4a_run_number);
    fprintf(fp, "signal=none\n");
    fprintf(fp, "stage=next-launch-fallback\n");
    fprintf(fp, "previous_run_log=%s\n", layout->d3_previous_run_path);
    fprintf(fp, "previous_private_run_log=%s\n", layout->d3_private_previous_run_path);
    fprintf(fp, "previous_last_marker=%s\n", previous_last_marker && previous_last_marker[0] ? previous_last_marker : "(none)");
    fprintf(fp, "note=This report was synthesized on the next launch because the prior run did not reach the signal handler.\n");
    fprintf(fp, "note=Use the previous_run_tail below plus ios_previous_run_stability_log.txt to debug abrupt exits or iOS kills.\n");
    fprintf(fp, "previous_run_tail:\n");
    sorr_ios_write_file_tail(fp, layout->d3_previous_run_path, 180);
    fclose(fp);
}

static void sorr_ios_maybe_write_previous_run_fallback_report(const sorr_ios_data_layout *layout)
{
    char previous_last_marker[1024] = "";
    int has_previous;
    int has_clean_shutdown;
    int has_signal;

    if (!layout)
    {
        return;
    }

    has_previous = sorr_ios_read_last_nonempty_line(layout->d3_previous_run_path,
                                                    previous_last_marker,
                                                    sizeof(previous_last_marker));
    if (!has_previous)
    {
        return;
    }

    has_clean_shutdown = sorr_ios_file_contains_line_prefix(layout->d3_previous_run_path, "clean_shutdown=1");
    has_signal = sorr_ios_file_contains_line_prefix(layout->d3_previous_run_path, "signal=");

    if (has_clean_shutdown || has_signal)
    {
        return;
    }

    sorr_ios_write_previous_run_fallback_report(layout,
                                                layout->d3_latest_crash_report_path,
                                                previous_last_marker,
                                                "previous current-run log rotated without clean_shutdown=1 or signal=");
    sorr_ios_write_previous_run_fallback_report(layout,
                                                layout->d3_private_latest_crash_report_path,
                                                previous_last_marker,
                                                "previous current-run log rotated without clean_shutdown=1 or signal=");
    sorr_ios_d3_append_log_file(layout->d3_current_run_path,
                                "previous_run_fallback_crash_report=ios_latest_crash_report.txt reason=no-clean-shutdown-no-signal");
    sorr_ios_d3_append_log_file(layout->d3_private_current_run_path,
                                "previous_run_fallback_crash_report=ios_latest_crash_report.txt reason=no-clean-shutdown-no-signal");
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
    sorr_ios_d3_last_rss_bytes = rss;

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
                              "heartbeat=%u ticks=%u runtime_ms=%u interval_next_ms=%u stage=%s rss_bytes=%llu frame_count=%u last_frame_ticks=%d frame_ms=%.3f fps_count=%d fps_init=%d max_jump=%d jump=%d instances=%d render_objects=%d render_object_creates=%u render_object_destroys=%u render_invalid_callbacks=%u opened_files=%d x_files=%d max_x_files=%d runtime_loops=%u runtime_frames=%u runtime_runs=%u runtime_created=%u runtime_destroyed=%u runtime_snapshots=%u runtime_last_proc=%s#%u:s%d:f%d:o%d runtime_lookup_guards=%u runtime_last_lookup_id=%u runtime_last_lookup_result=%u runtime_last_lookup=%s audio_stub_zero=%u audio_stub_minus_one=%u audio_init_attempts=%u audio_init_ok=%u audio_init_fail=%u audio_wav_load_ok=%u audio_wav_load_fail=%u audio_wav_play=%u audio_inert_handles=%u audio_queue_clears=%u audio_music_load_attempts=%u audio_music_open_ok=%u audio_music_open_fail=%u audio_music_mem_ok=%u audio_music_mem_fail=%u audio_music_play_attempts=%u audio_music_play_ok=%u audio_music_play_fail=%u audio_music_controls=%u audio_music_queries=%u audio_music_free=%u audio_music_halt=%u audio_music_playing=%u audio_music_last_handle=%llu audio_music_last_ptr=0x%llx audio_music_last_bytes=%llu audio_music_total_bytes=%llu audio_live_handles=%u audio_live_wav=%u audio_live_inert_wav=%u audio_live_music=%u audio_max_live_handles=%u audio_zero_music_play=%u audio_zero_music_control=%u audio_zero_music_query=%u audio_zero_wav_control=%u audio_zero_wav_query=%u audio_zero_wav_volume=%u audio_zero_channel_effect=%u audio_zero_play_wav_guard=%u audio_music_last_status=%s audio_music_last_path=%s runtime_snapshot=%s runtime_lifecycle=%s",
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
                              sorr_ios_d3_lookup_guard_count,
                              sorr_ios_d3_last_lookup_id,
                              sorr_ios_d3_last_lookup_result_id,
                              sorr_ios_d3_last_lookup_event,
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

    sorr_ios_d4a_process_sdl_event(event);

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

    sorr_ios_d4a_set_active_layout(layout);
    sorr_ios_copy_file_contents(layout->d3_stability_path, layout->d3_visible_stability_path);
    if (!sorr_ios_read_last_nonempty_line(layout->d3_previous_run_path,
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
    sorr_ios_d3_log(layout, "current run log path=%s", layout->d3_current_run_path);
    sorr_ios_d3_log(layout, "previous run log path=%s", layout->d3_previous_run_path);
    sorr_ios_d3_log(layout, "latest crash report path=%s", layout->d3_latest_crash_report_path);
    sorr_ios_d3_log(layout, "audio SFX diagnostics path=%s", layout->audio_sfx_diagnostics_path);
    sorr_ios_d3_log(layout, "touch config path=%s", layout->d4_touch_config_path);
    snprintf(sorr_ios_d3_visible_event_log_path,
             1024,
             "%s",
             layout->d3_current_run_path);
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
    sorr_ios_status_add("D4B CUSTOM TOUCH ENABLED");
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
    SDL_setenv("SORR_IOS_AUDIO_DIAG_PATH", layout->audio_sfx_diagnostics_path, 1);
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
        !sorr_ios_join_path(layout->d3_visible_stability_path, sizeof(layout->d3_visible_stability_path), layout->documents_diagnostics_dir, "ios_d3_runtime_stability_probe.txt") ||
        !sorr_ios_join_path(layout->d3_current_run_path, sizeof(layout->d3_current_run_path), layout->documents_diagnostics_dir, "ios_current_run_stability_log.txt") ||
        !sorr_ios_join_path(layout->d3_previous_run_path, sizeof(layout->d3_previous_run_path), layout->documents_diagnostics_dir, "ios_previous_run_stability_log.txt") ||
        !sorr_ios_join_path(layout->d3_latest_crash_report_path, sizeof(layout->d3_latest_crash_report_path), layout->documents_diagnostics_dir, "ios_latest_crash_report.txt") ||
        !sorr_ios_join_path(layout->d3_private_current_run_path, sizeof(layout->d3_private_current_run_path), layout->logs_dir, "ios_current_run_stability_log.txt") ||
        !sorr_ios_join_path(layout->d3_private_previous_run_path, sizeof(layout->d3_private_previous_run_path), layout->logs_dir, "ios_previous_run_stability_log.txt") ||
        !sorr_ios_join_path(layout->d3_private_latest_crash_report_path, sizeof(layout->d3_private_latest_crash_report_path), layout->logs_dir, "ios_latest_crash_report.txt") ||
        !sorr_ios_join_path(layout->d4_touch_config_path, sizeof(layout->d4_touch_config_path), layout->documents_diagnostics_dir, "ios_touch_controls.ini") ||
        !sorr_ios_join_path(layout->audio_sfx_diagnostics_path, sizeof(layout->audio_sfx_diagnostics_path), layout->documents_diagnostics_dir, "ios_audio_sfx_diagnostics.txt"))
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
                                 "For crashes, reopen Streets of Rage once and send ios_latest_crash_report.txt.\n"
                                 "If more context is needed, also send ios_current_run_stability_log.txt.\n"
                                 "For wrong SFX/run sound bugs, send ios_audio_sfx_diagnostics.txt after reproducing.\n"
                                 "ios_previous_run_stability_log.txt contains the prior launch, and ios_d3_runtime_stability_probe.txt remains the full rolling log.\n"
                                 "D4b-lite touch settings are saved in ios_touch_controls.ini.\n");
    }

    if (!sorr_ios_create_dir_marker("savegame", layout->savegame_dir) ||
        !sorr_ios_create_dir_marker("xbox", layout->xbox_dir) ||
        !sorr_ios_create_dir_marker("logs", layout->logs_dir))
    {
        return 0;
    }

    sorr_ios_d4a_prepare_run_logs(layout);

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
    sorr_ios_d4a_set_active_layout(&data_layout);
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
            sorr_ios_d4a_process_sdl_event(&event);
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
            sorr_ios_d4a_draw_touch_overlay(renderer);
            SDL_RenderPresent(renderer);
        }
        SDL_Delay(16);
    }

#ifdef SORR_IOS_D3_FIRST_RENDER
    sorr_ios_d3_log(&data_layout,
                    "clean_shutdown=1 ticks=%u stage=%s",
                    SDL_GetTicks(),
                    sorr_ios_d3_stage_name(sorr_ios_d3_stage));
#else
    sorr_ios_d3_log(&data_layout,
                    "clean_shutdown=1 ticks=%u stage=shell-loop",
                    SDL_GetTicks());
#endif
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
