/*
 *  Copyright © 2006-2012 SplinterGU (Fenix/Bennugd)
 *  Copyright © 2002-2006 Fenix Team (Fenix)
 *  Copyright © 1999-2002 José Luis Cebrián Pagüe (Fenix)
 *
 *  This file is part of Bennu - Game Development
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define __LIB_RENDER
#include "librender.h"

#ifdef PORTABLE_RUNTIME_DIAG
#include "portable_diag.h"
#endif

/* --------------------------------------------------------------------------- */

#define FPS_INTIAL_VALUE    25
#define FPS_INTIAL_SKIP     2

/* --------------------------------------------------------------------------- */

int fps_value = FPS_INTIAL_VALUE ;
int max_jump = FPS_INTIAL_SKIP ;
float frame_ms = 1000.0 / FPS_INTIAL_VALUE ; /* 40.0 ; */

uint32_t frame_count = 0 ;
int last_frame_ticks = 0 ;
int jump = 0 ;

int FPS_count = 0 ;
int FPS_init = 0 ;

int FPS_count_sync = 0 ;
int FPS_init_sync = 0 ;

float ticks_per_frame = 0;
float fps_partial = 0;

/* --------------------------------------------------------------------------- */
/* Inicialización y controles de tiempo                                        */
/* --------------------------------------------------------------------------- */

/*
 *  FUNCTION : gr_set_fps
 *
 *  Change the game fps and frameskip values
 *
 *  PARAMS :
 *      fps         New number of frames per second
 *      jump        New value of maximum frameskip
 *
 *  RETURN VALUE :
 *      None
 */

void gr_set_fps( int fps, int skip )
{
#ifdef PORTABLE_RUNTIME_DIAG
    int old_fps = fps_value;
    int old_skip = max_jump;
    float old_frame_ms = frame_ms;
#endif

    if ( fps == fps_value && skip == max_jump )
    {
        PORTABLE_DIAG_LOG( "TIMING", "gr_set_fps unchanged fps=%d skip=%d frame_ms=%.3f frame_count=%u", fps_value, max_jump, frame_ms, frame_count );
        return ;
    }

    frame_ms = fps ? 1000.0 / ( float ) fps : 0.0 ;
    max_jump = skip ;
    fps_value = ( int ) fps;

    FPS_init_sync = FPS_init = 0 ;
    FPS_count_sync = FPS_count = 0 ;

    jump = 0;

    PORTABLE_DIAG_LOG( "TIMING", "gr_set_fps old_fps=%d old_skip=%d old_frame_ms=%.3f new_fps=%d new_skip=%d new_frame_ms=%.3f frame_count=%u",
                       old_fps, old_skip, old_frame_ms, fps_value, max_jump, frame_ms, frame_count );
}

/* --------------------------------------------------------------------------- */
/*
 *  FUNCTION : gr_wait_frame
 *
 *  Wait for the next frame start.
 *
 *  PARAMS :
 *      None
 *
 *  RETURN VALUE :
 *      None
 */

