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
#include <math.h>
#include <time.h>

#include "bgddl.h"

#include "xctype.h"

#include "libkey.h"

/* --------------------------------------------------------------------------- */

#ifdef __GNUC__
#define _inline inline
#endif

/* --------------------------------------------------------------------------- */

#ifdef TARGET_IOS
static volatile unsigned char sorr_ios_touch_bennu_keys[127];

void sorr_ios_touch_set_bennu_key( int code, int pressed )
{
    if ( code < 0 || code >= ( int )sizeof( sorr_ios_touch_bennu_keys ) )
    {
        return;
    }

    sorr_ios_touch_bennu_keys[code] = pressed ? 1 : 0;
}

int sorr_ios_touch_get_bennu_key( int code )
{
    if ( code < 0 || code >= ( int )sizeof( sorr_ios_touch_bennu_keys ) )
    {
        return 0;
    }

    return sorr_ios_touch_bennu_keys[code] ? 1 : 0;
}
#endif

/* --------------------------------------------------------------------------- */

#ifdef PORTABLE_RUNTIME_DIAG
static int portable_input_diag_enabled = -1;
static unsigned char portable_key_poll_logged[127];
static unsigned char portable_key_last_state[127];

static int portable_input_diag( void )
{
    if ( portable_input_diag_enabled < 0 )
    {
        portable_input_diag_enabled = getenv( "PORTABLE_INPUT_DIAG" ) ? 1 : 0;
    }
    return portable_input_diag_enabled;
}

static int portable_key_is_watched( int code )
{
    switch ( code )
    {
        case 28: /* Enter */
        case 45: /* X */
        case 46: /* C */
        case 47: /* V */
        case 48: /* B */
        case 72: /* Up */
        case 75: /* Left */
        case 77: /* Right */
        case 80: /* Down */
            return 1;
    }
    return 0;
}
#endif

/* --------------------------------------------------------------------------- */
/*
 *  FUNCTION : _get_key
 *
 *  Returns the current status of a key (pressed or not)
 *
 *  PARAMS :
 *      key             Internal code of the key
 *
 *  RETURN VALUE :
 *      A non-zero positive value if the key is pressed, 0 otherwise
 */

static _inline int _get_key( int code )
{
    key_equiv * curr ;
    int found = 0 ;

#ifdef TARGET_IOS
    if ( code < 0 || code >= 127 ) return 0;
    if ( sorr_ios_touch_get_bennu_key( code ) ) return 1;
#endif

    if ( !keystate ) return 0;

    curr = &key_table[code] ;

    while ( curr && found == 0 )
    {
        found = keystate[curr->sdlk_equiv] ;
        curr = curr->next ;
    }

    return found ;
}

/* --------------------------------------------------------------------------- */

static int modkey_key( INSTANCE * my, int * params )
{
    int code = params[0];
    int result = _get_key( code );

#ifdef PORTABLE_RUNTIME_DIAG
    if ( portable_input_diag() && code >= 0 && code < 127 && portable_key_is_watched( code ) )
    {
        if ( !portable_key_poll_logged[code] )
        {
            fprintf( stderr, "[PORTABLE:INPUT] KEY poll code=%d initial=%d\n", code, result );
            portable_key_poll_logged[code] = 1;
            portable_key_last_state[code] = ( unsigned char )result;
        }
        else if ( portable_key_last_state[code] != ( unsigned char )result )
        {
            fprintf( stderr, "[PORTABLE:INPUT] KEY state code=%d value=%d\n", code, result );
            portable_key_last_state[code] = ( unsigned char )result;
        }
    }
#endif

    return result;
}

/* --------------------------------------------------------------------------- */

DLSYSFUNCS  __bgdexport( mod_key, functions_exports )[] =
{
    { "KEY" , "I"   , TYPE_INT  , modkey_key   },
    { 0     , 0     , 0         , 0            }
};

/* --------------------------------------------------------------------------- */

char * __bgdexport( mod_key, modules_dependency )[] =
{
    "libkey",
    NULL
};

/* --------------------------------------------------------------------------- */
