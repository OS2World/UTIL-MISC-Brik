/* nextfile.c */
/* ::[[ @(#) turboc.c 1.4 89/03/10 19:10:15 ]]:: */
#ifndef LINT
static char sccsid[]="::[[ @(#) turboc.c 1.4 89/03/10 19:10:15 ]]::";
#endif

/*
This file is used only for MS-DOS & Turbo C.
*/

/*
Checksum: 2196257943      (check or update this with "brik")
*/

/*
nextfile() is a general wildcard expansion function that may be used
with other programs.  Usage instructions are below.  It does not
simply expand wildcards in an entire argument list.  Instead, it is
called in a loop as described below, and returns one matching
filename each time it is called.

These functions are for the SMALL MEMORY MODEL ONLY.
*/

#include "assert.h"
#include "brik.h"

#define  FMAX  2        /* Number of different filename patterns */
#define  PATHSIZE 200   /* Size of MS-DOS pathname */
#define  NULL  0

#ifdef ANSIPROTO
char *strtcpy (char *, char *);
int strlen (char *);
char *strcpy (char *, char *);
#endif


/* Structure definitions for MS-DOS software interrupt intdos() */

struct WORD_REGISTERS {
   unsigned int ax, bx, cx, dx, si, di, carry, flags;
};

/* byte registers */

struct BYTE_REGISTERS {
   unsigned char al, ah, bl, bh, cl, ch, dl, dh;
};

union REGS {
   struct WORD_REGISTERS x;
   struct BYTE_REGISTERS h;
};

int intdos (union REGS *, union REGS *);

/*
format of disk transfer address after MS-DOS calls FindFirst and
FindNext
*/
struct dta_t {
   char junk[22];
   int time;
   int date;
   long size;
   char fname[13];
   char just_in_case[4];   /* in case MS-DOS writes too much */
};

void setdta (struct dta_t *);
void fcbpath (struct dta_t *, char *, char *);

/*******************/
/*
nextfile() returns the name of the next source file matching a filespec.

INPUT
   what: A flag specifying what to do.  If "what" is 0, nextfile()
      initializes itself.  If "what" is 1, nextfile() returns the next
      matching filename.
   filespec:  The filespec, usually containing wildcard characters, that
      specifies which files are needed.  If "what" is 0, filespec must be
      the filespec for which matching filenames are needed.  If "what" is 1,
      nextfile() does not use "filespec" and "filespec" should be NULL to
      avoid an assertion error during debugging.
   fileset:  nextfile() can keep track of more than one set of filespecs.
      The fileset specifies which filespec is being matched and therefore
      which set of files is being considered.  "fileset" can be in the
      range 0:FMAX.  Initialization of one fileset does not affect the
      other filesets.

OUTPUT
   IF what == 0 THEN
      return value is NULL
   ELSE IF what == 1 THEN
      IF a matching filename is found THEN
         return value is pointer to matching filename including supplied path
      ELSE
         IF at least one file matched previously but no more match THEN
            return value is NULL
         ELSE IF supplied filespec never matched any filename THEN
            IF this is the first call with what == 1 THEN
               return value is pointer to original filespec
            ELSE
               return value is NULL
            END IF
         END IF
      END IF
   END IF

NOTE

   Initialization done when "what"=0 is not dependent on the correctness
   of the supplied filespec but simply initializes internal variables
   and makes a local copy of the supplied filespec.  If the supplied
   filespec was illegal, the only effect is that the first time that
   nextfile() is called with "what"=1, it will return the original
   filespec instead of a matching filename.  That the filespec was
   illegal will become obvious when the caller attempts to open the
   returned filename for input/output and the open attempt fails.

USAGE HINTS

nextfile() can be used in the following manner:

      char *filespec;                  -- will point to filespec
      char *this_file;                 -- will point to matching filename
      filespec = parse_command_line(); -- may contain wildcards
      FILE *stream;

      nextfile (0, filespec, 0);          -- initialize fileset 0
      while ((this_file = nextfile(1, (char *) NULL, 0)) != NULL) {
         stream = fopen (this_file, "whatever");
         if (stream == NULL)
            printf ("could not open %s\n", this_file);
         else
            perform_operations (stream);
      }
*/

