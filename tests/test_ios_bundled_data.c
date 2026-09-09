#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
typedef struct {
    int bundled_data;
    char game_bundle_root[1024], support_root[1024], savegame_dir[1024], xbox_dir[1024];
} sorr_ios_data_layout;
static int sorr_ios_mkdir_if_needed(const char *p) { return mkdir(p, 0755) == 0 || errno == EEXIST; }
static int sorr_ios_join_path(char *out, size_t n, const char *root, const char *name)
{ return snprintf(out, n, "%s/%s", root, name) < (int)n; }
static void write_file(const char *path, const char *value)
{ FILE *f=fopen(path,"wb"); assert(f); fputs(value,f); fclose(f); }
static int sorr_ios_copy_file_if_needed(const char *src, const char *dst)
{ FILE *a=fopen(src,"rb"), *b; int c; if(!a) return 0; b=fopen(dst,"wb"); if(!b) {fclose(a);return 0;} while((c=fgetc(a))!=EOF) fputc(c,b); fclose(a);fclose(b);return 1; }
#include "../sorr-vita-master/cmake/ios/src/sorr_ios_bundled_data.h"
int main(void)
{
    char dir[]="/tmp/sorr-bundle-XXXXXX", path[1024], target[1024];
    char value[64]={0};
    FILE *f;
    struct stat st;
    sorr_ios_data_layout layout={0};
    assert(mkdtemp(dir));
    assert(chdir(dir)==0);
    assert(mkdir("bundle",0755)==0);
    assert(mkdir("bundle/savegame",0755)==0);
    assert(mkdir("bundle/xbox",0755)==0);
    assert(mkdir("bundle/mod",0755)==0);
    write_file("bundle/SorR.dat","game");
    write_file("bundle/mod/system.txt","config");
    write_file("bundle/savegame/savegame.sor","defaults");
    write_file("bundle/xbox/xbox.cfg","xbox");
    assert(mkdir("SORR",0755)==0);
    assert(mkdir("SORR/savegame",0755)==0);
    write_file("SORR/savegame/savegame.sor","existing-progress");
    assert(mkdir("SORR-Bundled",0755)==0);
    assert(mkdir("SORR-Bundled/savegame",0755)==0);
    assert(mkdir("SORR-Bundled/xbox",0755)==0);
    layout.bundled_data=1;
    sorr_ios_join_path(layout.game_bundle_root,1024,dir,"bundle");
    sorr_ios_join_path(layout.support_root,1024,dir,"SORR-Bundled");
    sorr_ios_join_path(layout.savegame_dir,1024,layout.support_root,"savegame");
    sorr_ios_join_path(layout.xbox_dir,1024,layout.support_root,"xbox");
    assert(sorr_ios_prepare_bundled_data(&layout,dir));
    assert(lstat("SORR-Bundled/mod",&st)==0 && S_ISLNK(st.st_mode));
    f=fopen("SORR-Bundled/savegame/savegame.sor","rb"); assert(f);
    fread(value,1,63,f); fclose(f); assert(!strcmp(value,"existing-progress"));
    write_file("SORR-Bundled/savegame/savegame.sor","new-progress");
    /* Simulate iOS moving the app container during an update. */
    assert(rename("bundle","bundle-updated")==0);
    sorr_ios_join_path(layout.game_bundle_root,1024,dir,"bundle-updated");
    assert(sorr_ios_prepare_bundled_data(&layout,dir));
    memset(value,0,sizeof(value));
    f=fopen("SORR-Bundled/savegame/savegame.sor","rb"); assert(f);
    fread(value,1,63,f); fclose(f); assert(!strcmp(value,"new-progress"));
    sorr_ios_join_path(path,1024,dir,"SORR-Bundled/mod");
    memset(target,0,sizeof(target)); assert(readlink(path,target,1023)>0);
    assert(strstr(target,"bundle-updated/mod"));
    assert(access("SORR-Bundled/mod/system.txt",F_OK)==0);
    assert(access("SORR/savegame/savegame.sor",F_OK)==0);
    puts("PASS: bundle symlinks, migration, save preservation, app-container relocation");
    return 0;
}
