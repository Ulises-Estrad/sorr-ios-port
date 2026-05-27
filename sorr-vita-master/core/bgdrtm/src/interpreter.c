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
#include <stdint.h>

#include "bgdrtm.h"
#include "dcb.h"

#include "sysprocs_p.h"
#include "pslang.h"
#include "instance.h"
#include "offsets.h"
#include "xstrings.h"
#include "files.h"
#include "varspace_file.h"

#ifdef PORTABLE_RUNTIME_DIAG
#include <SDL.h>
#include "portable_diag.h"
#endif

#include <assert.h>
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define PORTABLE_X64_STACK_PTR_TABLE_SIZE 262144u

typedef struct portable_x64_stack_ptr_entry
{
    int * cell;
    uint32_t low_value;
    uintptr_t full_ptr;
} portable_x64_stack_ptr_entry;

static portable_x64_stack_ptr_entry portable_x64_stack_ptr_table[PORTABLE_X64_STACK_PTR_TABLE_SIZE];

static unsigned int portable_x64_stack_ptr_hash( const int * cell )
{
    uintptr_t value = ( uintptr_t )cell;
    value >>= 2;
    value ^= value >> 16;
    value ^= value >> 32;
    return ( unsigned int )value & ( PORTABLE_X64_STACK_PTR_TABLE_SIZE - 1u );
}

