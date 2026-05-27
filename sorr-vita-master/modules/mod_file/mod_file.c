/*
 *  Copyright � 2006-2012 SplinterGU (Fenix/Bennugd)
 *  Copyright � 2002-2006 Fenix Team (Fenix)
 *  Copyright � 1999-2002 Jos� Luis Cebri�n Pag�e (Fenix)
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

#include "bgddl.h"

#include "arrange.h"
#include "files.h"
#include "xstrings.h"
#include "dcb.h"
#include "varspace_file.h"

#ifdef PORTABLE_RUNTIME_DIAG
#include "portable_diag.h"
#endif

#if defined(_WIN64)
#define MODFILE_X64_HANDLE_TABLE_SIZE 4096

static file * modfile_x64_handles[MODFILE_X64_HANDLE_TABLE_SIZE];

static int modfile_x64_store_handle( file * fp, const char * label )
{
    int i;

    if ( !fp ) return 0;

    for ( i = 1; i < MODFILE_X64_HANDLE_TABLE_SIZE; i++ )
    {
        if ( !modfile_x64_handles[i] )
        {
            modfile_x64_handles[i] = fp;
#ifdef PORTABLE_RUNTIME_DIAG
            PORTABLE_DIAG_LOG( "FILE", "x64 file handle store label=%s handle=%d fp=%p", label ? label : "(null)", i, ( void * )fp );
#endif
            return i;
        }
    }

#ifdef PORTABLE_RUNTIME_DIAG
    PORTABLE_DIAG_LOG( "FILE", "x64 file handle table full label=%s fp=%p", label ? label : "(null)", ( void * )fp );
#endif
    file_close( fp );
    return 0;
}

static file * modfile_x64_get_handle( int handle, const char * op )
{
    file * fp = NULL;

    if ( handle > 0 && handle < MODFILE_X64_HANDLE_TABLE_SIZE )
    {
        fp = modfile_x64_handles[handle];
    }

#ifdef PORTABLE_RUNTIME_DIAG
    PORTABLE_DIAG_LOG( "FILE", "x64 file handle get op=%s handle=%d fp=%p", op ? op : "(null)", handle, ( void * )fp );
#endif
    return fp;
}

static file * modfile_x64_release_handle( int handle, const char * op )
{
    file * fp = modfile_x64_get_handle( handle, op );

    if ( handle > 0 && handle < MODFILE_X64_HANDLE_TABLE_SIZE )
    {
        modfile_x64_handles[handle] = NULL;
    }

#ifdef PORTABLE_RUNTIME_DIAG
    PORTABLE_DIAG_LOG( "FILE", "x64 file handle release op=%s handle=%d fp=%p", op ? op : "(null)", handle, ( void * )fp );
#endif
    return fp;
}
#else
#define modfile_x64_store_handle(fp,label) (( int )( fp ))
#define modfile_x64_get_handle(handle,op) (( file * )( handle ))
#define modfile_x64_release_handle(handle,op) (( file * )( handle ))
#endif

/* ----------------------------------------------------------------- */

DLCONSTANT  __bgdexport( mod_file, constants_def)[] =
{
    { "O_READ"      , TYPE_INT, 0  },
    { "O_READWRITE" , TYPE_INT, 1  },
    { "O_RDWR"      , TYPE_INT, 1  },
    { "O_WRITE"     , TYPE_INT, 2  },
    { "O_ZREAD"     , TYPE_INT, 3  },
    { "O_ZWRITE"    , TYPE_INT, 4  },

    { "SEEK_SET"    , TYPE_INT, 0  },
    { "SEEK_CUR"    , TYPE_INT, 1  },
    { "SEEK_END"    , TYPE_INT, 2  },

    { NULL          , 0       , 0  }
} ;

/* ----------------------------------------------------------------- */

static int modfile_save( INSTANCE * my, int * params )
{
    file * fp ;
    const char * filename ;
    int result = 0 ;

    filename = string_get( params[0] ) ;
    if ( !filename ) return 0 ;

    fp = file_open( filename, "wb0" ) ;
    if ( fp )
    {
        result = savetypes( fp, ( void * )params[1], ( void * )params[2], params[3], 0 );
        file_close( fp ) ;
    }
    string_discard( params[0] ) ;
    return result ;
}

