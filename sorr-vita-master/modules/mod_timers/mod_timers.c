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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bgddl.h"

#include <SDL.h>

#include "dlvaracc.h"

#ifdef PORTABLE_RUNTIME_DIAG
#include "portable_diag.h"
#endif

/* ----------------------------------------------------------------- */

enum {
    TIMER = 0
};

/* ----------------------------------------------------------------- */
/* Definicion de variables globales (usada en tiempo de compilacion) */

char * __bgdexport( mod_timers, globals_def ) = "timer[9];\n";

/* ----------------------------------------------------------------- */
/* Son las variables que se desea acceder.                           */
/* El interprete completa esta estructura, si la variable existe.    */
/* (usada en tiempo de ejecucion)                                    */

DLVARFIXUP __bgdexport( mod_timers, globals_fixup )[] =
{
    /* Nombre de variable global, puntero al dato, tamaño del elemento, cantidad de elementos */
    { "timer"   , NULL, -1, -1 },
    { NULL, NULL, -1, -1 }
};

/* ----------------------------------------------------------------- */
/*
 *  FUNCTION : _advance_timers
 *
 *  Update the value of all global timers
 *
 *  PARAMS :
 *      None
 *
 *  RETURN VALUE :
 *      None
 */

static void _advance_timers( void )
{
    int * timer, i ;
    int curr_ticktimer = SDL_GetTicks() ;
    static int initial_ticktimer[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ;
    static int ltimer[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1} ; // -1 to force initial_ticktimer update

    /* TODO: Here add checking for console_mode, don't advance in this mode */
    timer = ( int * ) ( &GLODWORD( mod_timers, TIMER ) );
    for ( i = 0 ; i < 10 ; i++ )
    {
        if ( timer[i] != ltimer[i] ) initial_ticktimer[i] = curr_ticktimer - ( timer[i] * 10 ) ;
        ltimer[i] = timer[i] = ( curr_ticktimer - initial_ticktimer[i] ) / 10 ;
    }

#ifdef PORTABLE_RUNTIME_DIAG
    if ( PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG_TIMING" ) )
    {
        static int diag_last_ticktimer = 0;
        if ( !diag_last_ticktimer ) diag_last_ticktimer = curr_ticktimer;
        if ( curr_ticktimer - diag_last_ticktimer >= 1000 )
        {
            PORTABLE_DIAG_LOG( "TIMING", "timer_summary tick_ms=%d timer0=%d timer1=%d timer2=%d timer3=%d timer4=%d timer5=%d timer6=%d timer7=%d timer8=%d timer9=%d",
                               curr_ticktimer, timer[0], timer[1], timer[2], timer[3], timer[4], timer[5], timer[6], timer[7], timer[8], timer[9] );
            diag_last_ticktimer = curr_ticktimer;
        }
    }
#endif
}

/* ----------------------------------------------------------------- */

/* Bigest priority first execute
   Lowest priority last execute */

HOOK __bgdexport( mod_timers, handler_hooks )[] =
{
    { 100, _advance_timers },
    {   0, NULL            }
} ;

