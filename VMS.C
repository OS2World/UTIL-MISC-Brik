/*
**   A wildcard expansion function for VAX/VMS
**   -- Rahul Dhesi
*/
/* ::[[ @(#) vms.c 1.5 89/03/10 19:09:28 ]]:: */
#ifndef LINT
static char sccsid[]="::[[ @(#) vms.c 1.5 89/03/10 19:09:28 ]]::";
#endif

/*
Checksum: 3221488897     (verify or update with "brik")
*/

#include <descrip.h>

#define  FMAX  3        /* Number of different filename patterns */
#define  PATHSIZE 1024  /* buffer area to store pathname */

char *nextfile (what, filespec, fileset)
int what;                        /* whether to initialize or match      */
register char *filespec;         /* filespec to match if initializing   */
register int fileset;            /* which set of files                  */
{
   int status;
   char *p;                      /* temp ptr */
   struct dsc$descriptor_s d_fwild, d_ffound;
   static int first_time [FMAX+1];
   static char saved_fspec [FMAX+1][PATHSIZE];  /* our own copy of filespec */
   static char found_fspec [FMAX+1][PATHSIZE];  /* matched filename */
   static unsigned long context [FMAX+1]; /* needed by VMS */
   if (what == 0) {
      strcpy (saved_fspec[fileset], filespec);  /* save the filespec */
      first_time[fileset] = 1;
      return (0);
   }

   /* Reach here if what is not 0, so it must be 1 */

   /* Create a descriptor for the wildcarded filespec */
   d_fwild.dsc$w_length = strlen (saved_fspec[fileset]);
   d_fwild.dsc$a_pointer = saved_fspec[fileset];
   d_fwild.dsc$b_class = DSC$K_CLASS_S;
   d_fwild.dsc$b_dtype = DSC$K_DTYPE_T;

   d_ffound.dsc$w_length = sizeof (found_fspec[fileset]);
   d_ffound.dsc$a_pointer = found_fspec[fileset];
   d_ffound.dsc$b_class = DSC$K_CLASS_S;
   d_ffound.dsc$b_dtype = DSC$K_DTYPE_T;

   if (first_time[fileset]) {
      first_time[fileset] = 0;
      context[fileset] = 0L;   /* tell VMS this is first search */
   }
   status = LIB$FIND_FILE (&d_fwild, &d_ffound, &context[fileset]);
   status = status & 1; /* use only lowest bit */

   if (status == 0) {
      LIB$FIND_FILE_END (&context[fileset]);
      return ((char *) 0);
   } else {
      found_fspec[fileset][d_ffound.dsc$w_length] = '\0'; /* just in case */
      p = found_fspec[fileset];
      while (*p != ' ' && *p != '\0')
         p++;
      if (*p != '\0')
         *p = '\0';
      return (found_fspec[fileset]);
   }
}

/* Compensate for bug in VAX/VMS C exit() function */

#ifdef exit
# undef exit
#endif

int bugexit (n)
int n;
{
   exit (1);
}
