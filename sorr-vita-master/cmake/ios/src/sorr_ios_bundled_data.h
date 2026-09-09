/* Keep large assets in the signed app bundle. Only saves/config are copied.
 * Private symlinks allow native directory enumeration and Bennu file_open to
 * use the same working directory without duplicating the game's assets.
 */
#ifndef _WIN32
static int sorr_ios_copy_missing_tree(const char *src, const char *dst)
{
    DIR *dir;
    struct dirent *entry;
    if (!sorr_ios_mkdir_if_needed(dst)) return 0;
    dir = opendir(src);
    if (!dir) return 0;
    while ((entry = readdir(dir)))
    {
        char from[1024], to[1024];
        struct stat st;
        if (entry->d_name[0] == '.') continue;
        if (!sorr_ios_join_path(from, sizeof(from), src, entry->d_name) ||
            !sorr_ios_join_path(to, sizeof(to), dst, entry->d_name) || stat(from, &st))
        { closedir(dir); return 0; }
        if (S_ISDIR(st.st_mode))
        {
            if (!sorr_ios_copy_missing_tree(from, to)) { closedir(dir); return 0; }
        }
        else if (access(to, F_OK) != 0 && !sorr_ios_copy_file_if_needed(from, to))
        { closedir(dir); return 0; }
    }
    closedir(dir);
    return 1;
}

static int sorr_ios_link_bundle_tree(const char *src, const char *dst)
{
    DIR *dir = opendir(src);
    struct dirent *entry;
    if (!dir) return 0;
    while ((entry = readdir(dir)))
    {
        char from[1024], to[1024];
        struct stat st;
        if (entry->d_name[0] == '.' || !strcmp(entry->d_name, "savegame") ||
            !strcmp(entry->d_name, "xbox") || !strcmp(entry->d_name, "logs")) continue;
        if (!sorr_ios_join_path(from, sizeof(from), src, entry->d_name) ||
            !sorr_ios_join_path(to, sizeof(to), dst, entry->d_name))
        { closedir(dir); return 0; }
        if (lstat(to, &st) == 0)
        {
            /* App update/re-sign can move its bundle. Refresh links, never files. */
            if (!S_ISLNK(st.st_mode) || unlink(to)) { closedir(dir); return 0; }
        }
        else if (errno != ENOENT) { closedir(dir); return 0; }
        if (symlink(from, to)) { closedir(dir); return 0; }
    }
    closedir(dir);
    return 1;
}

static int sorr_ios_prepare_bundled_data(const sorr_ios_data_layout *layout, const char *app_support)
{
    char source[1024], legacy[1024], legacy_save[1024], legacy_file[1024];
    char bundle_file[1024], xbox_file[1024], marker[1024];
    FILE *fp;
    if (!layout->bundled_data) return 1;
    if (!sorr_ios_link_bundle_tree(layout->game_bundle_root, layout->support_root)) return 0;
    if (!sorr_ios_join_path(marker, sizeof(marker), layout->support_root, ".saves-initialized")) return 0;
    if (access(marker, F_OK) == 0) return 1;
    if (!sorr_ios_join_path(legacy, sizeof(legacy), app_support, "SORR") ||
        !sorr_ios_join_path(legacy_save, sizeof(legacy_save), legacy, "savegame") ||
        !sorr_ios_join_path(legacy_file, sizeof(legacy_file), legacy_save, "savegame.sor") ||
        !sorr_ios_join_path(source, sizeof(source), layout->game_bundle_root, "savegame")) return 0;
    /* Existing players keep progress and options; upgrades never overwrite saves. */
    if (access(legacy_file, F_OK) == 0 && !sorr_ios_copy_missing_tree(legacy_save, layout->savegame_dir)) return 0;
    if (!sorr_ios_copy_missing_tree(source, layout->savegame_dir)) return 0;
    if (!sorr_ios_join_path(bundle_file, sizeof(bundle_file), layout->game_bundle_root, "xbox/xbox.cfg") ||
        !sorr_ios_join_path(xbox_file, sizeof(xbox_file), layout->xbox_dir, "xbox.cfg")) return 0;
    if (access(xbox_file, F_OK) != 0 && !sorr_ios_copy_file_if_needed(bundle_file, xbox_file)) return 0;
    fp = fopen(marker, "wb");
    if (!fp) return 0;
    fputs("1\n", fp);
    if (fclose(fp)) return 0;
    return 1;
}
#else
static int sorr_ios_prepare_bundled_data(const sorr_ios_data_layout *layout, const char *app_support)
{ (void)layout; (void)app_support; return 1; }
#endif