static void portable_x64_stack_set_ptr( int * cell, const void * ptr )
{
    uintptr_t full_ptr = ( uintptr_t )ptr;
    uint32_t low_value = ( uint32_t )full_ptr;
    unsigned int slot = portable_x64_stack_ptr_hash( cell );
    unsigned int i;

    *cell = ( int )low_value;

    for ( i = 0; i < PORTABLE_X64_STACK_PTR_TABLE_SIZE; i++ )
    {
        portable_x64_stack_ptr_entry * entry = &portable_x64_stack_ptr_table[( slot + i ) & ( PORTABLE_X64_STACK_PTR_TABLE_SIZE - 1u )];
        if ( entry->cell == NULL || entry->cell == cell )
        {
            entry->cell = cell;
            entry->low_value = low_value;
            entry->full_ptr = full_ptr;
#ifdef PORTABLE_RUNTIME_DIAG
            if ( PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG_PTRS" ) )
            {
                PORTABLE_DIAG_LOG( "SCRIPT", "x64 ptr-set cell=%p low=0x%08x full=%p", ( void * )cell, ( unsigned int )low_value, ( void * )full_ptr );
            }
#endif
            return;
        }
    }

#ifdef PORTABLE_RUNTIME_DIAG
    PORTABLE_DIAG_LOG( "SCRIPT", "x64 ptr-set table full cell=%p full=%p", ( void * )cell, ( void * )full_ptr );
#endif
}

static void * portable_x64_stack_get_ptr( int * cell )
{
    uint32_t low_value = ( uint32_t )*cell;
    unsigned int slot = portable_x64_stack_ptr_hash( cell );
    unsigned int i;

    for ( i = 0; i < PORTABLE_X64_STACK_PTR_TABLE_SIZE; i++ )
    {
        portable_x64_stack_ptr_entry * entry = &portable_x64_stack_ptr_table[( slot + i ) & ( PORTABLE_X64_STACK_PTR_TABLE_SIZE - 1u )];
        if ( entry->cell == NULL ) break;
        if ( entry->cell == cell )
        {
            if ( entry->low_value == low_value )
            {
#ifdef PORTABLE_RUNTIME_DIAG
                if ( PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG_PTRS" ) )
                {
                    PORTABLE_DIAG_LOG( "SCRIPT", "x64 ptr-get hit cell=%p low=0x%08x full=%p", ( void * )cell, ( unsigned int )low_value, ( void * )entry->full_ptr );
                }
#endif
                return ( void * )entry->full_ptr;
            }

            entry->cell = NULL;
            entry->low_value = 0;
            entry->full_ptr = 0;
            break;
        }
    }

#ifdef PORTABLE_RUNTIME_DIAG
    if ( PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG_PTRS" ) )
    {
        PORTABLE_DIAG_LOG( "SCRIPT", "x64 ptr-get miss cell=%p low=0x%08x fallback=%p", ( void * )cell, ( unsigned int )low_value, ( void * )( uintptr_t )low_value );
    }
#endif
    return ( void * )( uintptr_t )low_value;
}

static int portable_x64_stack_peek_ptr( int * cell, uintptr_t * full_ptr )
{
    uint32_t low_value = ( uint32_t )*cell;
    unsigned int slot = portable_x64_stack_ptr_hash( cell );
    unsigned int i;

    for ( i = 0; i < PORTABLE_X64_STACK_PTR_TABLE_SIZE; i++ )
    {
        portable_x64_stack_ptr_entry * entry = &portable_x64_stack_ptr_table[( slot + i ) & ( PORTABLE_X64_STACK_PTR_TABLE_SIZE - 1u )];
        if ( entry->cell == NULL ) break;
        if ( entry->cell == cell )
        {
            if ( entry->low_value == low_value )
            {
                *full_ptr = entry->full_ptr;
                return 1;
            }
            break;
        }
    }
    return 0;
}

static void portable_x64_stack_copy_cell( int * dst, int * src )
{
    uintptr_t full_ptr;

    if ( portable_x64_stack_peek_ptr( src, &full_ptr ) )
    {
        portable_x64_stack_set_ptr( dst, ( void * )full_ptr );
    }
    else
    {
        *dst = *src;
    }
}

static void portable_x64_stack_adjust_ptr( int * cell, intptr_t offset )
{
    uintptr_t full_ptr;

    if ( portable_x64_stack_peek_ptr( cell, &full_ptr ) )
    {
        portable_x64_stack_set_ptr( cell, ( void * )( full_ptr + offset ) );
    }
    else
    {
        *cell += ( int )offset;
    }
}

void * portable_x64_sysproc_pointer_param( int * cell )
{
    return portable_x64_stack_get_ptr( cell );
}
static int portable_x64_sysproc_get_desktop_size( INSTANCE * r, int * params )
{
    int * width = ( int * )portable_x64_stack_get_ptr( &params[0] );
    int * height = ( int * )portable_x64_stack_get_ptr( &params[1] );

    if ( width ) *width = GetSystemMetrics( SM_CXSCREEN );
    if ( height ) *height = GetSystemMetrics( SM_CYSCREEN );

#ifdef PORTABLE_RUNTIME_DIAG
    PORTABLE_DIAG_LOG( "SCRIPT", "x64 bridged GET_DESKTOP_SIZE width_ptr=%p height_ptr=%p width=%d height=%d", ( void * )width, ( void * )height, width ? *width : 0, height ? *height : 0 );
#endif
    return 1;
}

static int portable_x64_sysproc_load_save( int is_save, int * params )
{
    file * fp;
    const char * filename;
    void * data;
    void * vars;
    int result = 0;

    filename = string_get( params[0] );
    if ( !filename ) return 0;

    data = portable_x64_stack_get_ptr( &params[1] );
    vars = portable_x64_stack_get_ptr( &params[2] );

    fp = file_open( filename, is_save ? "wb0" : "rb0" );
    if ( fp )
    {
        result = is_save ? savetypes( fp, data, vars, params[3], 0 ) : loadtypes( fp, data, vars, params[3], 0 );
        file_close( fp );
    }

#ifdef PORTABLE_RUNTIME_DIAG
    PORTABLE_DIAG_LOG( "SCRIPT", "x64 bridged %s filename=%s data=%p vars=%p count=%d result=%d", is_save ? "SAVE" : "LOAD", filename, data, vars, params[3], result );
#endif
    string_discard( params[0] );
    return result;
}
#endif

#if defined(PORTABLE_RUNTIME_DIAG) && (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

static int portable_x64_ptr_looks_readable( const void * ptr )
{
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = ( uintptr_t )ptr;

    if ( address < 0x10000 ) return 0;
    if ( !VirtualQuery( ptr, &mbi, sizeof( mbi ) ) ) return 0;
    if ( mbi.State != MEM_COMMIT ) return 0;
    if ( mbi.Protect & ( PAGE_NOACCESS | PAGE_GUARD ) ) return 0;

    return 1;
}

static void portable_x64_log_mn_a2str( INSTANCE * r, int * ptr, int arg )
{
    int slot = -arg - 1;
    int depth = ( int )( r->stack_ptr - r->stack );
    int i;
    uintptr_t cell_ptr;
    char * stitched_ptr;

    if ( !PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG_SCRIPT" ) &&
         !PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG" ) )
    {
        return;
    }

    cell_ptr = ( uintptr_t )( uint32_t )r->stack_ptr[slot];
    stitched_ptr = *( char ** )( &r->stack_ptr[slot] );

    PORTABLE_DIAG_LOG(
        "SCRIPT",
        "MN_A2STR proc=%s id=%d code_offset=%ld arg=%d slot=%d depth=%d stack=%p stack_ptr=%p cell_addr=%p raw0=0x%08x raw1=0x%08x cell_ptr=%p cell_readable=%d stitched_ptr=%p stitched_readable=%d",
        r->proc ? r->proc->name : "(null)",
        LOCDWORD( r, PROCESS_ID ),
        ( long )( ptr - r->code ),
        arg,
        slot,
        depth,
        ( void * )r->stack,
        ( void * )r->stack_ptr,
        ( void * )&r->stack_ptr[slot],
        ( unsigned int )r->stack_ptr[slot],
        ( unsigned int )r->stack_ptr[slot + 1],
        ( void * )cell_ptr,
        portable_x64_ptr_looks_readable( ( const void * )cell_ptr ),
        ( void * )stitched_ptr,
        portable_x64_ptr_looks_readable( stitched_ptr )
    );

    for ( i = -4; i <= 4; i++ )
    {
        int rel = slot + i;
        if ( rel >= -depth && rel <= 0 )
        {
            PORTABLE_DIAG_LOG(
                "SCRIPT",
                "MN_A2STR stack[%+d] addr=%p value=0x%08x signed=%d",
                i,
                ( void * )&r->stack_ptr[rel],
                ( unsigned int )r->stack_ptr[rel],
                r->stack_ptr[rel]
            );
        }
    }
}

static void portable_x64_log_step( INSTANCE * r, int * ptr )
{
    static unsigned int step_log_count = 0;
    int depth;
    int top0 = 0;
    int top1 = 0;

    if ( !PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG_STEPS" ) ) return;
    if ( step_log_count >= 20000 ) return;

    depth = ( int )( r->stack_ptr - r->stack );
    if ( depth > 0 ) top0 = r->stack_ptr[-1];
    if ( depth > 1 ) top1 = r->stack_ptr[-2];

    PORTABLE_DIAG_LOG(
        "SCRIPT",
        "step=%u proc=%s id=%d code_offset=%ld opcode=0x%08x param=%d depth=%d top0=0x%08x top1=0x%08x",
        step_log_count,
        r->proc ? r->proc->name : "(null)",
        LOCDWORD( r, PROCESS_ID ),
        ( long )( ptr - r->code ),
        ( unsigned int )*ptr,
        ptr[1],
        depth,
        ( unsigned int )top0,
        ( unsigned int )top1
    );

    step_log_count++;
}
#endif
/* ---------------------------------------------------------------------- */
/* Interpreter's main module                                              */
/* ---------------------------------------------------------------------- */

int debug_mode = 0 ;

int exit_value = 0 ;
int must_exit = 0 ;

int force_debug = 0 ;
int debug_next = 0 ;

int trace_sentence  = -1;
INSTANCE * trace_instance = NULL;

/* ---------------------------------------------------------------------- */

static INSTANCE * last_instance_run = NULL;

/* ---------------------------------------------------------------------- */

static int stack_dump( INSTANCE * r )
{
    register int * ptr = &r->stack[1] ;
    register int i = 0;

    while ( ptr < r->stack_ptr )
    {
        if ( i == 5 )
        {
            i = 0;
            printf( "\n" );
        }
        printf( "%08X ", *ptr++ ) ;
        i++;
    }

    return i;
}

/* ---------------------------------------------------------------------- */

int instance_go_all()
{
    INSTANCE * i = NULL;
    int i_count = 0 ;
    int n;
    int status;
    int loop_count = 0;

    must_exit = 0 ;
    PORTABLE_DIAG_LOG( "LOOP", "instance_go_all enter first_instance=%p", first_instance );

    while ( first_instance )
    {
        loop_count++;
        if ( loop_count <= 20 || ( loop_count % 1000 ) == 0 )
            PORTABLE_DIAG_LOG( "LOOP", "instance_go_all loop=%d first_instance=%p last_instance_run=%p", loop_count, first_instance, last_instance_run );

        if ( debug_mode )
        {
            /* Hook */
            if ( handler_hook_count )
                for ( n = 0; n < handler_hook_count; n++ )
                    handler_hook_list[n].hook();
            /* Hook */
            if ( must_exit ) break ;

        }
        else
        {
            if ( last_instance_run )
            {
                if ( instance_exists( last_instance_run ) )
                {
                    i = last_instance_run;
                }
                else
                {
                    last_instance_run = NULL;
                    i = instance_next_by_priority();
                }
            }
            else
            {
                i = instance_next_by_priority();
                i_count = 0 ;
            }

            while ( i )
            {
                status = LOCDWORD( i, STATUS );
                /* If instance is KILLED or DEAD or return from some debug command, then execute it again.
                   No exec_hook is executed.
                 */
                if ( status == STATUS_KILLED || status == STATUS_DEAD || last_instance_run )
                {
                    /* Run instance */
                }
                else if ( status == STATUS_RUNNING && LOCINT32( i, FRAME_PERCENT ) < 100 )
                {
                    /* Run instance */
                    /* Hook */
                    if ( process_exec_hook_count )
                        for ( n = 0; n < process_exec_hook_count; n++ )
                            process_exec_hook_list[n]( i );
                    /* Hook */
                }
                else
                {
                    i = instance_next_by_priority();
                    last_instance_run = NULL;
                    continue;
                }

                i_count++;

                last_instance_run = NULL;

                if ( loop_count <= 20 )
                    PORTABLE_DIAG_LOG( "SCRIPT", "instance_go_all run loop=%d proc=%s id=%d status=%d frame_percent=%d", loop_count, i->proc ? i->proc->name : "(null)", LOCDWORD( i, PROCESS_ID ), status, LOCINT32( i, FRAME_PERCENT ) );

                instance_go( i );

                if ( force_debug )
                {
                    debug_mode = 1;
                    last_instance_run  = trace_instance;
                    break;
                }

                if ( must_exit ) break;

                i = instance_next_by_priority();
            }

            if ( must_exit ) break ;

            /* If frame is complete, then update internal vars and execute main hooks. */

            if ( !i_count && !force_debug )
            {
                /* Honors the signal-changed status of the process and
                 * saves so it is used in this loop the next frame
                 */

                i = first_instance ;
                while ( i )
                {
                    status = LOCDWORD( i, STATUS );
                    if ( status == STATUS_RUNNING ) LOCINT32( i, FRAME_PERCENT ) -= 100 ;
                    LOCDWORD( i, SAVED_STATUS ) = status ;

                    if ( LOCINT32( i, SAVED_PRIORITY ) != LOCINT32( i, PRIORITY ) )
                    {
                        LOCINT32( i, SAVED_PRIORITY ) = LOCINT32( i, PRIORITY );
                        instance_dirty( i );
                    }

                    i = i->next ;
                }

                if ( !first_instance ) break ;

                /* Hook */
                if ( handler_hook_count )
                    for ( n = 0; n < handler_hook_count; n++ )
                        handler_hook_list[n].hook();
                /* Hook */

                continue ;
            }
        }
    }

    PORTABLE_DIAG_LOG( "LOOP", "instance_go_all leave exit_value=%d must_exit=%d loops=%d", exit_value, must_exit, loop_count );
    return exit_value;

}

/* ---------------------------------------------------------------------- */

int instance_go( INSTANCE * r )
{
    if ( !r ) return 0 ;

    register int * ptr = r->codeptr ;

    int n, return_value = LOCDWORD( r, PROCESS_ID ) ;
    SYSPROC * p = NULL ;
    INSTANCE * i = NULL ;
    static char buffer[16];
    char * str = NULL ;
    int status ;
    static int entry_log_count = 0;
    static int frame_log_count = 0;

    /* Pointer to the current process's code (it may be a called one) */

    int child_is_alive = 0;

    /* ------------------------------------------------------------------------------- */
    /* Restore if exit by debug                                                        */

    if ( debug > 0 )
    {
        printf( "\n>>> Instance:%s ProcID:%d StackUsed:%d/%d\n", r->proc->name,
                                                                 LOCDWORD( r, PROCESS_ID ),
                                                                 ( r->stack_ptr - r->stack ) / sizeof( r->stack[0] ),
                                                                 ( r->stack[0] & ~STACK_RETURN_VALUE )
              ) ;
    }

    if ( entry_log_count < 200 )
    {
        PORTABLE_DIAG_LOG( "SCRIPT", "instance_go enter proc=%s id=%d status=%d code_offset=%ld", r->proc ? r->proc->name : "(null)", LOCDWORD( r, PROCESS_ID ), LOCDWORD( r, STATUS ), ( long )( r->codeptr - r->code ) );
        entry_log_count++;
    }

    /* Hook */
    if ( instance_pre_execute_hook_count )
        for ( n = 0; n < instance_pre_execute_hook_count; n++ )
            instance_pre_execute_hook_list[n]( r );
    /* Hook */

    if (( r->proc->breakpoint || r->breakpoint ) && trace_instance != r ) debug_next = 1;

    trace_sentence = -1;

    while ( !must_exit )
    {
        /* If I was killed or I'm waiting status, then exit */
        status = LOCDWORD( r, STATUS );
        if (( status & ~STATUS_WAITING_MASK ) == STATUS_KILLED || ( status & STATUS_WAITING_MASK ) )
        {
            r->codeptr = ptr;
            return_value = LOCDWORD( r, PROCESS_ID );
            goto break_all ;
        }

        if ( debug_next && trace_sentence != -1 )
        {
            force_debug = 1;
            debug_next = 0;
            r->codeptr = ptr ;
            return_value = LOCDWORD( r, PROCESS_ID );
            break;
        }

        /* debug output */
        if ( debug > 0 )
        {
            if ( debug > 2 )
            {
                int c = 45 - stack_dump( r ) * 9;
                if ( debug > 1 ) printf( "%*.*s[%4u] ", c, c, "", ( ptr - r->code ) ) ;
            }
            else if ( debug > 1 ) printf( "[%4u] ", ( ptr - r->code ) ) ;
            mnemonic_dump( *ptr, ptr[1] ) ;
        }

#if defined(PORTABLE_RUNTIME_DIAG) && (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
        portable_x64_log_step( r, ptr );
#endif

        switch ( *ptr )
        {
            /* Stack manipulation */

            case MN_DUP:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                portable_x64_stack_copy_cell( r->stack_ptr, &r->stack_ptr[-1] );
#else
                *r->stack_ptr = r->stack_ptr[-1] ;
#endif
                r->stack_ptr++;
                ptr++ ;
                break ;

            case MN_PUSH:
                *r->stack_ptr++ = ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_POP:
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_INDEX:
            case MN_INDEX | MN_UNSIGNED:
            case MN_INDEX | MN_STRING:
            case MN_INDEX | MN_WORD:
            case MN_INDEX | MN_WORD | MN_UNSIGNED:
            case MN_INDEX | MN_BYTE:
            case MN_INDEX | MN_BYTE | MN_UNSIGNED:
            case MN_INDEX | MN_FLOAT: /* Add float, I don't know why it was missing (SplinterGU) */
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                portable_x64_stack_adjust_ptr( &r->stack_ptr[-1], ptr[1] );
#else
                r->stack_ptr[-1] += ptr[1] ;
#endif
                ptr += 2 ;
                break ;

            case MN_ARRAY:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                portable_x64_stack_adjust_ptr( &r->stack_ptr[-2], ptr[1] * r->stack_ptr[-1] );
#else
                r->stack_ptr[-2] += ( ptr[1] * r->stack_ptr[-1] ) ;
#endif
                r->stack_ptr-- ;
                ptr += 2 ;
                break ;

            /* Process calls */

            case MN_CLONE:
                i = instance_duplicate( r ) ;
                i->codeptr = ptr + 2 ;
                ptr = r->code + ptr[1] ;
                continue ;

            case MN_CALL:
            case MN_PROC:
            {
                PROCDEF * proc = procdef_get( ptr[1] ) ;

                if ( !proc )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Unknown process\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }

                /* Process uses FRAME or locals, must create an instance */
                i = instance_new( proc, r ) ;

                assert ( i ) ;

                for ( n = 0; n < proc->params; n++ )
                    PRIDWORD( i, 4 * n ) = r->stack_ptr[-proc->params+n] ;

                r->stack_ptr -= proc->params ;

                /* I go to waiting status (by default) */
                LOCDWORD( r, STATUS ) |= STATUS_WAITING_MASK;
                i->called_by   = r;

                /* Run the process/function */
                if ( *ptr == MN_CALL )
                {
                    r->stack[0] |= STACK_RETURN_VALUE;
                    r->stack_ptr++;
                    *r->stack_ptr = instance_go( i );
                }
                else
                {
                    r->stack[0] &= ~STACK_RETURN_VALUE;
                    instance_go( i );
                }

                child_is_alive = instance_exists( i );

                ptr += 2 ;

                /* If the process is a function in a frame, save the stack and leave */
                /* If the process/function still running, then it is in a FRAME.
                   If the process/function is running code, then it his status is RUNNING */
                if ( child_is_alive &&
                        (
                            (( status = LOCDWORD( r, STATUS ) ) &  STATUS_WAITING_MASK ) ||
                            ( status & ~STATUS_WAITING_MASK ) == STATUS_FROZEN ||
                            ( status & ~STATUS_WAITING_MASK ) == STATUS_SLEEPING
                        )
                   )
                {
                    /* I go to sleep and return from this process/function */
                    i->called_by   = r;

                    /* Save the instruction pointer */
                    /* This instance don't run other code until the child return */
                    r->codeptr = ptr ;

                    /* If it don't was a CALL, then I set a flag in "len" for no return value */
                    if ( ptr[-2] == MN_CALL )
                        r->stack[0] |= STACK_RETURN_VALUE;
                    else
                        r->stack[0] &= ~STACK_RETURN_VALUE;

                    if ( debug_next && trace_sentence != -1 )
                    {
                        force_debug = 1;
                        debug_next = 0;
                    }
                    return 0;
                }

                /* Wake up! */
                LOCDWORD( r, STATUS ) &= ~STATUS_WAITING_MASK;
                if ( child_is_alive ) i->called_by = NULL;

                break ;
            }

            case MN_SYSCALL:
                p = sysproc_get( ptr[1] ) ;
                if ( !p )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Unknown system function\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }

                r->stack_ptr -= p->params ;
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                if ( p->name && p->paramtypes && p->params == 4 && strcmp( p->paramtypes, "SV++" ) == 0 && ( strcmp( p->name, "LOAD" ) == 0 || strcmp( p->name, "SAVE" ) == 0 ) )
                {
                    *r->stack_ptr = portable_x64_sysproc_load_save( strcmp( p->name, "SAVE" ) == 0, r->stack_ptr );
                    r->stack_ptr++ ;
                    ptr += 2 ;
                    break ;
                }
#endif
                *r->stack_ptr = ( *p->func )( r, r->stack_ptr ) ;
                r->stack_ptr++ ;
                ptr += 2 ;
                break ;

            case MN_SYSPROC:
                p = sysproc_get( ptr[1] ) ;
                if ( !p )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Unknown system process %x\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), ptr[1] ) ;
                    exit( 0 );
                }
                r->stack_ptr -= p->params ;
#if defined(PORTABLE_RUNTIME_DIAG) && (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                PORTABLE_DIAG_LOG( "SCRIPT", "MN_SYSPROC code=%d name=%s params=%d types=%s stack_ptr=%p p0=0x%08x p1=0x%08x p2=0x%08x", ptr[1], p->name ? p->name : "(null)", p->params, p->paramtypes ? p->paramtypes : "(null)", ( void * )r->stack_ptr, p->params > 0 ? ( unsigned int )r->stack_ptr[0] : 0u, p->params > 1 ? ( unsigned int )r->stack_ptr[1] : 0u, p->params > 2 ? ( unsigned int )r->stack_ptr[2] : 0u );
#endif
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                if ( p->name && strcmp( p->name, "GET_DESKTOP_SIZE" ) == 0 && p->params == 2 )
                {
                    portable_x64_sysproc_get_desktop_size( r, r->stack_ptr );
                    ptr += 2 ;
                    break ;
                }
                if ( p->name && p->paramtypes && p->params == 4 && strcmp( p->paramtypes, "SV++" ) == 0 && ( strcmp( p->name, "LOAD" ) == 0 || strcmp( p->name, "SAVE" ) == 0 ) )
                {
                    portable_x64_sysproc_load_save( strcmp( p->name, "SAVE" ) == 0, r->stack_ptr );
                    ptr += 2 ;
                    break ;
                }
#endif
                ( *p->func )( r, r->stack_ptr ) ;
                ptr += 2 ;
                break ;

            /* Access to variables address */

            case MN_PRIVATE:
            case MN_PRIVATE | MN_UNSIGNED:
            case MN_PRIVATE | MN_WORD:
            case MN_PRIVATE | MN_BYTE:
            case MN_PRIVATE | MN_WORD | MN_UNSIGNED:
            case MN_PRIVATE | MN_BYTE | MN_UNSIGNED:
            case MN_PRIVATE | MN_STRING:
            case MN_PRIVATE | MN_FLOAT:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                portable_x64_stack_set_ptr( r->stack_ptr++, &PRIDWORD( r, ptr[1] ) );
#else
                *r->stack_ptr++ = ( uint32_t ) & PRIDWORD( r, ptr[1] );
#endif
                ptr += 2 ;
                break ;

            case MN_PUBLIC:
            case MN_PUBLIC | MN_UNSIGNED:
            case MN_PUBLIC | MN_WORD:
            case MN_PUBLIC | MN_BYTE:
            case MN_PUBLIC | MN_WORD | MN_UNSIGNED:
            case MN_PUBLIC | MN_BYTE | MN_UNSIGNED:
            case MN_PUBLIC | MN_STRING:
            case MN_PUBLIC | MN_FLOAT:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                portable_x64_stack_set_ptr( r->stack_ptr++, &PUBDWORD( r, ptr[1] ) );
#else
                *r->stack_ptr++ = ( uint32_t ) & PUBDWORD( r, ptr[1] ) ;
#endif
                ptr += 2 ;
                break ;

            case MN_LOCAL:
            case MN_LOCAL | MN_UNSIGNED:
            case MN_LOCAL | MN_WORD:
            case MN_LOCAL | MN_BYTE:
            case MN_LOCAL | MN_WORD | MN_UNSIGNED:
            case MN_LOCAL | MN_BYTE | MN_UNSIGNED:
            case MN_LOCAL | MN_STRING:
            case MN_LOCAL | MN_FLOAT:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                portable_x64_stack_set_ptr( r->stack_ptr++, &LOCDWORD( r, ptr[1] ) );
#else
                *r->stack_ptr++ = ( uint32_t ) & LOCDWORD( r, ptr[1] ) ;
#endif
                ptr += 2 ;
                break ;

            case MN_GLOBAL:
            case MN_GLOBAL | MN_UNSIGNED:
            case MN_GLOBAL | MN_WORD:
            case MN_GLOBAL | MN_BYTE:
            case MN_GLOBAL | MN_WORD | MN_UNSIGNED:
            case MN_GLOBAL | MN_BYTE | MN_UNSIGNED:
            case MN_GLOBAL | MN_STRING:
            case MN_GLOBAL | MN_FLOAT:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                portable_x64_stack_set_ptr( r->stack_ptr++, &GLODWORD( ptr[1] ) );
#else
                *r->stack_ptr++ = ( uint32_t ) & GLODWORD( ptr[1] ) ;
#endif
                ptr += 2 ;
                break ;

            case MN_REMOTE:
            case MN_REMOTE | MN_UNSIGNED:
            case MN_REMOTE | MN_WORD:
            case MN_REMOTE | MN_BYTE:
            case MN_REMOTE | MN_WORD | MN_UNSIGNED:
            case MN_REMOTE | MN_BYTE | MN_UNSIGNED:
            case MN_REMOTE | MN_STRING:
            case MN_REMOTE | MN_FLOAT:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                    portable_x64_stack_set_ptr( &r->stack_ptr[-1], &LOCDWORD( i, ptr[1] ) );
#else
                    r->stack_ptr[-1] = ( uint32_t ) & LOCDWORD( i, ptr[1] ) ;
#endif
                ptr += 2 ;
                break ;

            case MN_REMOTE_PUBLIC:
            case MN_REMOTE_PUBLIC | MN_UNSIGNED:
            case MN_REMOTE_PUBLIC | MN_WORD:
            case MN_REMOTE_PUBLIC | MN_BYTE:
            case MN_REMOTE_PUBLIC | MN_WORD | MN_UNSIGNED:
            case MN_REMOTE_PUBLIC | MN_BYTE | MN_UNSIGNED:
            case MN_REMOTE_PUBLIC | MN_STRING:
            case MN_REMOTE_PUBLIC | MN_FLOAT:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                    portable_x64_stack_set_ptr( &r->stack_ptr[-1], &PUBDWORD( i, ptr[1] ) );
#else
                    r->stack_ptr[-1] = ( uint32_t ) & PUBDWORD( i, ptr[1] ) ;
#endif
                ptr += 2 ;
                break ;

            /* Access to variables DWORD type */

            case MN_GET_PRIV:
            case MN_GET_PRIV | MN_FLOAT:
            case MN_GET_PRIV | MN_UNSIGNED:
                *r->stack_ptr++ = PRIDWORD( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_GET_PUBLIC:
            case MN_GET_PUBLIC | MN_FLOAT:
            case MN_GET_PUBLIC | MN_UNSIGNED:
                *r->stack_ptr++ = PUBDWORD( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_GET_LOCAL:
            case MN_GET_LOCAL | MN_FLOAT:
            case MN_GET_LOCAL | MN_UNSIGNED:
                *r->stack_ptr++ = LOCDWORD( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_GET_GLOBAL:
            case MN_GET_GLOBAL | MN_FLOAT:
            case MN_GET_GLOBAL | MN_UNSIGNED:
                *r->stack_ptr++ = GLODWORD( ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_GET_REMOTE:
            case MN_GET_REMOTE | MN_FLOAT:
            case MN_GET_REMOTE | MN_UNSIGNED:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = LOCDWORD( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_GET_REMOTE_PUBLIC:
            case MN_GET_REMOTE_PUBLIC | MN_FLOAT:
            case MN_GET_REMOTE_PUBLIC | MN_UNSIGNED:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = PUBDWORD( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_PTR:
            case MN_PTR | MN_UNSIGNED:
            case MN_PTR | MN_FLOAT:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                r->stack_ptr[-1] = *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] ) ;
#else
                r->stack_ptr[-1] = *( int32_t * )r->stack_ptr[-1] ;
#endif
                ptr++ ;
                break ;

            /* Access to variables STRING type */

            case MN_PUSH | MN_STRING:
                *r->stack_ptr++ = ptr[1];
                string_use( r->stack_ptr[-1] );
                ptr += 2 ;
                break ;

            case MN_GET_PRIV | MN_STRING:
                *r->stack_ptr++ = PRIDWORD( r, ptr[1] ) ;
                string_use( r->stack_ptr[-1] );
                ptr += 2 ;
                break ;

            case MN_GET_PUBLIC | MN_STRING:
                *r->stack_ptr++ = PUBDWORD( r, ptr[1] ) ;
                string_use( r->stack_ptr[-1] );
                ptr += 2 ;
                break ;

            case MN_GET_LOCAL | MN_STRING:
                *r->stack_ptr++ = LOCDWORD( r, ptr[1] ) ;
                string_use( r->stack_ptr[-1] );
                ptr += 2 ;
                break ;

            case MN_GET_GLOBAL | MN_STRING:
                *r->stack_ptr++ = GLODWORD( ptr[1] ) ;
                string_use( r->stack_ptr[-1] );
                ptr += 2 ;
                break ;

            case MN_GET_REMOTE | MN_STRING:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = LOCDWORD( i, ptr[1] ) ;
                string_use( r->stack_ptr[-1] );
                ptr += 2 ;
                break ;

            case MN_GET_REMOTE_PUBLIC | MN_STRING:
                i = instance_get( r->stack_ptr[-1] );
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = PUBDWORD( i, ptr[1] ) ;
                string_use( r->stack_ptr[-1] );
                ptr += 2 ;
                break ;

            case MN_STRING | MN_PTR:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                r->stack_ptr[-1] = *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] ) ;
#else
                r->stack_ptr[-1] = *( int32_t * )r->stack_ptr[-1] ;
#endif
                string_use( r->stack_ptr[-1] );
                ptr++ ;
                break ;

            case MN_STRING | MN_POP:
                string_discard( r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* Access to variables WORD type */

            case MN_WORD | MN_GET_PRIV:
                *r->stack_ptr++ = PRIINT16( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_PRIV | MN_UNSIGNED:
                *r->stack_ptr++ = PRIWORD( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_PUBLIC:
                *r->stack_ptr++ = PUBINT16( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_PUBLIC | MN_UNSIGNED:
                *r->stack_ptr++ = PUBWORD( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_LOCAL:
                *r->stack_ptr++ = LOCINT16( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_LOCAL | MN_UNSIGNED:
                *r->stack_ptr++ = LOCWORD( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_GLOBAL:
                *r->stack_ptr++ = GLOINT16( ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_GLOBAL | MN_UNSIGNED:
                *r->stack_ptr++ = GLOWORD( ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_REMOTE:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = LOCINT16( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_REMOTE | MN_UNSIGNED:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = LOCWORD( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_REMOTE_PUBLIC:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = PUBINT16( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_GET_REMOTE_PUBLIC | MN_UNSIGNED:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = PUBWORD( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_PTR:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                r->stack_ptr[-1] = *( int16_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] ) ;
#else
                r->stack_ptr[-1] = *( int16_t * )r->stack_ptr[-1] ;
#endif
                ptr++ ;
                break ;

            case MN_WORD | MN_PTR | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                r->stack_ptr[-1] = *( uint16_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] ) ;
#else
                r->stack_ptr[-1] = *( uint16_t * )r->stack_ptr[-1] ;
#endif
                ptr++ ;
                break ;

            /* Access to variables BYTE type */

            case MN_BYTE | MN_GET_PRIV:
                *r->stack_ptr++ = PRIINT8( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_PRIV | MN_UNSIGNED:
                *r->stack_ptr++ = PRIBYTE( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_PUBLIC:
                *r->stack_ptr++ = PUBINT8( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_PUBLIC | MN_UNSIGNED:
                *r->stack_ptr++ = PUBBYTE( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_LOCAL:
                *r->stack_ptr++ = LOCINT8( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_LOCAL | MN_UNSIGNED:
                *r->stack_ptr++ = LOCBYTE( r, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_GLOBAL:
                *r->stack_ptr++ = GLOINT8( ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_GLOBAL | MN_UNSIGNED:
                *r->stack_ptr++ = GLOBYTE( ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_REMOTE:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = LOCINT8( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_REMOTE | MN_UNSIGNED:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = LOCBYTE( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_REMOTE_PUBLIC:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = PUBINT8( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_GET_REMOTE_PUBLIC | MN_UNSIGNED:
                i = instance_get( r->stack_ptr[-1] ) ;
                if ( !i )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Process %d not active\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), r->stack_ptr[-1] ) ;
                    exit( 0 );
                }
                else
                    r->stack_ptr[-1] = PUBBYTE( i, ptr[1] ) ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_PTR:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                r->stack_ptr[-1] = *(( int8_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] ) ) ;
#else
                r->stack_ptr[-1] = *(( int8_t * )r->stack_ptr[-1] ) ;
#endif
                ptr++ ;
                break ;

            case MN_BYTE | MN_PTR | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                r->stack_ptr[-1] = *(( uint8_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] ) ) ;
#else
                r->stack_ptr[-1] = *(( uint8_t * )r->stack_ptr[-1] ) ;
#endif
                ptr++ ;
                break ;

            /* Floating point math */

            case MN_FLOAT | MN_NEG:
                *( float * )&r->stack_ptr[-1] = -*(( float * ) & r->stack_ptr[-1] ) ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_NOT:
                *( float * )&r->stack_ptr[-1] = ( float ) !*(( float * ) & r->stack_ptr[-1] ) ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_ADD:
                *( float * )&r->stack_ptr[-2] += *(( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_SUB:
                *( float * )&r->stack_ptr[-2] -= *(( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_MUL:
                *( float * )&r->stack_ptr[-2] *= *(( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_DIV:
                *( float * )&r->stack_ptr[-2] /= *(( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_FLOAT2INT:
                *( int32_t * )&( r->stack_ptr[-ptr[1] - 1] ) = ( int32_t ) * ( float * ) & ( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_INT2FLOAT:
            case MN_INT2FLOAT | MN_UNSIGNED:
                *( float * )&( r->stack_ptr[-ptr[1] - 1] ) = ( float ) * ( int32_t * ) & ( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_INT2WORD:
            case MN_INT2WORD | MN_UNSIGNED:
                *( uint32_t * )&( r->stack_ptr[-ptr[1] - 1] ) = ( int32_t )( uint16_t ) * ( int32_t * ) & ( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2;
                break;
            case MN_INT2BYTE:
            case MN_INT2BYTE | MN_UNSIGNED:
                *( uint32_t * )&( r->stack_ptr[-ptr[1] - 1] ) = ( int32_t )( uint8_t ) * ( int32_t * ) & ( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2;
                break;

            /* Mathematical operations */

            case MN_NEG:
            case MN_NEG | MN_UNSIGNED:
                r->stack_ptr[-1] = -r->stack_ptr[-1] ;
                ptr++ ;
                break ;

            case MN_NOT:
            case MN_NOT | MN_UNSIGNED:
                r->stack_ptr[-1] = !( r->stack_ptr[-1] ) ;
                ptr++ ;
                break ;

            case MN_ADD:
                r->stack_ptr[-2] += r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_SUB:
                r->stack_ptr[-2] -= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_MUL | MN_WORD:
            case MN_MUL | MN_BYTE:
            case MN_MUL:
                r->stack_ptr[-2] *= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_MUL | MN_WORD | MN_UNSIGNED:
            case MN_MUL | MN_BYTE | MN_UNSIGNED:
            case MN_MUL | MN_UNSIGNED:
                r->stack_ptr[-2] = ( uint32_t )r->stack_ptr[-2] * ( uint32_t )r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_DIV | MN_WORD:
            case MN_DIV | MN_BYTE:
            case MN_DIV:
                if ( r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                r->stack_ptr[-2] /= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_DIV | MN_WORD | MN_UNSIGNED:
            case MN_DIV | MN_BYTE | MN_UNSIGNED:
            case MN_DIV | MN_UNSIGNED:
                if ( r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                r->stack_ptr[-2] = ( uint32_t )r->stack_ptr[-2] / ( uint32_t )r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_MOD | MN_WORD:
            case MN_MOD | MN_BYTE:
            case MN_MOD:
                if ( r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                r->stack_ptr[-2] %= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_MOD | MN_WORD | MN_UNSIGNED:
            case MN_MOD | MN_BYTE | MN_UNSIGNED:
            case MN_MOD | MN_UNSIGNED:
                if ( r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                r->stack_ptr[-2] = ( uint32_t )r->stack_ptr[-2] % ( uint32_t )r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* Bitwise operations */

            case MN_ROR:
                ( r->stack_ptr[-2] ) = (( int32_t )r->stack_ptr[-2] ) >> r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_ROR | MN_UNSIGNED:
                r->stack_ptr[-2] = (( uint32_t ) r->stack_ptr[-2] ) >> r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_ROR:
                r->stack_ptr[-2] = (( int16_t ) r->stack_ptr[-2] ) >> r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_ROR | MN_UNSIGNED:
                r->stack_ptr[-2] = (( uint16_t ) r->stack_ptr[-2] ) >> r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_ROR:
                r->stack_ptr[-2] = (( int8_t ) r->stack_ptr[-2] >> r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_ROR | MN_UNSIGNED:
                r->stack_ptr[-2] = (( uint8_t ) r->stack_ptr[-2] ) >> r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_ROL:
                ( r->stack_ptr[-2] ) = (( int32_t )r->stack_ptr[-2] ) << r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* All the next ROL operations, don't could be necessaries, but well... */

            case MN_ROL | MN_UNSIGNED:
                ( r->stack_ptr[-2] ) = ( uint32_t )( r->stack_ptr[-2] << r->stack_ptr[-1] );
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_ROL:
                ( r->stack_ptr[-2] ) = (( int16_t )r->stack_ptr[-2] ) << r->stack_ptr[-1];
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_ROL | MN_UNSIGNED:
                ( r->stack_ptr[-2] ) = ( uint16_t )( r->stack_ptr[-2] << r->stack_ptr[-1] );
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_ROL:
                ( r->stack_ptr[-2] ) = (( int8_t )r->stack_ptr[-2] ) << r->stack_ptr[-1];
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_ROL | MN_UNSIGNED:
                ( r->stack_ptr[-2] ) = ( uint8_t )( r->stack_ptr[-2] << r->stack_ptr[-1] );
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BAND:
            case MN_BAND | MN_UNSIGNED:
                r->stack_ptr[-2] &= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BOR:
            case MN_BOR | MN_UNSIGNED:
                r->stack_ptr[-2] |= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BXOR:
            case MN_BXOR | MN_UNSIGNED:
                r->stack_ptr[-2] ^= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BNOT:
            case MN_BNOT | MN_UNSIGNED:
                r->stack_ptr[-1] = ~( r->stack_ptr[-1] ) ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_BNOT:
                r->stack_ptr[-1] = ( int8_t ) ~( r->stack_ptr[-1] ) ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_BNOT | MN_UNSIGNED:
                r->stack_ptr[-1] = ( uint8_t ) ~( r->stack_ptr[-1] ) ;
                ptr++ ;
                break ;

            case MN_WORD | MN_BNOT:
                r->stack_ptr[-1] = ( int16_t ) ~( r->stack_ptr[-1] ) ;
                ptr++ ;
                break ;

            case MN_WORD | MN_BNOT | MN_UNSIGNED:
                r->stack_ptr[-1] = ( uint16_t ) ~( r->stack_ptr[-1] ) ;
                ptr++ ;
                break ;

            /* Logical operations */

            case MN_AND:
                r->stack_ptr[-2] = r->stack_ptr[-2] && r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_OR:
                r->stack_ptr[-2] = r->stack_ptr[-2] || r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_XOR:
                r->stack_ptr[-2] = ( r->stack_ptr[-2] != 0 ) ^( r->stack_ptr[-1] != 0 ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* Comparisons */

            case MN_EQ:
                r->stack_ptr[-2] = ( r->stack_ptr[-2] == r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_NE:
                r->stack_ptr[-2] = ( r->stack_ptr[-2] != r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_GTE:
                r->stack_ptr[-2] = ( r->stack_ptr[-2] >= r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_GTE | MN_UNSIGNED:
                r->stack_ptr[-2] = (( uint32_t )r->stack_ptr[-2] >= ( uint32_t )r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LTE:
                r->stack_ptr[-2] = ( r->stack_ptr[-2] <= r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LTE | MN_UNSIGNED:
                r->stack_ptr[-2] = (( uint32_t )r->stack_ptr[-2] <= ( uint32_t )r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LT:
                r->stack_ptr[-2] = ( r->stack_ptr[-2] < r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LT | MN_UNSIGNED:
                r->stack_ptr[-2] = (( uint32_t )r->stack_ptr[-2] < ( uint32_t )r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_GT:
                r->stack_ptr[-2] = ( r->stack_ptr[-2] > r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_GT | MN_UNSIGNED:
                r->stack_ptr[-2] = (( uint32_t )r->stack_ptr[-2] > ( uint32_t )r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* Floating point comparisons */

            case MN_EQ | MN_FLOAT:
                r->stack_ptr[-2] = ( *( float * ) & r->stack_ptr[-2] == *( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_NE | MN_FLOAT:
                r->stack_ptr[-2] = ( *( float * ) & r->stack_ptr[-2] != *( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_GTE | MN_FLOAT:
                r->stack_ptr[-2] = ( *( float * ) & r->stack_ptr[-2] >= *( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LTE | MN_FLOAT:
                r->stack_ptr[-2] = ( *( float * ) & r->stack_ptr[-2] <= *( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LT | MN_FLOAT:
                r->stack_ptr[-2] = ( *( float * ) & r->stack_ptr[-2] < *( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_GT | MN_FLOAT:
                r->stack_ptr[-2] = ( *( float * ) & r->stack_ptr[-2] > *( float * ) & r->stack_ptr[-1] ) ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* String comparisons */

            case MN_EQ | MN_STRING :
                n = string_comp( r->stack_ptr[-2], r->stack_ptr[-1] ) == 0 ;
                string_discard( r->stack_ptr[-2] );
                string_discard( r->stack_ptr[-1] );
                r->stack_ptr[-2] = n;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_NE | MN_STRING :
                n = string_comp( r->stack_ptr[-2], r->stack_ptr[-1] ) != 0 ;
                string_discard( r->stack_ptr[-2] );
                string_discard( r->stack_ptr[-1] );
                r->stack_ptr[-2] = n;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_GTE | MN_STRING :
                n = string_comp( r->stack_ptr[-2], r->stack_ptr[-1] ) >= 0 ;
                string_discard( r->stack_ptr[-2] );
                string_discard( r->stack_ptr[-1] );
                r->stack_ptr[-2] = n;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LTE | MN_STRING :
                n = string_comp( r->stack_ptr[-2], r->stack_ptr[-1] ) <= 0 ;
                string_discard( r->stack_ptr[-2] );
                string_discard( r->stack_ptr[-1] );
                r->stack_ptr[-2] = n;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LT | MN_STRING :
                n = string_comp( r->stack_ptr[-2], r->stack_ptr[-1] ) <  0 ;
                string_discard( r->stack_ptr[-2] );
                string_discard( r->stack_ptr[-1] );
                r->stack_ptr[-2] = n;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_GT | MN_STRING :
                n = string_comp( r->stack_ptr[-2], r->stack_ptr[-1] ) >  0 ;
                string_discard( r->stack_ptr[-2] );
                string_discard( r->stack_ptr[-1] );
                r->stack_ptr[-2] = n;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* String operations */

            case MN_VARADD | MN_STRING:
                n = *( int32_t * )( r->stack_ptr[-2] ) ;
                *( int32_t * )( r->stack_ptr[-2] ) = string_add( n, r->stack_ptr[-1] ) ;
                string_use( *( int32_t * )( r->stack_ptr[-2] ) );
                string_discard( n );
                string_discard( r->stack_ptr[-1] );
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_LETNP | MN_STRING:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                {
                    int32_t * string_slot = ( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] );
                    string_discard( *string_slot );
                    *string_slot = r->stack_ptr[-1];
                }
#else
                string_discard( *( int32_t * )( r->stack_ptr[-2] ) );
                ( *( int32_t * )( r->stack_ptr[-2] ) ) = r->stack_ptr[-1];
#endif
                r->stack_ptr -= 2 ;
                ptr++ ;
                break ;

            case MN_LET | MN_STRING:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                {
                    int32_t * string_slot = ( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] );
                    string_discard( *string_slot );
                    *string_slot = r->stack_ptr[-1];
                }
#else
                string_discard( *( int32_t * )( r->stack_ptr[-2] ) );
                ( *( int32_t * )( r->stack_ptr[-2] ) ) = r->stack_ptr[-1];
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_ADD | MN_STRING:
                n = string_add( r->stack_ptr[-2], r->stack_ptr[-1] );
                string_use( n ) ;
                string_discard( r->stack_ptr[-2] );
                string_discard( r->stack_ptr[-1] );
                r->stack_ptr-- ;
                r->stack_ptr[-1] = n ;
                ptr++ ;
                break ;

            case MN_INT2STR:
                r->stack_ptr[-ptr[1] - 1] = string_itoa( r->stack_ptr[-ptr[1] - 1] ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_INT2STR | MN_UNSIGNED:
                r->stack_ptr[-ptr[1] - 1] = string_uitoa( r->stack_ptr[-ptr[1] - 1] ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_INT2STR | MN_WORD:
                r->stack_ptr[-ptr[1] - 1] = string_itoa( r->stack_ptr[-ptr[1] - 1] ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_INT2STR | MN_UNSIGNED | MN_WORD:
                r->stack_ptr[-ptr[1] - 1] = string_uitoa( r->stack_ptr[-ptr[1] - 1] ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_INT2STR | MN_BYTE:
                r->stack_ptr[-ptr[1] - 1] = string_itoa( r->stack_ptr[-ptr[1] - 1] ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_INT2STR | MN_UNSIGNED | MN_BYTE:
                r->stack_ptr[-ptr[1] - 1] = string_uitoa( r->stack_ptr[-ptr[1] - 1] ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_FLOAT2STR:
                r->stack_ptr[-ptr[1] - 1] = string_ftoa( *( float * ) & r->stack_ptr[-ptr[1] - 1] ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_CHR2STR:
                buffer[0] = ( uint8_t )r->stack_ptr[-ptr[1] - 1] ;
                buffer[1] = 0 ;
                r->stack_ptr[-ptr[1] - 1] = string_new( buffer ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_STRI2CHR:
                n = string_char( r->stack_ptr[-2], r->stack_ptr[-1] ) ;
                string_discard( r->stack_ptr[-2] );
                r->stack_ptr-- ;
                r->stack_ptr[-1] = n ;
                ptr++ ;
                break ;

            case MN_STR2CHR:
                n = r->stack_ptr[-ptr[1] - 1] ;
                r->stack_ptr[-1] = *string_get( n ) ;
                string_discard( n );
                ptr += 2 ;
                break ;

            case MN_POINTER2STR:
                r->stack_ptr[-ptr[1] - 1] = string_ptoa( *( void ** ) & r->stack_ptr[-ptr[1] - 1] ) ;
                string_use( r->stack_ptr[-ptr[1] - 1] ) ;
                ptr += 2 ;
                break ;

            case MN_STR2FLOAT:
                n = r->stack_ptr[-ptr[1] - 1] ;
                str = ( char * )string_get( n ) ;
                *( float * )( &r->stack_ptr[-ptr[1] - 1] ) = str ? ( float )atof( str ) : 0.0f ;
                string_discard( n ) ;
                ptr += 2 ;
                break ;

            case MN_STR2INT:
                n = r->stack_ptr[-ptr[1] - 1] ;
                str = ( char * )string_get( n ) ;
                r->stack_ptr[-ptr[1] - 1] = str ? atoi( str ) : 0 ;
                string_discard( n ) ;
                ptr += 2 ;
                break ;

            /* Fixed-length strings operations*/

            case MN_A2STR:
#if defined(PORTABLE_RUNTIME_DIAG) && (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                portable_x64_log_mn_a2str( r, ptr, ptr[1] );
#endif
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                str = ( char * )portable_x64_stack_get_ptr( &r->stack_ptr[-ptr[1] - 1] );
#else
                str = *( char ** )( &r->stack_ptr[-ptr[1] - 1] ) ;
#endif
                n = string_new( str );
                string_use( n );
                r->stack_ptr[-ptr[1] - 1] = n ;
                ptr += 2 ;
                break ;

            case MN_STR2A:
                n = r->stack_ptr[-1];
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                str = ( char * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] );
                strncpy( str, string_get( n ), ptr[1] ) ;
                str[ptr[1]] = 0;
#else
                strncpy( *( char ** )( &r->stack_ptr[-2] ), string_get( n ), ptr[1] ) ;
                (( char * )( r->stack_ptr[-2] ) )[ptr[1]] = 0;
#endif
                r->stack_ptr[-2] = r->stack_ptr[-1];
                r->stack_ptr--;
                ptr += 2 ;
                break ;

            case MN_STRACAT:
                n = r->stack_ptr[-1];
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                str = ( char * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] );
                strncat( str, string_get( n ), (ptr[1]-1) - strlen( str ) ) ;
                str[ptr[1]-1] = 0;
#else
                strncat( *( char ** )( &r->stack_ptr[-2] ), string_get( n ), (ptr[1]-1) - strlen( *( char ** )( &r->stack_ptr[-2] ) ) ) ;
                (( char * )( r->stack_ptr[-2] ) )[ptr[1]-1] = 0;
#endif
                r->stack_ptr[-2] = r->stack_ptr[-1];
                r->stack_ptr--;
                ptr += 2 ;
                break ;

            /* Direct operations with variables DWORD type */

            case MN_LETNP:
            case MN_LETNP | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                ( *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
#else
                ( *( int32_t * )( r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
#endif
                r->stack_ptr -= 2 ;
                ptr++ ;
                break ;

            case MN_LET:
            case MN_LET | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                ( *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
#else
                ( *( int32_t * )( r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_INC:
            case MN_INC | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                ( *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] ) ) += ptr[1] ;
#else
                ( *( int32_t * )( r->stack_ptr[-1] ) ) += ptr[1] ;
#endif
                ptr += 2 ;
                break ;

            case MN_DEC:
            case MN_DEC | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                ( *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] ) ) -= ptr[1] ;
#else
                ( *( int32_t * )( r->stack_ptr[-1] ) ) -= ptr[1] ;
#endif
                ptr += 2 ;
                break ;

            case MN_POSTDEC:
            case MN_POSTDEC | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                {
                    int32_t * value = ( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] );
                    *value -= ptr[1] ;
                    r->stack_ptr[-1] = *value + ptr[1] ;
                }
#else
                ( *( int32_t * )( r->stack_ptr[-1] ) ) -= ptr[1] ;
                r->stack_ptr[-1] = *( int32_t * )( r->stack_ptr[-1] ) + ptr[1] ;
#endif
                ptr += 2 ;
                break ;

            case MN_POSTINC:
            case MN_POSTINC | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                {
                    int32_t * value = ( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-1] );
                    *value += ptr[1] ;
                    r->stack_ptr[-1] = *value - ptr[1] ;
                }
#else
                *(( int32_t * )( r->stack_ptr[-1] ) ) += ptr[1] ;
                r->stack_ptr[-1] = *( int32_t * )( r->stack_ptr[-1] ) - ptr[1] ;
#endif
                ptr += 2 ;
                break ;

            case MN_VARADD:
            case MN_VARADD | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) += r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) += r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARSUB:
            case MN_VARSUB | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) -= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) -= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARMUL:
            case MN_VARMUL | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) *= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) *= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARDIV:
            case MN_VARDIV | MN_UNSIGNED:
                if ( r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) /= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) /= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARMOD:
            case MN_VARMOD | MN_UNSIGNED:
                if ( r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) %= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) %= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VAROR:
            case MN_VAROR | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) |= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) |= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARXOR:
            case MN_VARXOR | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) ^= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) ^= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARAND:
            case MN_VARAND | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) &= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) &= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARROR:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) >>= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) >>= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARROR | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( uint32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) >>= r->stack_ptr[-1] ;
#else
                *( uint32_t * )( r->stack_ptr[-2] ) >>= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARROL:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( int32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) <<= r->stack_ptr[-1] ;
#else
                *( int32_t * )( r->stack_ptr[-2] ) <<= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_VARROL | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( uint32_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) <<= r->stack_ptr[-1] ;
#else
                *( uint32_t * )( r->stack_ptr[-2] ) <<= r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;
            /* Direct operations with variables WORD type */

            case MN_WORD | MN_LETNP:
            case MN_WORD | MN_LETNP | MN_UNSIGNED:
                ( *( int16_t * )( r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
                r->stack_ptr -= 2 ;
                ptr++ ;
                break ;

            case MN_WORD | MN_LET:
            case MN_WORD | MN_LET | MN_UNSIGNED:
                ( *( int16_t * )( r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_INC:
            case MN_WORD | MN_INC | MN_UNSIGNED:
                ( *( int16_t * )( r->stack_ptr[-1] ) ) += ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_DEC:
            case MN_WORD | MN_DEC | MN_UNSIGNED:
                ( *( int16_t * )( r->stack_ptr[-1] ) ) -= ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_POSTDEC:
            case MN_WORD | MN_POSTDEC | MN_UNSIGNED:
                ( *( int16_t * )( r->stack_ptr[-1] ) ) -= ptr[1] ;
                r->stack_ptr[-1] = *( int16_t * )( r->stack_ptr[-1] ) + ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_POSTINC:
            case MN_WORD | MN_POSTINC | MN_UNSIGNED:
                *(( int16_t * )( r->stack_ptr[-1] ) ) += ptr[1] ;
                r->stack_ptr[-1] = *( int16_t * )( r->stack_ptr[-1] ) - ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_WORD | MN_VARADD:
            case MN_WORD | MN_VARADD | MN_UNSIGNED:
                *( int16_t * )( r->stack_ptr[-2] ) += r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARSUB:
            case MN_WORD | MN_VARSUB | MN_UNSIGNED:
                *( int16_t * )( r->stack_ptr[-2] ) -= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARMUL:
            case MN_WORD | MN_VARMUL | MN_UNSIGNED:
                *( int16_t * )( r->stack_ptr[-2] ) *= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARDIV:
            case MN_WORD | MN_VARDIV | MN_UNSIGNED:
                if (( int16_t )r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                *( int16_t * )( r->stack_ptr[-2] ) /= ( int16_t )r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARMOD:
            case MN_WORD | MN_VARMOD | MN_UNSIGNED:
                if (( int16_t )r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                *( int16_t * )( r->stack_ptr[-2] ) %= ( int16_t )r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VAROR:
            case MN_WORD | MN_VAROR | MN_UNSIGNED:
                *( int16_t * )( r->stack_ptr[-2] ) |= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARXOR:
            case MN_WORD | MN_VARXOR | MN_UNSIGNED:
                *( int16_t * )( r->stack_ptr[-2] ) ^= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARAND:
            case MN_WORD | MN_VARAND | MN_UNSIGNED:
                *( int16_t * )( r->stack_ptr[-2] ) &= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARROR:
                *( int16_t * )( r->stack_ptr[-2] ) >>= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARROR | MN_UNSIGNED:
                *( uint16_t * )( r->stack_ptr[-2] ) >>= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARROL:
                *( int16_t * )( r->stack_ptr[-2] ) <<= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_WORD | MN_VARROL | MN_UNSIGNED:
                *( uint16_t * )( r->stack_ptr[-2] ) <<= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* Direct operations with variables BYTE type */

            case MN_BYTE | MN_LETNP:
            case MN_BYTE | MN_LETNP | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                ( *( uint8_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
#else
                ( *( uint8_t * )( r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
#endif
                r->stack_ptr -= 2 ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_LET:
            case MN_BYTE | MN_LET | MN_UNSIGNED:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                ( *( uint8_t * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
#else
                ( *( uint8_t * )( r->stack_ptr[-2] ) ) = r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_INC:
            case MN_BYTE | MN_INC | MN_UNSIGNED:
                ( *( uint8_t * )( r->stack_ptr[-1] ) ) += ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_DEC:
            case MN_BYTE | MN_DEC | MN_UNSIGNED:
                ( *( uint8_t * )( r->stack_ptr[-1] ) ) -= ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_POSTDEC:
            case MN_BYTE | MN_POSTDEC | MN_UNSIGNED:
                ( *( uint8_t * )( r->stack_ptr[-1] ) ) -= ptr[1] ;
                r->stack_ptr[-1] = *( uint8_t * )( r->stack_ptr[-1] ) + ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_POSTINC:
            case MN_BYTE | MN_POSTINC | MN_UNSIGNED:
                *(( uint8_t * )( r->stack_ptr[-1] ) ) += ptr[1] ;
                r->stack_ptr[-1] = *( uint8_t * )( r->stack_ptr[-1] ) - ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_BYTE | MN_VARADD:
            case MN_BYTE | MN_VARADD | MN_UNSIGNED:
                *( uint8_t * )( r->stack_ptr[-2] ) += r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARSUB:
            case MN_BYTE | MN_VARSUB | MN_UNSIGNED:
                *( uint8_t * )( r->stack_ptr[-2] ) -= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARMUL:
            case MN_BYTE | MN_VARMUL | MN_UNSIGNED:
                *( uint8_t * )( r->stack_ptr[-2] ) *= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARDIV:
            case MN_BYTE | MN_VARDIV | MN_UNSIGNED:
                if (( uint8_t )r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                *( uint8_t * )( r->stack_ptr[-2] ) /= ( uint8_t )r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARMOD:
            case MN_BYTE | MN_VARMOD | MN_UNSIGNED:
                if (( uint8_t )r->stack_ptr[-1] == 0 )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Division by zero\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                *( uint8_t * )( r->stack_ptr[-2] ) %= ( uint8_t )r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VAROR:
            case MN_BYTE | MN_VAROR | MN_UNSIGNED:
                *( uint8_t * )( r->stack_ptr[-2] ) |= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARXOR:
            case MN_BYTE | MN_VARXOR | MN_UNSIGNED:
                *( uint8_t * )( r->stack_ptr[-2] ) ^= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARAND:
            case MN_BYTE | MN_VARAND | MN_UNSIGNED:
                *( uint8_t * )( r->stack_ptr[-2] ) &= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARROR:
                *( int8_t * )( r->stack_ptr[-2] ) >>= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARROR | MN_UNSIGNED:
                *( uint8_t * )( r->stack_ptr[-2] ) >>= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARROL:
                *( int8_t * )( r->stack_ptr[-2] ) <<= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_BYTE | MN_VARROL | MN_UNSIGNED:
                *( uint8_t * )( r->stack_ptr[-2] ) <<= r->stack_ptr[-1] ;
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* Direct operations with variables FLOAT type */

            case MN_FLOAT | MN_LETNP:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                ( *( float * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) ) = *( float * ) & r->stack_ptr[-1] ;
#else
                ( *( float * )( r->stack_ptr[-2] ) ) = *( float * ) & r->stack_ptr[-1] ;
#endif
                r->stack_ptr -= 2 ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_LET :
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                ( *( float * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) ) = *( float * ) & r->stack_ptr[-1] ;
#else
                ( *( float * )( r->stack_ptr[-2] ) ) = *( float * ) & r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_INC:
                ( *( float * )( r->stack_ptr[-1] ) ) += ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_FLOAT | MN_DEC:
                ( *( float * )( r->stack_ptr[-1] ) ) -= ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_FLOAT | MN_POSTDEC:
                ( *( float * )( r->stack_ptr[-1] ) ) -= ptr[1] ;
                r->stack_ptr[-1] = *( uint32_t * )( r->stack_ptr[-1] ) + ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_FLOAT | MN_POSTINC:
                *(( float * )( r->stack_ptr[-1] ) ) += ptr[1] ;
                r->stack_ptr[-1] = *( uint32_t * )( r->stack_ptr[-1] ) - ptr[1] ;
                ptr += 2 ;
                break ;

            case MN_FLOAT | MN_VARADD:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( float * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) += *( float * ) & r->stack_ptr[-1] ;
#else
                *( float * )( r->stack_ptr[-2] ) += *( float * ) & r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_VARSUB:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( float * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) -= *( float * ) & r->stack_ptr[-1] ;
#else
                *( float * )( r->stack_ptr[-2] ) -= *( float * ) & r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_VARMUL:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( float * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) *= *( float * ) & r->stack_ptr[-1] ;
#else
                *( float * )( r->stack_ptr[-2] ) *= *( float * ) & r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            case MN_FLOAT | MN_VARDIV:
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
                *( float * )portable_x64_stack_get_ptr( &r->stack_ptr[-2] ) /= *( float * ) & r->stack_ptr[-1] ;
#else
                *( float * )( r->stack_ptr[-2] ) /= *( float * ) & r->stack_ptr[-1] ;
#endif
                r->stack_ptr-- ;
                ptr++ ;
                break ;

            /* Jumps */

            case MN_JUMP:
                ptr = r->code + ptr[1] ;
                continue ;

            case MN_JTRUE:
                r->stack_ptr-- ;
                if ( *r->stack_ptr )
                {
                    ptr = r->code + ptr[1] ;
                    continue ;
                }
                ptr += 2 ;
                break ;

            case MN_JFALSE:
                r->stack_ptr-- ;
                if ( !*r->stack_ptr )
                {
                    ptr = r->code + ptr[1] ;
                    continue ;
                }
                ptr += 2 ;
                break ;

            case MN_JTTRUE:
                if ( r->stack_ptr[-1] )
                {
                    ptr = r->code + ptr[1] ;
                    continue ;
                }
                ptr += 2 ;
                break ;

            case MN_JTFALSE:
                if ( !r->stack_ptr[-1] )
                {
                    ptr = r->code + ptr[1] ;
                    continue ;
                }
                ptr += 2 ;
                break ;

            case MN_NCALL:
                *r->stack_ptr++ = ptr - r->code + 2 ; /* Push next address */
                ptr = r->code + ptr[1] ; /* Call function */
                r->call_level++;
                break ;

            /* Switch */

            case MN_SWITCH:
                r->switchval = *--r->stack_ptr ;
                r->cased = 0 ;
                ptr++ ;
                break ;

            case MN_SWITCH | MN_STRING:
                if ( r->switchval_string != 0 ) string_discard( r->switchval_string );
                r->switchval_string = *--r->stack_ptr;
                r->cased = 0;
                ptr++;
                break;

            case MN_CASE:
                if ( r->switchval == *--r->stack_ptr ) r->cased = 2 ;
                ptr++ ;
                break ;

            case MN_CASE | MN_STRING:
                if ( string_comp( r->switchval_string, *--r->stack_ptr ) == 0 ) r->cased = 2 ;
                string_discard( *r->stack_ptr );
                string_discard( r->stack_ptr[-1] );
                ptr++;
                break;

            case MN_CASE_R:
                r->stack_ptr -= 2 ;
                if ( r->switchval >= r->stack_ptr[0] && r->switchval <= r->stack_ptr[1] ) r->cased = 1 ;
                ptr++ ;
                break ;

            case MN_CASE_R | MN_STRING:
                r->stack_ptr -= 2;
                if ( string_comp( r->switchval_string, r->stack_ptr[0] ) >= 0 &&
                     string_comp( r->switchval_string, r->stack_ptr[1] ) <= 0 )
                    r->cased = 1;
                string_discard( r->stack_ptr[0] );
                string_discard( r->stack_ptr[1] );
                ptr++;
                break;

            case MN_JNOCASE:
                if ( r->cased < 1 )
                {
                    ptr = r->code + ptr[1] ;
                    continue ;
                }
                ptr += 2 ;
                break ;

            /* Process control */

            case MN_TYPE:
            {
                PROCDEF * proct = procdef_get( ptr[1] ) ;
                if ( !proct )
                {
                    fprintf( stderr, "ERROR: Runtime error in %s(%d) - Invalid type\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    exit( 0 );
                }
                *r->stack_ptr++ = proct->type ;
                ptr += 2 ;
                break ;
            }

            case MN_FRAME:
            {
                int frame_inc = r->stack_ptr[-1];
                LOCINT32( r, FRAME_PERCENT ) += frame_inc;
#ifdef PORTABLE_RUNTIME_DIAG
                if ( PORTABLE_DIAG_ENV_ON( "SORR_PORTABLE_DIAG_TIMING" ) )
                {
                    static Uint32 timing_last_tick = 0;
                    static int timing_count = 0;
                    static int timing_sum = 0;
                    static int timing_min = 0;
                    static int timing_max = 0;
                    Uint32 now = SDL_GetTicks();
                    if ( !timing_last_tick ) timing_last_tick = now;
                    if ( !timing_count || frame_inc < timing_min ) timing_min = frame_inc;
                    if ( !timing_count || frame_inc > timing_max ) timing_max = frame_inc;
                    timing_count++;
                    timing_sum += frame_inc;
                    if ( now - timing_last_tick >= 1000 )
                    {
                        float avg = timing_count ? ( ( float )timing_sum / ( float )timing_count ) : 0.0f;
                        PORTABLE_DIAG_LOG( "TIMING", "mn_frame_summary elapsed_ms=%u count=%d avg_inc=%.2f min_inc=%d max_inc=%d last_proc=%s last_id=%d last_frame_percent=%d",
                                           ( unsigned int )( now - timing_last_tick ), timing_count, avg, timing_min, timing_max,
                                           r->proc ? r->proc->name : "(null)", LOCDWORD( r, PROCESS_ID ), LOCINT32( r, FRAME_PERCENT ) );
                        timing_last_tick = now;
                        timing_count = 0;
                        timing_sum = 0;
                        timing_min = 0;
                        timing_max = 0;
                    }
                }
#endif
                if ( frame_log_count < 100 )
                {
                    PORTABLE_DIAG_LOG( "SCRIPT", "MN_FRAME proc=%s id=%d frame_percent=%d", r->proc ? r->proc->name : "(null)", LOCDWORD( r, PROCESS_ID ), LOCINT32( r, FRAME_PERCENT ) );
                    frame_log_count++;
                }
                r->stack_ptr-- ;
                r->codeptr = ptr + 1 ;
                return_value = LOCDWORD( r, PROCESS_ID );

                if ( !( r->proc->flags & PROC_FUNCTION ) &&
                        r->called_by && instance_exists( r->called_by ) && ( LOCDWORD( r->called_by, STATUS ) & STATUS_WAITING_MASK ) )
                {
                    /* We're returning and the parent is waiting: wake it up */
                    if ( r->called_by->stack && ( r->called_by->stack[0] & STACK_RETURN_VALUE ) )
                        r->called_by->stack_ptr[-1] = return_value;

                    LOCDWORD( r->called_by, STATUS ) &= ~STATUS_WAITING_MASK;
                    r->called_by = NULL;
                }
                goto break_all ;
            }

            case MN_END:
                if ( r->call_level > 0 )
                {
                    ptr = r->code + *--r->stack_ptr ;
                    r->call_level--;
                    continue;
                }

                if ( LOCDWORD( r, STATUS ) != STATUS_DEAD ) LOCDWORD( r, STATUS ) = STATUS_KILLED ;
                goto break_all ;

            case MN_RETURN:
                if ( r->call_level > 0 )
                {
                    ptr = r->code + *--r->stack_ptr ;
                    r->call_level--;
                    continue;
                }

                if ( LOCDWORD( r, STATUS ) != STATUS_DEAD ) LOCDWORD( r, STATUS ) = STATUS_KILLED ;
                r->stack_ptr-- ;
                return_value = *r->stack_ptr ;
                goto break_all ;

            /* Handlers */

            case MN_EXITHNDLR:
                r->exitcode = ptr[1] ;
                ptr += 2 ;
                break;

            case MN_ERRHNDLR:
                r->errorcode = ptr[1] ;
                ptr += 2 ;
                break;

            /* Others */

            case MN_DEBUG:
                if ( dcb.data.NSourceFiles )
                {
                    if ( debug > 0 ) printf( "\n::: DEBUG from %s(%d)\n", r->proc->name, LOCDWORD( r, PROCESS_ID ) ) ;
                    debug_next = 1;
                }
                ptr++;
                break ;

            case MN_SENTENCE:
                trace_sentence     = ptr[1];
                trace_instance     = r;
                ptr += 2 ;
                break ;

            default:
                fprintf( stderr, "ERROR: Runtime error in %s(%d) - Mnemonic 0x%02X not implemented\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), *ptr ) ;
                exit( 0 );
        }

        if ( r->stack_ptr < r->stack )
        {
            fprintf( stderr, "ERROR: Runtime error in %s(%d) - Critical Stack Problem StackBase=%p StackPTR=%p\n", r->proc->name, LOCDWORD( r, PROCESS_ID ), (void *)r->stack, (void *)r->stack_ptr ) ;
            exit( 0 );
        }

#ifdef EXIT_ON_EMPTY_STACK
        if ( r->stack_ptr == r->stack )
        {
            r->codeptr = ptr ;
            if ( LOCDWORD( r, STATUS ) != STATUS_RUNNING && LOCDWORD( r, STATUS ) != STATUS_DEAD ) break ;
        }
#endif

    }

    /* *** GENERAL EXIT *** */
break_all:

    if ( !*ptr || *ptr == MN_RETURN || *ptr == MN_END || LOCDWORD( r, STATUS ) == STATUS_KILLED )
    {
        /* Check for waiting parent */
        if ( r->called_by && instance_exists( r->called_by ) && ( LOCDWORD( r->called_by, STATUS ) & STATUS_WAITING_MASK ) )
        {
            /* We're returning and the parent is waiting: wake it up */
            if ( r->called_by->stack && ( r->called_by->stack[0] & STACK_RETURN_VALUE ) )
                r->called_by->stack_ptr[-1] = return_value;

            LOCDWORD( r->called_by, STATUS ) &= ~STATUS_WAITING_MASK;
        }
        r->called_by = NULL;

        /* The process should be destroyed immediately, it is a function-type one */
        /* Run ONEXIT */
        if (( LOCDWORD( r, STATUS ) & ~STATUS_WAITING_MASK ) != STATUS_DEAD && r->exitcode )
        {
            LOCDWORD( r, STATUS ) = STATUS_DEAD;
            r->codeptr = r->code + r->exitcode;
            instance_go( r );
            if ( !instance_exists( r ) ) r = NULL;
        }
        else
        {
            instance_destroy( r );
            r = NULL ;
        }
    }

    /* Hook */
    if ( r && instance_pos_execute_hook_count )
    {
        for ( n = 0; n < instance_pos_execute_hook_count; n++ )
            instance_pos_execute_hook_list[n]( r );
    }
    /* Hook */
    if ( r && LOCDWORD( r, STATUS ) != STATUS_KILLED && r->first_run ) r->first_run = 0;

    if ( debug_next && trace_sentence != -1 )
    {
        force_debug = 1;
        debug_next = 0;
    }

    return return_value;
}

/* ---------------------------------------------------------------------- */
