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
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
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
    char documents_root[1024];
    char documents_import_dir[1024];
    char support_root[1024];
    char savegame_dir[1024];
    char xbox_dir[1024];
    char logs_dir[1024];
    char sorr_dat_path[1024];
    char required_file_path[1024];
    char d2_probe_path[1024];
    bool d2_data_ready;
    bool d2_import_seen;
    bool d2_import_failed;
} sorr_ios_data_layout;

#define SORR_IOS_STATUS_MAX_LINES 12
#define SORR_IOS_STATUS_LINE_LEN 96

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
        sorr_ios_draw_text(renderer, 28, 30 + i * 34, 4, sorr_ios_status_lines[i]);
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
        !sorr_ios_join_path(layout->documents_root, sizeof(layout->documents_root), documents_dir, "SORR") ||
        !sorr_ios_join_path(library_dir, sizeof(library_dir), home, "Library") ||
        !sorr_ios_join_path(app_support_dir, sizeof(app_support_dir), library_dir, "Application Support") ||
        !sorr_ios_join_path(layout->support_root, sizeof(layout->support_root), app_support_dir, "SORR") ||
        !sorr_ios_join_path(layout->savegame_dir, sizeof(layout->savegame_dir), layout->support_root, "savegame") ||
        !sorr_ios_join_path(layout->xbox_dir, sizeof(layout->xbox_dir), layout->support_root, "xbox") ||
        !sorr_ios_join_path(layout->logs_dir, sizeof(layout->logs_dir), layout->support_root, "logs") ||
        !sorr_ios_join_path(layout->documents_import_dir, sizeof(layout->documents_import_dir), layout->documents_root, "data") ||
        !sorr_ios_join_path(layout->sorr_dat_path, sizeof(layout->sorr_dat_path), layout->support_root, "SorR.dat") ||
        !sorr_ios_join_path(layout->required_file_path, sizeof(layout->required_file_path), layout->support_root, "mod/system.txt") ||
        !sorr_ios_join_path(layout->d2_probe_path, sizeof(layout->d2_probe_path), layout->logs_dir, "ios_d2_data_import_probe.txt"))
    {
        SDL_Log("SORR iOS shell: data layout path construction failed");
        return 0;
    }

    if (!sorr_ios_mkdir_if_needed(documents_dir) ||
        !sorr_ios_mkdir_if_needed(layout->documents_root) ||
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
    SDL_Log("SORR iOS shell: D2 file sharing import path=%s", layout->documents_root);

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
        "SoRR iOS D2 data import folder\n"
        "\n"
        "Copy the contents of the prepared SoRR data folder here.\n"
        "Expected examples:\n"
        "- SorR.dat\n"
        "- mod/system.txt\n"
        "- savegame/\n"
        "- xbox/\n"
        "\n"
        "You may also copy the prepared folder as Documents/SORR/data.\n"
        "The shell copies detected data into Library/Application Support/SORR.\n"
        "No game runtime is executed during D2.\n";

    if (!layout ||
        !sorr_ios_join_path(readme_path, sizeof(readme_path), layout->documents_root, "README_D2_IMPORT.txt"))
    {
        return;
    }

    if (sorr_ios_write_text_file(readme_path, readme_text))
    {
        SDL_Log("SORR iOS shell: D2 import README available path=%s", readme_path);
    }
}

static int sorr_ios_detect_import_root(const sorr_ios_data_layout *layout, char *out_root, size_t out_root_size)
{
    char candidate[1024];

    if (!layout || !out_root || out_root_size == 0)
    {
        return 0;
    }

    if (sorr_ios_join_path(candidate, sizeof(candidate), layout->documents_root, "SorR.dat") &&
        sorr_ios_file_size(candidate, NULL))
    {
        snprintf(out_root, out_root_size, "%s", layout->documents_root);
        return 1;
    }

    if (sorr_ios_join_path(candidate, sizeof(candidate), layout->documents_import_dir, "SorR.dat") &&
        sorr_ios_file_size(candidate, NULL))
    {
        snprintf(out_root, out_root_size, "%s", layout->documents_import_dir);
        return 1;
    }

    return 0;
}

static int sorr_ios_write_d2_probe(const sorr_ios_data_layout *layout)
{
    char readback[128];
    const char *proof_text = "sorr-ios-d2-data-import-ok\n";
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

static void sorr_ios_run_d2_probe(sorr_ios_data_layout *layout)
{
    char import_root[1024];
    long sorr_dat_size = 0;
    long required_size = 0;
    int import_available;
    int sorr_dat_ok;
    int required_ok;
    int probe_ok;

    if (!layout)
    {
        return;
    }

    sorr_ios_status_clear();
    sorr_ios_status_add("D2 SORR DATA PROBE");
    sorr_ios_status_add("FILE SHARING ROUTE");
    sorr_ios_write_import_readme(layout);

    import_available = sorr_ios_detect_import_root(layout, import_root, sizeof(import_root));
    if (import_available)
    {
        layout->d2_import_seen = true;
        SDL_Log("SORR iOS shell: D2 import started source=%s destination=%s",
                import_root,
                layout->support_root);
        sorr_ios_status_add("IMPORT STARTED");
        if (sorr_ios_copy_tree(import_root, layout->support_root))
        {
            SDL_Log("SORR iOS shell: D2 import completed source=%s", import_root);
            sorr_ios_status_add("IMPORT COMPLETED");
            layout->d2_import_failed = false;
        }
        else
        {
            SDL_Log("SORR iOS shell: D2 import failed source=%s", import_root);
            sorr_ios_status_add("IMPORT FAILED");
            layout->d2_import_failed = true;
        }
    }
    else
    {
        SDL_Log("SORR iOS shell: D2 waiting for data import path=%s", layout->documents_root);
        sorr_ios_status_add("WAITING FOR DATA IMPORT");
        sorr_ios_status_add("FILES APP: SORRIOSSHELL/SORR");
    }

    sorr_dat_ok = sorr_ios_open_file_probe("D2 SorR.dat", layout->sorr_dat_path, &sorr_dat_size);
    required_ok = sorr_ios_open_file_probe("D2 required data file mod/system.txt",
                                           layout->required_file_path,
                                           &required_size);
    probe_ok = sorr_ios_write_d2_probe(layout);

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
        sorr_ios_status_add("SAVEGAME XBOX LOGS WRITABLE");
    }

    sorr_ios_status_add("NO GAME EXECUTION");
    layout->d2_data_ready = (sorr_dat_ok && required_ok && probe_ok && !layout->d2_import_failed);

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

int main(int argc, char *argv[])
{
    sorr_ios_data_layout data_layout;
    Uint32 last_d2_probe_ticks = 0;

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

        if (SDL_GetTicks() - last_d2_probe_ticks > 2000)
        {
            sorr_ios_run_d2_probe(&data_layout);
            last_d2_probe_ticks = SDL_GetTicks();
        }

        sorr_ios_draw_status(renderer);
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