static int modfile_load( INSTANCE * my, int * params )
{
    file * fp ;
    const char * filename ;
    int result = 0 ;

    filename = string_get( params[0] ) ;
    if ( !filename ) return 0 ;

    fp = file_open( filename, "rb0" ) ;
    if ( fp )
    {
        result = loadtypes( fp, ( void * )params[1], ( void * )params[2], params[3], 0 );
        file_close( fp ) ;
    }
    string_discard( params[0] ) ;
    return result ;
}

static int modfile_fopen( INSTANCE * my, int * params )
{
    static char * ops[] = { "rb0", "r+b0", "wb0", "rb", "wb6" } ;
    int r ;

    if ( params[1] < 0 || params[1] > 4 )
        params[0] = 0 ;

    const char *p = string_get(params[0]);

    file *fp = file_open( p, ops[params[1]] );
//    SDL_Log("Calling file_open(%s, %s) -> %d", p, ops[params[1]], r);

#if defined(_WIN64)
#ifdef PORTABLE_RUNTIME_DIAG
    PORTABLE_DIAG_LOG( "FILE", "x64 FOPEN path=%s mode=%s fp=%p", p ? p : "(null)", ops[params[1]], ( void * )fp );
#endif
#endif
    r = modfile_x64_store_handle( fp, "FOPEN" ) ;
    string_discard( params[0] ) ;
    return r ;
}

static int modfile_fclose( INSTANCE * my, int * params )
{
//    SDL_Log("Called fclose");
    file * fp = modfile_x64_release_handle( params[0], "FCLOSE" );
    if ( !fp ) return 0;
    file_close( fp ) ;
    return 1 ;
}

static int modfile_fread( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[0], "FREAD" );
    if ( !fp ) return 0;
    return loadtypes( fp, ( void * )params[1], ( void * )params[2], params[3], 0 );
}

static int modfile_fwrite( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[0], "FWRITE" );
    if ( !fp ) return 0;
    return savetypes( fp, ( void * )params[1], ( void * )params[2], params[3], 0 );
}

static int modfile_freadC( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[2], "FREADC" );
    if ( !fp ) return 0;
    return file_read( fp, ( void * )params[0], params[1] );
}

static int modfile_fwriteC( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[2], "FWRITEC" );
    if ( !fp ) return 0;
    return file_write( fp, ( void * )params[0], params[1] );
}

static int modfile_fseek( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[0], "FSEEK" );
    if ( !fp ) return 0;
    return file_seek( fp, params[1], params[2] ) ;
}

static int modfile_frewind( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[0], "FREWIND" );
    if ( !fp ) return 0;
    file_rewind( fp ) ;
    return 1;
}

static int modfile_ftell( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[0], "FTELL" );
    if ( !fp ) return 0;
    return file_pos( fp ) ;
}

static int modfile_fflush( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[0], "FFLUSH" );
    if ( !fp ) return 0;
    return file_flush( fp ) ;
}

static int modfile_filelength( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[0], "FLENGTH" );
    if ( !fp ) return 0;
    return file_size( fp ) ;
}

static int modfile_fputs( INSTANCE * my, int * params )
{
    char * str = ( char * ) string_get( params[1] );
    file * fp = modfile_x64_get_handle( params[0], "FPUTS" );
    if ( !fp ) { string_discard( params[1] ) ; return 0; }
    int r = file_puts( fp, str ) ;
    if ( str[strlen( str )-1] != '\n' ) file_puts( fp, "\r\n" ) ;
    /*    int r = file_puts ((file *)params[0], string_get(params[1])) ; */
    string_discard( params[1] ) ;
    return r ;
}

static int modfile_fgets( INSTANCE * my, int * params )
{
    char buffer[1025] ;
    int len, done = 0 ;
    int str = string_new( "" );

    while ( !done )
    {
        file * fp = modfile_x64_get_handle( params[0], "FGETS" );
        if ( !fp ) break;
        len = file_gets( fp, buffer, sizeof( buffer ) - 1) ;
        if ( len < 1 ) break;

        if ( buffer[len-1] == '\r' || buffer[len-1] == '\n' )
        {
            len--;
            if ( len && ( buffer[len-1] == '\r' || buffer[len-1] == '\n' )) len--;
            buffer[len] = '\0' ;
            done = 1;
        }
        string_concat( str, buffer );
    }
//    SDL_Log("Called fgets() -> %s", string_get(str));
    string_use( str ) ;
    return str ;
}