void gr_wait_frame()
{
    int frame_ticks ;
#ifdef PORTABLE_RUNTIME_DIAG
    static int diag_last_report_ticks = 0;
    static uint32_t diag_last_report_frame = 0;
    static int diag_delay_total = 0;
    static int diag_delay_count = 0;
    static int diag_skip_count = 0;
    int diag_delay = 0;
#endif

    frame_count++ ;

    /* -------------- */

    /* Tomo Tick actual */
#if defined(TARGET_GP2X_WIZ) || defined(TARGET_CAANOO)
    frame_ticks = bgdrtm_ptimer_get_ticks_us() / 1000L;
#else
    frame_ticks = SDL_GetTicks() ;
#endif
    if ( !FPS_init_sync )
    {
#if defined(TARGET_GP2X_WIZ) || defined(TARGET_CAANOO)
        FPS_init_sync = FPS_init = bgdrtm_ptimer_get_ticks_us() / 1000L;
#else
        FPS_init_sync = FPS_init = SDL_GetTicks() ;
#endif
        FPS_count_sync = FPS_count = 0 ;
        jump = 0;

        /* Tiempo inicial del nuevo frame */
        last_frame_ticks = frame_ticks ;

#ifdef PORTABLE_RUNTIME_DIAG
        diag_last_report_ticks = frame_ticks;
        diag_last_report_frame = frame_count;
        diag_delay_total = 0;
        diag_delay_count = 0;
        diag_skip_count = 0;
        PORTABLE_DIAG_LOG( "TIMING", "gr_wait_frame init tick=%d fps=%d skip=%d frame_ms=%.3f waitvsync=%d",
                           frame_ticks, fps_value, max_jump, frame_ms, waitvsync );
#endif

        return;
    }

    /* Tiempo transcurrido total del ejecucion del ultimo frame (Frame time en ms) */
    * ( float * ) &GLODWORD( librender, FRAME_TIME ) = ( frame_ticks - last_frame_ticks ) / 1000.0f ;

    /* -------------- */

    FPS_count++ ;

    /* -------------- */

    if ( fps_value )
    {
        FPS_count_sync++ ;

        ticks_per_frame = ( ( float ) ( frame_ticks - FPS_init_sync ) ) / ( float ) FPS_count_sync ;
        fps_partial = 1000.0 / ticks_per_frame ;

        if ( fps_partial == fps_value )
        {
            FPS_init_sync = frame_ticks ;
            FPS_count_sync = 0 ;
            jump = 0;
        }
        else if ( fps_partial > fps_value )
        {
            int delay = FPS_count_sync * frame_ms - ( frame_ticks - FPS_init_sync ) ;

            if ( delay > 0 )
            {
#ifdef PORTABLE_RUNTIME_DIAG
                diag_delay = delay;
                diag_delay_total += delay;
                diag_delay_count++;
#endif
#if defined(TARGET_GP2X_WIZ) || defined(TARGET_CAANOO)
                {
                    unsigned long ta = bgdrtm_ptimer_get_ticks_us(), te = ta + delay * 1000;
                    if ( ta > te ) while ( bgdrtm_ptimer_get_ticks_us() > te );
                    while ( bgdrtm_ptimer_get_ticks_us() < te );
                }
#else
                SDL_Delay( delay ) ;
#endif
                /* Reajust after delay */
#if defined(TARGET_GP2X_WIZ) || defined(TARGET_CAANOO)
                frame_ticks = bgdrtm_ptimer_get_ticks_us() / 1000L;
#else
                frame_ticks = SDL_GetTicks() ;
#endif
                ticks_per_frame = ( ( float ) ( frame_ticks - FPS_init_sync ) ) / ( float ) FPS_count_sync ;
                fps_partial = 1000.0 / ticks_per_frame ;
            }

            jump = 0 ;
        }
        else
        {
            if ( jump < max_jump ) /* Como no me alcanza el tiempo, voy a hacer skip */
            {
                jump++ ; /* No dibujar el frame */
            }
            else
            {
                FPS_init_sync = frame_ticks ;
                FPS_count_sync = 0 ;
                jump = 0 ;
            }
        }
    }

#ifdef PORTABLE_RUNTIME_DIAG
    if ( jump ) diag_skip_count++;
#endif

    /* Si paso 1 segundo o mas desde la ultima lectura */
    if ( frame_ticks - FPS_init >= 1000 )
    {
        if ( fps_value )
        {
            GLODWORD( librender, SPEED_GAUGE ) = FPS_count /*fps_partial*/ * 100.0 / fps_value ;
        }
        else
        {
            GLODWORD( librender, SPEED_GAUGE ) = 100 ;
        }

        GLODWORD( librender, FPS ) = FPS_count ;

        FPS_init = frame_ticks ;
        FPS_count = 0 ;
    }

    /* Tiempo inicial del nuevo frame */
    last_frame_ticks = frame_ticks ;

#ifdef PORTABLE_RUNTIME_DIAG
    if ( PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG_TIMING" ) )
    {
        if ( !diag_last_report_ticks ) diag_last_report_ticks = frame_ticks;
        if ( frame_ticks - diag_last_report_ticks >= 1000 )
        {
            int elapsed = frame_ticks - diag_last_report_ticks;
            uint32_t frames = frame_count - diag_last_report_frame;
            float actual_fps = elapsed > 0 ? ( ( float )frames * 1000.0f ) / ( float )elapsed : 0.0f;
            float frame_time = * ( float * ) &GLODWORD( librender, FRAME_TIME );
            PORTABLE_DIAG_LOG( "TIMING",
                               "frame_summary elapsed_ms=%d frames=%u actual_fps=%.2f target_fps=%d frame_ms=%.3f frame_time=%.4f ticks_per_frame=%.3f fps_partial=%.2f delay_count=%d delay_total_ms=%d last_delay_ms=%d jump=%d skip_count=%d max_jump=%d waitvsync=%d fps_global=%d speed_gauge=%d",
                               elapsed, frames, actual_fps, fps_value, frame_ms, frame_time, ticks_per_frame, fps_partial,
                               diag_delay_count, diag_delay_total, diag_delay, jump, diag_skip_count, max_jump, waitvsync,
                               GLODWORD( librender, FPS ), GLODWORD( librender, SPEED_GAUGE ) );
            diag_last_report_ticks = frame_ticks;
            diag_last_report_frame = frame_count;
            diag_delay_total = 0;
            diag_delay_count = 0;
            diag_skip_count = 0;
        }
    }
#endif
}

/* --------------------------------------------------------------------------- */

static SDL_Color palette[256];

void gr_refresh_palette()
{
    int n ;

    if ( sys_pixel_format->depth > 8 )
    {
        if ( sys_pixel_format->palette )
        {
            for ( n = 0 ; n < 256 ; n++ )
            {
                sys_pixel_format->palette->colorequiv[ n ] = gr_map_rgb
                        (
                            sys_pixel_format,
                            sys_pixel_format->palette->rgb[ n ].r,
                            sys_pixel_format->palette->rgb[ n ].g,
                            sys_pixel_format->palette->rgb[ n ].b
                        ) ;
            }
        }
    }
    else if ( sys_pixel_format->depth == 8 )
    {
        if ( sys_pixel_format->palette )
        {
            for ( n = 0 ; n < 256 ; n++ )
            {
                palette[ n ].r = sys_pixel_format->palette->rgb[ n ].r;
                palette[ n ].g = sys_pixel_format->palette->rgb[ n ].g;
                palette[ n ].b = sys_pixel_format->palette->rgb[ n ].b;
            }
        }
        else
        {
            uint8_t * pal = default_palette;
            for ( n = 0 ; n < 256 ; n++ )
            {
                palette[ n ].r = *pal++;
                palette[ n ].g = *pal++;
                palette[ n ].b = *pal++;
            }
        }
        SDL_SetPaletteColors(screen->format->palette, palette, 0, 256);
    }

    palette_changed = 0;
    trans_table_updated = 0 ;
}

/* --------------------------------------------------------------------------- */

void gr_draw_frame()
{
    if ( ! scr_initialized ) return ;
    if ( jump ) return ;

    /* Actualiza paleta */

    if ( palette_changed ) gr_refresh_palette();

    if ( !trans_table_updated ) gr_make_trans_table();

    /* Bloquea el bitmap de pantalla */

    if ( gr_lock_screen() < 0 ) return ;

    /* Dibuja la pantalla */

    gr_draw_screen( scrbitmap, GLODWORD( librender, RESTORETYPE ), GLODWORD( librender, DUMPTYPE ) );

    /* Fading */

    if ( fade_on || fade_set )
    {
        gr_fade_step() ;
        if ( background ) background->modified = 1 ;
    }

    /* Actualiza la paleta y la pantalla */

    gr_unlock_screen() ;

}

/* --------------------------------------------------------------------------- */

void __bgdexport( librender, module_initialize )()
{
#ifndef TARGET_DINGUX_A320
    if ( !SDL_WasInit( SDL_INIT_TIMER ) ) SDL_InitSubSystem( SDL_INIT_TIMER );
#endif
}

/* --------------------------------------------------------------------------- */

void __bgdexport( librender, module_finalize )()
{
#ifndef TARGET_DINGUX_A320
    if ( SDL_WasInit( SDL_INIT_TIMER ) ) SDL_QuitSubSystem( SDL_INIT_TIMER );
#endif
}

/* --------------------------------------------------------------------------- */
