#ifndef _NSTRING_H
#define _NSTRING_H

#include <stdarg.h>

/* These are all private versions of commonly available standard C
   library subroutines whose names are the same except with the N at
   the end.  Because not all standard C libraries have them all,
   Netpbm must include them in its own libraries, and because some
   standard C libraries have some of them, Neptbm must use different
   names for them.
   
   The GNU C library has all of them.  All but the oldest standard C libraries
   have snprintf().

   There is one difference between the asprintf() family and that found in 
   other libraries:  The return value is a const char * instead of a char *.
   The const is more correct.  

   strfree() is strictly a Netpbm invention, to allow proper type checking
   when freeing storage allocated by the Neptbm asprintf().
*/

int snprintfN(char *, size_t, const char *, /*args*/ ...);
int vsnprintfN(char *, size_t, const char *, va_list);
int asprintfN(const char **ptr, const char *fmt, /*args*/ ...);
int vasprintfN(const char **ptr, const char *fmt, va_list ap);
int asnprintfN(const char **ptr, size_t str_m, const char *fmt, ...);
int vasnprintfN(const char **ptr, size_t str_m, const char *fmt, va_list ap);
void strfree(const char * const string);
#endif

