#include <string.h>
#include <stdio.h>

#ifndef COLORNAME_H
#define COLORNAME_H
enum colornameFormat {PAM_COLORNAME_ENGLISH = 0,
                      PAM_COLORNAME_HEXOK   = 1};

struct colorfile_entry {
    long r, g, b;
    char * colorname;
};



void 
pm_canonstr(char * const str);

FILE *
pm_openColornameFile(const int must_open);

struct colorfile_entry
pm_colorget(FILE * const f);

long
pm_rgbnorm(const long rgb, const long lmaxval, const int n, 
           const char * const colorname);

#endif