static int modfile_file( INSTANCE * my, int * params )
{
    char buffer[1025] ;
    int str = string_new( "" ) ;
    file * f ;
    int l;

    f = file_open( string_get( params[0] ), "rb" ) ;
    string_discard( params[0] ) ;

    if ( f )
    {
        while ( !file_eof( f ) )
        {
            l = file_read( f, buffer, sizeof( buffer ) - 1 ) ;
            buffer[l] = '\0' ;
            if ( l )
            {
                string_concat( str, buffer ) ;
                buffer[0] = '\0' ;
            }
            else
                break;
        }
        file_close( f ) ;
    }

    string_use( str ) ;

    return str ;
}

static int modfile_feof( INSTANCE * my, int * params )
{
    file * fp = modfile_x64_get_handle( params[0], "FEOF" );
    if ( !fp ) return 1;
    return file_eof( fp ) ;
}

static int modfile_exists( INSTANCE * my, int * params )
{
    const char *p = string_get(params[0]);
    int r = file_exists( p ) ;
//    SDL_Log("Wondering if file_exists(%s) -> %d", p, r);
    string_discard( params[0] ) ;
    return r ;
}

static int modfile_remove( INSTANCE * my, int * params )
{
    int r = file_remove( string_get( params[0] ) ) ;
    string_discard( params[0] ) ;
    return r ;
}

static int modfile_move( INSTANCE * my, int * params )
{
    int r = file_move( string_get( params[0] ), string_get( params[1] ) ) ;
    string_discard( params[1] ) ;
    string_discard( params[0] ) ;
    return r ;
}

/* ----------------------------------------------------------------- */
/* Declaracion de funciones                                          */

DLSYSFUNCS  __bgdexport( mod_file, functions_exports)[] =
{
    /* Ficheros */
    { "SAVE"        , "SV++" , TYPE_INT         , modfile_save        },
    { "LOAD"        , "SV++" , TYPE_INT         , modfile_load        },
    { "FOPEN"       , "SI"   , TYPE_INT         , modfile_fopen       },
    { "FCLOSE"      , "I"    , TYPE_INT         , modfile_fclose      },
    { "FREAD"       , "IV++" , TYPE_INT         , modfile_fread       },
    { "FREAD"       , "PII"  , TYPE_INT         , modfile_freadC      },
    { "FWRITE"      , "IV++" , TYPE_INT         , modfile_fwrite      },
    { "FWRITE"      , "PII"  , TYPE_INT         , modfile_fwriteC     },
    { "FSEEK"       , "III"  , TYPE_INT         , modfile_fseek       },
    { "FREWIND"     , "I"    , TYPE_UNDEFINED   , modfile_frewind     },
    { "FTELL"       , "I"    , TYPE_INT         , modfile_ftell       },
    { "FFLUSH"      , "I"    , TYPE_INT         , modfile_fflush      },
    { "FLUSH"       , "I"    , TYPE_INT         , modfile_fflush      },
    { "FLENGTH"     , "I"    , TYPE_INT         , modfile_filelength  },
    { "FPUTS"       , "IS"   , TYPE_INT         , modfile_fputs       },
    { "FGETS"       , "I"    , TYPE_STRING      , modfile_fgets       },
    { "FEOF"        , "I"    , TYPE_INT         , modfile_feof        },
    { "FILE"        , "S"    , TYPE_STRING      , modfile_file        },
    { "FEXISTS"     , "S"    , TYPE_INT         , modfile_exists      } ,
    { "FILE_EXISTS" , "S"    , TYPE_INT         , modfile_exists      } ,
    { "FREMOVE"     , "S"    , TYPE_INT         , modfile_remove      } ,
    { "FMOVE"       , "SS"   , TYPE_INT         , modfile_move        } ,
    { 0             , 0      , 0                , 0                   }
};
