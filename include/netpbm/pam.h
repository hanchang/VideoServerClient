/*----------------------------------------------------------------------------
   These are declarations for use with the Portable Arbitrary Map (PAM)
   format and the Netpbm library functions specific to them.
-----------------------------------------------------------------------------*/

#ifndef PAM_H
#define PAM_H

#include "pnm.h"

typedef unsigned long sample;

struct pam {
/* This structure describes an open PAM image file.  It consists
   entirely of information that belongs in the header of a PAM image
   and filesystem information.  It does not contain any state
   information about the processing of that image.  

   This is not considered to be an opaque object.  The user of Netbpm
   libraries is free to access and set any of these fields whenever
   appropriate.  The structure exists to make coding of function calls
   easy.
*/

    /* 'size' and 'len' are necessary in order to provide forward and
       backward compatibility between library functions and calling programs
       as this structure grows.
       */
    unsigned int size;   
        /* The storage size of this entire structure, in bytes */
    unsigned int len;    
        /* The length, in bytes, of the information in this structure.
           The information starts in the first byte and is contiguous.  
           This cannot be greater than 'size'
           */
    FILE *file;
    int format;
        /* The format code of the raw image.  This is PAM_FORMAT
           unless the PAM image is really a view of a PBM, PGM, or PPM
           image.  Then it's PBM_FORMAT, RPBM_FORMAT, etc.
           */
    unsigned int plainformat;
        /* Logical:  the format above is a plain (text) format as opposed
           to a raw (binary) format.  This is entirely redundant with the
           'format' member and exists as a separate member only for 
           computational speed.
        */
    int height;  /* Height of image in rows */
    int width;   
        /* Width of image in number of columns (tuples per row) */
    unsigned int depth;   
        /* Depth of image (number of samples in each tuple). */
    sample maxval;  /* Maximum defined value for a sample */
    unsigned int bytes_per_sample;  
        /* Number of bytes used to represent each sample in the image file.
           Note that this is strictly a function of 'maxval'.  It is in a
           a separate member for computational speed.
           */
    char tuple_type[256];
        /* The tuple type string from the image header.  Netpbm does 
           not define any values for this except the following, which are
           used for a PAM image which is really a view of a PBM, PGM,
           or PPM image:  PAM_PBM_TUPLETYPE, PAM_PGM_TUPLETYPE, 
           PAM_PPM_TUPLETYPE.
           */
};

#define PAM_PBM_TUPLETYPE "BLACKANDWHITE"
#define PAM_PGM_TUPLETYPE "GRAYSCALE"
#define PAM_PPM_TUPLETYPE "RGB"

#define PAM_PBM_BLACK 0
#define PAM_PBM_WHITE 1
    /* These are values of samples in a PAM image that represents a black
       and white bitmap image.  They are the values of black and white,
       respectively.  For example, if you use pnm_readpamrow() to read a
       row from a PBM file, the black pixels get returned as 
       PAM_PBM_BLACK.
    */

#define PAM_RED_PLANE 0
#define PAM_GRN_PLANE 1
#define PAM_BLU_PLANE 2
    /* These are plane numbers for the 3 planes of a PAM image that
       represents an RGB image (tuple type is PAM_PPM_TUPLETYPE).  So
       if 'pixel' is a tuple returned by pnmreadpamrow(), then
       pixel[PAM_GRN_PLANE] is the value of the green sample in that
       pixel.
       */
#define PAM_TRN_PLANE 3
    /* Some PAMs with "RGB" or "RGBA" tuple types have this 4th plane
       for transparency.  0 = transparent, maxval = opaque.
    */

typedef sample *tuple;  
    /* A tuple in a PAM.  This is an array such that tuple[i-1] is the
       ith sample (element) in the tuple.  It's dimension is the depth
       of the image (see pam.depth above).
       */

#define PAM_OVERALL_MAXVAL ULONG_MAX

/* Note: xv uses the same "P7" signature for its thumbnail images (it
   started using it years before PAM and unbeknownst to the designer
   of PAM).  But these images are still easily distinguishable from
   PAMs 
*/
#define PAM_MAGIC1 'P'
#define PAM_MAGIC2 '7'
#define PAM_FORMAT (PAM_MAGIC1 * 256 + PAM_MAGIC2)
#define PAM_TYPE PAM_FORMAT

/* Macro for turning a format number into a type number. */

#define PAM_FORMAT_TYPE(f) ((f) == PAM_FORMAT ? PAM_TYPE : PPM_FORMAT_TYPE(f))

/* Declarations of library functions. */

/* We don't have a specific PAM function for init and nextimage, because
   one can simply use pnm_init() and pnm_nextimage() from pnm.h.
*/

char
stripeq(const char * const comparand, const char * const comparator);

int
pnm_tupleequal(const struct pam * const pamP, 
               tuple              const comparand, 
               tuple              const comparator);

void
pnm_assigntuple(const struct pam * const pamP,
                tuple              const dest,
                tuple              const source);

static __inline__ sample
pnm_scalesample(sample const source, 
                sample const oldmaxval, 
                sample const newmaxval) {

    if (oldmaxval == newmaxval)
        /* Fast path for common case */
        return source;
    else 
        return (source * newmaxval + (oldmaxval/2)) / oldmaxval;
}



void
pnm_scaletuple(const struct pam * const pamP,
               tuple              const dest,
               tuple              const source, 
               sample             const newmaxval);

void 
pnm_scaletuplerow(const struct pam * const pamP,
                  tuple *            const destRow,
                  tuple *            const sourceRow,
                  sample             const newMaxval);

void
createBlackTuple(const struct pam * const pamP, tuple * const blackTupleP);

#define pnm_allocpamtuple(pamP) \
    ((tuple) pm_allocrow(1, (pamP)->depth*sizeof(sample)))

#define pnm_freepamtuple(tuple) pm_freerow((char*) tuple)

tuple *
pnm_allocpamrow(struct pam * const pamP);

#define pnm_freepamrow(tuplerow) pm_freerow((char*) tuplerow)

tuple **
pnm_allocpamarray(struct pam * const pamP);

void
pnm_freepamarray(tuple ** const tuplearray, struct pam * const pamP);

void
pnm_setpamrow(struct pam const pam, 
              tuple *    const tuplerow, 
              sample     const value);

void 
pnm_readpaminit(FILE *file, struct pam * const pamP, const int size);

void 
pnm_readpamrow(struct pam * const pamP, tuple* const tuplerow);

tuple** 
pnm_readpam(FILE *file, struct pam * const pamP, const int size);

void 
pnm_writepaminit(struct pam * const pamP);

void 
pnm_writepamrow(struct pam * const pamP, const tuple * const tuplerow);

void 
pnm_writepam(struct pam * const pamP, tuple ** const tuplearray);

void
pnm_checkpam(struct pam * const pamP, const enum pm_check_type check_type, 
             enum pm_check_code * const retvalP);

extern double 
pnm_lumin_factor[3];

void
pnm_YCbCrtuple(const tuple tuple, 
               double * const YP, double * const CbP, double * const CrP);


#endif
