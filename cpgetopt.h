
#ifndef _CPGETOPT_H
#define _CPGETOPT_H
#include "cpdltype.h"

#ifdef _NWCP_WIN32

#ifdef __cplusplus
extern "C" {
#endif


/* if error message should be printed
   Callers store zero here to inhibit the error message `getopt' prints
   for unrecognized options.  */
extern CPDLAPI int opterr;

/* index into parent argv vector
   Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */
extern CPDLAPI int optind;

/* character checked for validity
   Set to an option character which was unrecognized.  */
extern CPDLAPI int optopt;

/* reset getopt */
extern CPDLAPI int optreset;

/* argument associated with option
   For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */
extern CPDLAPI char* optarg;

struct option
{
    const char* name;
    int has_arg;
    int* flag;
    int val;
};

/* Names for the values of the `has_arg' field of `struct option'.  */
#define no_argument       0
#define required_argument 1
#define optional_argument 2

extern CPDLAPI int getopt(int argc, char* const* argv, const char* shortopts);
extern CPDLAPI int getopt_long(int nargc, char* const* nargv, const char* options,
                               const struct option* long_options, int* index);

#ifdef __cplusplus
}
#endif

#endif

#endif  /* not _CPGETOPT_H */