char *nextfile (what, filespec, fileset)
int what;                        /* whether to initialize or match      */
register char *filespec;         /* filespec to match if initializing   */
register int fileset;            /* which set of files                  */
{
   static struct dta_t new_dta [FMAX+1];     /* our own private dta        */
   static int first_time [FMAX+1];
   static char pathholder [FMAX+1][PATHSIZE]; /* holds a pathname to return */
   static char saved_fspec [FMAX+1][PATHSIZE];/* our own copy of filespec   */
   union REGS regs;

   assert(fileset >= 0 && fileset <= FMAX);
   if (what == 0) {
      assert(filespec != NULL);
      strcpy (saved_fspec[fileset], filespec);  /* save the filespec */
      first_time[fileset] = 1;
      return ((char *) NULL);
   }

   setdta (&new_dta[fileset]);   /* set new dta -- our very own */
   assert(what == 1);
   assert(filespec == NULL);
   assert(first_time[fileset] == 0 || first_time[fileset] == 1);

   if (first_time[fileset]) {             /* first time -- initialize etc. */
      /* find first matching file */
      regs.h.ah = 0x4e;                   /* FindFirst MS-DOS call    */
      regs.x.dx = (unsigned int) saved_fspec[fileset]; /* filespec to match */
      regs.x.cx = 0;                      /* search attributes       */
      intdos (&regs, &regs);
   } else {
      /* find next matching file */
      regs.h.ah = 0x4f;                   /* FindNext MS-DOS call     */
      intdos (&regs, &regs);
   }

   if (regs.x.carry != 0) {            /* if error status                  */
      if (first_time[fileset]) {       /*   if file never matched then     */
         first_time[fileset] = 0;
         return (saved_fspec[fileset]);/*      return original filespec    */
      } else {                         /*   else                           */
         first_time[fileset] = 0;      /*                                  */
         return ((char *) NULL);         /*      return (NULL) for no more   */
      }
   } else {                                        /* a file matched */
      first_time[fileset] = 0;
      /* add path info  */
      fcbpath (&new_dta[fileset], saved_fspec[fileset], pathholder[fileset]);
      return (pathholder[fileset]);                /* matching path  */
   }
} /* nextfile */

/*******************/
/* This function sets the dta to a new dta */
void setdta (dta)
struct dta_t *dta;
{
   union REGS regs;
   regs.h.ah = 0x1a;                /* SetDTA Call       */
   regs.x.dx = (unsigned int) dta;  /* new DTA address   */
   intdos (&regs, &regs);
}

/*******************/
/*
fcbpath() accepts a pointer to the Disk Transfer Area, a character
pointer to a pathname that may contain wildcards, and a character
pointer to a buffer.  It copies into the buffer the path prefix from
the pathname and the filename prefix from the DTA so that it forms a
complete path.
*/

void fcbpath (dta, old_path, new_path)
struct dta_t *dta;
char *old_path;
register char *new_path;
{
   register int i;
   int length, start_pos;

   strcpy(new_path, old_path);               /* copy the whole thing first */
   length = strlen(new_path);
   i = length - 1;                           /* i points to end of path */
   while (i >= 0 && new_path[i] != '/' && new_path[i] != '\\' && new_path[i] != ':')
      i--;
   /* either we found a "/", "\", or ":", or we reached the beginning of
      the name.  In any case, i points to the last character of the
      path part. */
   start_pos = i + 1;
   for (i = 0; i < 13; i++)
      new_path[start_pos+i] = dta->fname[i];
   new_path[start_pos+13] = '\0';
}
/* -- END OF nextfile() and related functions -- */

extern unsigned _stklen = 10000;

#include <conio.h>
brktst() { kbhit(); }      /* test for user interrupt */
