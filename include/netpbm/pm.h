/* pm.h - interface to format-independent part of libpbm.
**
** Copyright (C) 1988, 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#ifndef _PM_H_
#define _PM_H_

#include "pm_config.h"

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef VMS
#include <perror.h>
#endif

#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#undef ABS
#define ABS(a) ((a) >= 0 ? (a) : -(a))
#undef SGN
#define SGN(a)		(((a)<0) ? -1 : 1)
#undef ODD
#define ODD(n) ((n) & 1)
#undef ROUND
#define ROUND(X) (((X) >= 0) ? (int)((X)+0.5) : (int)((X)-0.5))
#undef ROUNDU
#define ROUNDU(X) ((unsigned int)((X)+0.5))


/* Definitions to make Netpbm programs work with either ANSI C or C
   Classic.

   This is obsolete, as all compilers recognize the ANSI syntax now.

   We are slowly removing all the ARGS invocations from the programs
   (and replacing them with explicit ANSI syntax), but we have a lot
   of programs where we have removed ARGS from the definition but not
   the prototype, and we have discovered that the Sun compiler
   considers the resulting mismatch between definition and prototype
   to be an error.  So we make ARGS create the ANSI syntax
   unconditionally to avoid having to fix all those mismatches.  */

#if 0
#if __STDC__
#define ARGS(alist) alist
#else /*__STDC__*/
#define ARGS(alist) ()
#define const
#endif /*__STDC__*/
#endif
#define ARGS(alist) alist


void 
pm_init(const char * const progname, unsigned int const flags);

void 
pm_proginit(int* const argcP, char* argv[]);

void
pm_setMessage(int const newState, int * const oldStateP);

void
pm_nextimage(FILE * const file, int * const eofP);

/* Variable-sized arrays definitions. */

char** 
pm_allocarray (int const cols, int const rows, int const size );

char* 
pm_allocrow (int const cols, int const size);

void 
pm_freearray (char** const its, int const rows);

void 
pm_freerow(char* const itrow);


/* Obsolete -- use shhopt instead */
int 
pm_keymatch (char* const str, const char* const keyword, int const minchars);


int 
pm_maxvaltobits (int const maxval);

int 
pm_bitstomaxval (int const bits);

unsigned int 
pm_lcm (const unsigned int x, 
        const unsigned int y,
        const unsigned int z,
        const unsigned int limit);

/* GNU_PRINTF_ATTR lets the GNU compiler check pm_message() and pm_error()
   calls to be sure the arguments match the format string, thus preventing
   runtime segmentation faults and incorrect messages.
*/
#ifdef __GNUC__
#define GNU_PRINTF_ATTR __attribute__ ((format (printf, 1,2)))
#else
#define GNU_PRINTF_ATTR
#endif

void GNU_PRINTF_ATTR
pm_message (const char format[], ...);     

void GNU_PRINTF_ATTR
pm_error (const char reason[], ...);       

/* Obsolete - use helpful error message instead */
void
pm_perror (const char reason[]);           

/* Obsolete - use shhopt and man page instead */
void 
pm_usage (const char usage[]);             

FILE* 
pm_openr (const char* const name);
         
FILE*    
pm_openw (const char* const name);
         
FILE *
pm_openr_seekable(const char name[]);

void     
pm_close (FILE* const f);

void 
pm_closer (FILE* const f);
          
void      
pm_closew (FILE* const f);



int
pm_readbigshort ( FILE* const in, short* const sP );

int
pm_writebigshort ( FILE* const out, short const s );

int
pm_readbiglong ( FILE* const in, long* const lP );

int
pm_writebiglong ( FILE* const out, long const l );

int
pm_readlittleshort ( FILE* const in, short* const sP );

int
pm_writelittleshort ( FILE* const out, short const s );

int
pm_readlittlelong ( FILE* const in, long* const lP );

int
pm_writelittlelong ( FILE* const out, long const l );

int 
pm_readmagicnumber(FILE * const ifP);

char* 
pm_read_unknown_size(FILE* const file, long* const buf);

unsigned int
pm_tell(FILE * const fileP);

void
pm_seek(FILE * const fileP, unsigned long filepos);

enum pm_check_code {
    PM_CHECK_OK,
    PM_CHECK_UNKNOWN_TYPE,
    PM_CHECK_TOO_LONG,
    PM_CHECK_UNCHECKABLE,
    PM_CHECK_TOO_SHORT
};

enum pm_check_type {
    PM_CHECK_BASIC
};

void
pm_check(FILE * const file, const enum pm_check_type check_type, 
         const unsigned int need_raster_size,
         enum pm_check_code * const retval_p);

char *
pm_arg0toprogname(const char arg0[]);

#endif

