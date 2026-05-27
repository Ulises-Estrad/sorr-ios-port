#include "mod_map.h"

#ifdef PORTABLE_RUNTIME_DIAG
#include "portable_diag.h"
#endif

GRAPH * gr_read_png(const char *filename)
{
    PORTABLE_DIAG_LOG("RENDER", "iOS D3 PNG read stub filename=%s", filename ? filename : "(null)");
    return NULL;
}

int gr_load_png(const char *mapname)
{
    PORTABLE_DIAG_LOG("RENDER", "iOS D3 PNG load stub filename=%s", mapname ? mapname : "(null)");
    return 0;
}

int gr_save_png(GRAPH *gr, const char *filename)
{
    (void)gr;
    PORTABLE_DIAG_LOG("RENDER", "iOS D3 PNG save stub filename=%s", filename ? filename : "(null)");
    return 0;
}
