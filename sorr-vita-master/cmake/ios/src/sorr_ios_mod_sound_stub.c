#include "bgddl.h"

#ifdef PORTABLE_RUNTIME_DIAG
#include "portable_diag.h"
#endif

static int sorr_ios_sound_zero(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3 audio stub returning 0");
    return 0;
}

static int sorr_ios_sound_minus_one(INSTANCE *my, int *params)
{
    (void)my;
    (void)params;
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3 audio stub returning -1");
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
    { "SOUND_INIT", "", TYPE_INT, sorr_ios_sound_zero },
    { "SOUND_CLOSE", "", TYPE_INT, sorr_ios_sound_zero },
    { "LOAD_SONG", "S", TYPE_INT, sorr_ios_sound_zero },
    { "LOAD_SONG", "SP", TYPE_INT, sorr_ios_sound_zero },
    { "PLAY_SONG", "II", TYPE_INT, sorr_ios_sound_zero },
    { "UNLOAD_SONG", "I", TYPE_INT, sorr_ios_sound_zero },
    { "UNLOAD_SONG", "P", TYPE_INT, sorr_ios_sound_zero },
    { "STOP_SONG", "", TYPE_INT, sorr_ios_sound_zero },
    { "PAUSE_SONG", "", TYPE_INT, sorr_ios_sound_zero },
    { "RESUME_SONG", "", TYPE_INT, sorr_ios_sound_zero },
    { "SET_SONG_VOLUME", "I", TYPE_INT, sorr_ios_sound_zero },
    { "IS_PLAYING_SONG", "", TYPE_INT, sorr_ios_sound_zero },
    { "LOAD_WAV", "S", TYPE_INT, sorr_ios_sound_zero },
    { "LOAD_WAV", "SP", TYPE_INT, sorr_ios_sound_zero },
    { "UNLOAD_WAV", "I", TYPE_INT, sorr_ios_sound_zero },
    { "UNLOAD_WAV", "P", TYPE_INT, sorr_ios_sound_zero },
    { "PLAY_WAV", "II", TYPE_INT, sorr_ios_sound_minus_one },
    { "PLAY_WAV", "III", TYPE_INT, sorr_ios_sound_minus_one },
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
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3 audio subsystem intentionally stubbed");
}

void __bgdexport(mod_sound, module_finalize)()
{
    PORTABLE_DIAG_LOG("AUDIO", "iOS D3 audio stub finalize");
}
