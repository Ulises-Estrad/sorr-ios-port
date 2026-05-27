#ifndef PORTABLE_DIAG_H
#define PORTABLE_DIAG_H

#ifdef PORTABLE_RUNTIME_DIAG

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORTABLE_DIAG_ENV_CACHE_SIZE 32

typedef struct portable_diag_env_cache_entry
{
    const char * name;
    int initialized;
    int value;
} portable_diag_env_cache_entry;

static portable_diag_env_cache_entry portable_diag_env_cache[PORTABLE_DIAG_ENV_CACHE_SIZE];

static int portable_diag_env_on( const char * name )
{
    unsigned int i;

    for ( i = 0; i < PORTABLE_DIAG_ENV_CACHE_SIZE; i++ )
    {
        portable_diag_env_cache_entry * entry = &portable_diag_env_cache[i];
        if ( entry->initialized && strcmp( entry->name, name ) == 0 )
        {
            return entry->value;
        }
    }

    {
        const char * value = getenv( name );
        int enabled = value && value[0] && strcmp( value, "0" ) != 0;

        for ( i = 0; i < PORTABLE_DIAG_ENV_CACHE_SIZE; i++ )
        {
            portable_diag_env_cache_entry * entry = &portable_diag_env_cache[i];
            if ( !entry->initialized )
            {
                entry->name = name;
                entry->initialized = 1;
                entry->value = enabled;
                break;
            }
        }

        return enabled;
    }
}

static int portable_diag_category_on( const char * category )
{
    if ( portable_diag_env_on( "SORR_PORTABLE_DIAG" ) ) return 1;
    if ( strcmp( category, "AUDIO" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_AUDIO" );
    if ( strcmp( category, "EVENT" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_EVENTS" );
    if ( strcmp( category, "FILE" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_FILES" );
    if ( strcmp( category, "INPUT" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_INPUT" );
    if ( strcmp( category, "LOOP" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_LOOP" );
    if ( strcmp( category, "PALETTE" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_PALETTE" );
    if ( strcmp( category, "RENDER" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_RENDER" );
    if ( strcmp( category, "SCRIPT" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_SCRIPT" );
    if ( strcmp( category, "TIMING" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_TIMING" );
    if ( strcmp( category, "VIDEO" ) == 0 ) return portable_diag_env_on( "SORR_PORTABLE_DIAG_VIDEO" );
    return 0;
}

static void portable_diag_log( const char * category, const char * format, ... )
{
    va_list args;

    if ( !portable_diag_category_on( category ) ) return;

    fprintf( stderr, "[PORTABLE:%s] ", category );
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    fputc( '\n', stderr );
    fflush( stderr );
}

#define PORTABLE_DIAG_LOG(category, ...) portable_diag_log((category), __VA_ARGS__)
#define PORTABLE_DIAG_ENV_ON(name) portable_diag_env_on((name))

#else

#define PORTABLE_DIAG_LOG(category, ...) ((void)0)
#define PORTABLE_DIAG_ENV_ON(name) 0

#endif

#endif
