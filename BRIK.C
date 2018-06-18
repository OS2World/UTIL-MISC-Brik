/* ::[[ @(#) brik.c 1.44 89/03/12 14:34:35 ]]:: */
#ifndef LINT
 static char sccsid[]="::[[ @(#) brik.c 1.44 89/03/12 14:34:35 ]]::";
#endif

#define DATESTAMP "1989/03/12 14:34:35"

/*
(c) Copyright 1989 Rahul Dhesi, All rights reserved.  Permission is
granted to copy and distribute this file in modified or unmodified
form, for any purpose whether commercial or noncommercial, provided
that (a) this paragraph, and the author identification printed by
the program, are preserved unchanged, and (b) no attempt is made to
restrict redistribution of this file.  Inclusion of this file in a
collection whose distribution is restricted by a compilation copyright
shall be considered an attempt to restrict redistribution of this file.
Notwithstanding the above, there are no restrictions on the copying or
distribution of not-human-readable executable machine code created by
processing this file with a language translator.
*/

/*
Checksum: 1280097858      (check or update this with "brik")
*/

/*
The following code assumes the ASCII character set and 8-bit bytes.
*/

#ifndef OK_STDIO
# include <stdio.h>
# define OK_STDIO
#endif

#include "brik.h"          /* configuration options */
#include "assert.h"

FILE *fopen();
FILE *efopen();
char *fgets();
int fclose();
char *strchr();
int strlen();
int strcmp();
char *strcat();
int fseek();
long ftell();
int fwrite();
void hdrcrc();
void showerr();
void longhelp();
void shorthelp();
long findcrc();

#ifdef ANSIPROTO
void addbfcrc (char *, int);
int printhdr (void);
FILE *efopen (char *, char *, int);
long xatol (char *);
int main (int, char **);
int dofname (char *);
int dofptr (FILE *, char *);
int whole_check (FILE *, char *);
void hdrcrc (FILE *, char *);
long findcrc (FILE *, char *);
int updatefile (FILE *, long, long, char *);
void longhelp(void);
void shorthelp(void);
void showerr (char *, int);
char suffix(void);
int lowerit (int);

int strlen (char *);
char *strchr (char *, char);
char *strcpy (char *, char *);
char *strcat (char *, char *);
char *strncat (char *, char *, int);
int getopt (int, char **, char *);
void exit (int);
int strcmp (char *, char *);
int readnames (FILE *);
char *nextfile (int, char *, int);
void brktst(void);
#endif

#define MYNL      10       /* newline for CRC calculation */
#define ERRLIMIT 127       /* exit(n) returns n not exceeding this */
#define LINESIZE  8192     /* handle lines of up to this length */
#define ERRBUFSIZ 1024     /* buffer size for perror() message */
#define PATTERN   "Checksum:"    /* look for this header */
#define CHARSINCRC 10      /* characters in CRC */
#define BACKSIZE  1024     /* how much to seek back looking for header */
#define CMTCH     '#'      /* begins comment in CRC list */

/* define macro for testing if chars within range */
#ifndef BINCHAR
# define LOW_PRINT '\007'   /* smallest printable char */
# define HIGH_PRINT '\176'  /* biggest printable char */
# define BINCHAR(c)     ((c) < LOW_PRINT || (c) > HIGH_PRINT)
#endif /* BINCHAR */

/* error levels */
#define LVL_WARN  0
#define LVL_ERR   1
#define LVL_FATAL 2

#ifdef  USEINDEX
# define strchr   index
#endif  /* USEINDEX */

#ifdef TRAIL_T             /* need trailing "t" in fopen string */
# define BRIK_RD  "rt"
# define BRIK_RW  "r+t"
# define BRIK_RDB "rb"
#else
# define BRIK_RD  "r"
# define BRIK_RW  "r+"
# define BRIK_RDB "r"
#endif   /* TRAIL_T */

#define  whitespace(x)     (strchr(" \t\n",(x)) != NULL)
/* format strings for printing CRCs and filenames etc. */
static char ok[] =      "ok ";
static char bad[] =     "BAD";
static char blank[] =   "   ";
static char fmtstr[] = "%10lu%c %s %s\n";


int patlen;                /* length of PATTERN */
int errcount = 0;          /* count of errors */
int gen1 = 0;              /* generate CRCs for all files */
int gen2 = 0;              /* generate CRCs for files with headers */
int silent = 0;            /* be quiet, just set error status */
int verbose = 0;           /* be verbose, print message for good files too */
int updfile = 0;           /* update file by inserting CRC */
int check1 = 0;            /* whether to check header crcs */
int check2 = 0;            /* whether to check whole file crcs */
int fromfile = 0;          /* read filenames from a file */
int binary = 0;            /* manipulate binary file */
int trailing = 0;          /* include trailing empty lines */

#ifdef DEBUG
int debugging = 0;
#endif

/* opens file, prints error message if can't open */
FILE *efopen (fname, mode, level)
char *fname;               /* filename to open */
char *mode;                /* mode, e.g. "r" or "r+" */
int level;                 /* error level */
{
   FILE *fptr;
   fptr = fopen (fname, mode);
   if (fptr == NULL)
      showerr (fname, level);
   return (fptr);
}

/* LOWERIT is a function or macro that returns lowercase of a character */
#ifndef LOWERIT
# ifdef AVOID_MACROS
#  define LOWERIT    lowerit
# else
#  define LOWERIT(c)        ((c)>='A' && (c)<='Z' ? ('a'-'A')+(c) : (c))
# endif
#endif

/* Function needed by SEEKFIX code even if a macro is available */
int lowerit (c) int c;  /* returns lowercase of an ASCII character */
{
  if (c >= 'A' && c <= 'Z') return (('a'-'A') + c);
  else return (c);
}

/* STRNICMP is a case-insensitive strncmp */
#ifndef STRNICMP
int STRNICMP (s1, s2, n)
register char *s1, *s2;
int n;
{
   assert (n >= 0);
   assert (LOWERIT('X') == 'x');
   assert (LOWERIT('*') == '*');

   for ( ; LOWERIT(*s1) == LOWERIT(*s2);  s1++, s2++) {
      if (--n == 0 || *s1 == '\0')
         return(0);
   }
   return(LOWERIT(*s1) - LOWERIT(*s2));
}
#endif /* STRNICMP */

#ifdef AVOID_MACROS
# define BRINCMP     STRNICMP
#else
# define BRINCMP(s1,s2,n) (LOWERIT(*(s1))!=LOWERIT(*(s2))||STRNICMP(s1,s2,n))
#endif


#define xdigit(x)    ((x) >= '0' && (x) <= '9')

/*
xatol is given a string that (supposedly) begins with a string
of digits.  It returns a corresponding positive numeric value.
*/
long xatol (str)
char *str;
{
   long retval;
   retval = 0L;
   while (xdigit(*str)) {
      retval = retval * 10L + (*str-'0');
      str++;
   }
   return (retval);
}

main (argc, argv)
int argc;
char **argv;
{
   int i;
   int c;                        /* next option letter */
   int count = 0;                /* count of required options seen */
   char *infname;                /* name of file to read filenames from */
   FILE *infptr;                 /* open file ptr for infname */

   extern int optind;            /* from getopt: next arg to process */
   extern int opterr;            /* used by getopt */

   opterr = 0;                   /* so getopt won't print err msg */

   patlen = strlen (PATTERN);

#ifdef DEBUG
   while ((c = getopt (argc, argv, "cCgGsvWfbThd")) != EOF)
#else
   while ((c = getopt (argc, argv, "cCgGsvWfbTh")) != EOF)
#endif
   {
      switch (c) {
         case 'c':   check1++; count++; break;
         case 'C':   check2++; count++; break;
         case 'g':   gen1++; count++; break;
         case 'G':   gen2++; count++; break;
         case 's':   silent++; verbose = 0; break;
         case 'v':   verbose++; silent = 0; break;
         case 'W':   updfile++; break;
         case 'f':   fromfile++; break;
         case 'b':   binary++; trailing = 0; break;
         case 'T':   trailing++; binary = 0; break;
#ifdef DEBUG
         case 'd':   debugging++; break;
#endif
         case 'h':   longhelp();
         case '?':   shorthelp();
      }
   }

   if (count != 1)
      shorthelp();

   if (binary && (check1 || gen1)) {
      fprintf (stderr, "brik: fatal: Can't read or update CRC header in binary mode\n");
      exit (1);
   }

   if (updfile && !gen1) {
      fprintf (stderr, "brik: fatal: Use of -W requires -g\n");
      exit (1);
   }

   if ((gen1 || gen2) && !updfile)
      silent = 0;

   i = optind;

   if (fromfile) {                  /* read filenames from file */
      if (i >= argc) {              /* need filenames after -f */
         fprintf (stderr, "brik: fatal: Filename(s) needed after -f\n");
         exit (1);
      }
      for (; i < argc;  i++) {
         infname = argv[i];
         if (strcmp(infname, "-") == 0) { /* "-" means stdin */
            readnames (stdin);
         } else {
#ifdef WILDCARD
            extern char *nextfile();
            nextfile (0, infname, 0);     /* initialize fileset 0 */
            while ((infname = nextfile(1, (char *) NULL, 0)) != NULL) {
               infptr = efopen (infname, BRIK_RD, LVL_ERR);
               readnames (infptr);
               fclose (infptr);
            }
#else
            infptr = efopen (infname, BRIK_RD, LVL_ERR);
            readnames (infptr);
            fclose (infptr);
#endif /* WILDCARD */
         }
      }
   } else {                         /* read filenames from command line */
      if (i >= argc) {
#ifdef TRAIL_T
         if (binary) {
            fprintf (stderr, "brik: fatal: Can't handle stdin in binary mode\n");
            exit (1);
         }
#endif
         dofptr (stdin, "stdin");      /* if no files, read stdin */
      } else {
         for (;  i < argc;  i ++) {
#ifdef WILDCARD
            extern char *nextfile();
            char *one_name;               /* a matching filename */
            nextfile (0, argv[i], 0);     /* initialize fileset 0 */
            while ((one_name = nextfile(1, (char *) NULL, 0)) != NULL)
               dofname (one_name);
#else
            dofname (argv[i]);
#endif /* WILDCARD */
         }
      }
   }
errexit:
   if (errcount > ERRLIMIT)
      errcount = ERRLIMIT;       /* don't overflow status code */
   exit (errcount);
   /*NOTREACHED*/
}

/*
**   Reads names from supplied file pointer and handles them.  Just
**   returns if supplied NULL file pointer.  Will also expand wildcards
**   in names read from this file.
*/
readnames (infptr)
FILE *infptr;
{
   char buf[LINESIZE];
   if (infptr == NULL)
      return;
   while (fgets (buf, LINESIZE, infptr) != NULL) {
#ifdef WILDCARD
      char *fname;                  /* matching filename */
      extern char *nextfile();
#endif /* WILDCARD */
      buf[strlen(buf)-1] = '\0'; /* zap trailing newline */
#ifdef WILDCARD
      nextfile (0, buf, 1);     /* initialize fileset 1 */
      while ((fname = nextfile(1, (char *) NULL, 1)) != NULL) {
         dofname (fname);
      }
#else
      dofname (buf);
#endif /* WILDCARD */
   }
}

/* do one filename */
dofname (this_arg)
char *this_arg;
{
   FILE *this_file;
   char *mode;                         /* "r", "rb", "rw", etc. for fopen */
#ifdef BRKTST
   extern void brktst();
   brktst();
#endif

   if (strcmp(this_arg,"-") == 0) {
#ifdef TRAIL_T
      if (binary) {
         fprintf (stderr, "brik: fatal: Can't handle stdin in binary mode\n");
         exit (1);
      }
#endif
      this_file = stdin;
      this_arg = "stdin";
   } else {
      if (updfile) {
         assert (!binary);
         this_file = efopen (this_arg, BRIK_RW, LVL_ERR);
      } else {
         if (binary && !check2) /* check2 reads filenames, not data */
            mode = BRIK_RDB;
         else
            mode = BRIK_RD;
         this_file = efopen (this_arg, mode, LVL_ERR);
      }
   }
   if (this_file == NULL)
      errcount++;
   else {
#ifdef NOCASE
      char *p;
      for (p = this_arg;  *p != '\0';  p++)
         *p = LOWERIT(*p);
#endif
      dofptr (this_file, this_arg);
      fclose (this_file);
   }
}

char suffix()
{
   return (binary ? 'b' : (trailing ? 'T' : ' '));
}


/*
**   Do one file pointer.  Decides if CRC header will be read or written,
**   or whether just the whole file will be read.
*/
dofptr (fptr, fname)
FILE *fptr;
char *fname;
{
   if (check2)
      whole_check (fptr, fname);    /* do whole file check from list */
   else if (gen1 || check1)         /* header-based CRC check or update */
      hdrcrc (fptr, fname);
   else {                           /* whole-file CRC calculation */
      extern long crccode;
      assert (gen2);
      printhdr();
      findcrc (fptr, fname);
      printf (fmtstr, crccode, suffix(), blank, fname);
   }
}


/* Does whole file check from a list of files and CRCs */
whole_check (fptr, listname)
FILE *fptr;                   /* open file ptr of CRC list file */
char *listname;               /* name of CRC list file */
{
   long fcrc;                 /* recorded crc */
   char *fname;               /* name of file whose CRC being checked */
   char buf [LINESIZE];       /* line buffer */
   char *p;                   /* temp ptr */
   FILE *orgfile;             /* file pointer for original file to check */
   int lino = 0;              /* line no. in list file for error msg */
   char *mode;                /* mode string for fopen */

   while (fgets (buf, LINESIZE, fptr) != NULL) {
      lino++;
      p = buf;
      if (*p == CMTCH)              /* skip comment lines */
         continue;
      while (*p != '\0' && whitespace(*p))      /* skip whitespace */
         p++;
      if (*p == '\0')
         continue;                              /* skip empty lines */
      if (!xdigit(*p))
         goto badline;
      fcrc = xatol (p); /* recorded CRC */

      while (xdigit(*p))
         p++;                                   /* skip past numeric chars */

      binary = trailing = 0;
      if (*p == 'b')                            /* 'b' means binary */
         binary = 1;

      if (*p == 'T')                            /* 'T' means trailing mode */
         trailing = 1;

      while (*p != '\0' && !whitespace(*p)) /* to whitespace */
         p++;
      while (whitespace(*p))   /* skip whitespace */
         p++;

      if (*p == '\n' || *p == '\0') {     /* if at end of line */
         goto badline;
      }
      fname = p;
      while (*p != '\0' && !whitespace(*p))  /* skip to whitespace */
         p++;
      *p = '\0';                    /* null-terminate filename */

      if (binary)
         mode = BRIK_RDB;
      else
         mode = BRIK_RD;

      orgfile = efopen (fname, mode, LVL_ERR);
      if (orgfile == NULL) {
         errcount++;
      } else {
         long foundcrc;
         assert (!(binary && trailing));
         foundcrc = findcrc (orgfile, fname);
         if (foundcrc == fcrc) {
            if (verbose)
               printf (fmtstr, foundcrc, suffix(), ok, fname);
         } else {
            if (!silent)
               printf (fmtstr, foundcrc, suffix(), bad, fname);
            errcount ++;
         }
         fclose (orgfile);
      }
   }
   return;
badline:
   fprintf (stderr,
      "brik: error: Abandoning %s due to badly formatted line %d\n",
      listname, lino);
   return;
}


/*
Initializing the CRC to all one bits avoids failure of detection
should entire data stream get cyclically bit-shifted by one position.
The calculation of the probability of this happening is left as
an exercise for the reader.
*/
#define INITCRC   0xFFFFFFFFL;

/*
**   hdrcrc processes one file given an open file pointer
**   and the filename.  The filename is used for messages etc.
**   It does all manipulation of header-related CRCs, i.e.,
**   checking generating header CRC.  It deals only with text files.
*/
void hdrcrc (fptr, fname)
FILE *fptr;
char *fname;
{
   char buf[LINESIZE];
   int lino = 0;
   char *ptr;
   long fcrc;   /* crc recorded in file */
   extern long crccode;
   long hdrpos;                     /* where we found crc header in file */

   crccode = INITCRC;

   assert (!binary);

#ifndef NIXSEEK
   hdrpos = ftell (fptr);
#endif
   while (fgets (buf, LINESIZE, fptr) != NULL) {
#ifdef BRKTST
      extern void brktst();
      brktst();
#endif
      lino++;
      if (BRINCMP (buf, PATTERN, patlen) == 0) {      /* found header */
#ifdef NIXSEEK
         hdrpos = ftell (fptr);        /* seek posn of line with header */
#endif
         ptr = buf + patlen;           /* point to beyond header */
         while (*ptr != '\0' && whitespace(*ptr))
            ptr++;                     /* skip white space */
         fcrc = xatol (ptr);           /* get stored crc */
         while (xdigit(*ptr))
            ptr++;                     /* skip past digits */
         if (check1) {
            if (*ptr == 'T')           /* if 'T' suffix then */
               trailing = 1;          /* ..include trailing empty lines */
            else
               trailing = 0;
         }

         findcrc (fptr, fname);        /* find CRC for rest of file */

         if (gen1) {                   /* generating CRC */
            if (updfile) {             /* if updating file posn */
               updatefile (fptr, hdrpos, crccode, fname); /* then do it */
            } else if (!silent)
               printf (fmtstr, crccode, suffix(), blank, fname);
         } else {                      /* checking CRC */
            if (fcrc == crccode) {
               if (verbose)
                  printf (fmtstr, crccode, suffix(), ok, fname);
            } else {
               if (!silent)
                  printf (fmtstr, crccode, suffix(), bad, fname);
               errcount ++;
            }
         }
         return;
      } /* end if (BRINCMP (...) ) */
#ifndef NIXSEEK
      hdrpos = ftell (fptr);
#endif
   } /* end of while (fgets(...)) */

   /* reach here if header not found -- this is an error */
   if (!silent)
      printf ("%10s      %s\n", "????", fname);
   errcount++;
   return;
}

/* update file with CRC -- must be seekable */
updatefile (fptr, hdrpos, crccode, fname)
FILE *fptr;
long hdrpos;
long crccode;
char *fname;
{
   char buf[LINESIZE];
   int buflen;             /* will hold count of chars in buf */
   int chars_to_print;     /* chars needed to fill in CRC */

   /*
   1 for blank, CHARSINCRC for actual CRC, and possibly
   1 more for 'T' suffix if including trailing empty lines too
   */
   chars_to_print = 1 + CHARSINCRC + (trailing ? 1 : 0);

#ifndef NIXSEEK
   /* hdrpos is already seek position of header */
   if (fseek (fptr, hdrpos, 0) != 0) { /* seek back */
      fprintf(stderr,
         "brik: error: No CRC written, seek failed on %s\n",fname);
      return;
   }

SEEKFIX

   fgets (buf, LINESIZE, fptr);
   if (BRINCMP (buf, PATTERN, patlen) == 0)
      goto foundit;
   fprintf(stderr,
      "brik: error: No CRC written, header lost in %s\n",fname);
   return;
#else
   /* Following code does fseeks in a non-ANSI-conformant way */
   /* hdrpos is seek position *after* header was read.  Need to get back */
   if (hdrpos >= BACKSIZE)
      hdrpos -= BACKSIZE;
   else
      hdrpos = 0L;
   if (fseek (fptr, hdrpos, 0) != 0) {       /* seek back first */
      fprintf(stderr,"brik: error: No CRC written, seek failed on %s\n",fname);
      return;
   }
   /* now seek forward until we see CRC header again */
   hdrpos = ftell (fptr);
   while (fgets (buf, LINESIZE, fptr) != NULL) {
      if (BRINCMP (buf, PATTERN, patlen) == 0)
         goto foundit;
      hdrpos = ftell (fptr);
   }
   fprintf(stderr,"brik: error: No CRC written, header lost in %s\n",fname);
   return;
#endif /* NIXSEEK */

foundit:    /* hdrpos points to line with header */
   if (fseek (fptr, hdrpos, 0) != 0) { /* seek failed */
      fprintf(stderr,"brik: error: No CRC written, seek failed on %s\n",fname);
      return;
   }
SEEKFIX
   /* we are seeked back to the line with the CRC header */

#ifdef CHECKSEEK  /* triple-check seeks */
   {
      char tmpbf1[LINESIZE];
      char tmpbf2[LINESIZE];
      fseek (fptr, hdrpos, 0);
      assert (ftell (fptr) == hdrpos);
SEEKFIX
      fgets (tmpbf1, LINESIZE, fptr);
      fseek (fptr, 0L, 0); fseek (fptr, 0L, 2);    /* exercise seeks */
      fseek (fptr, hdrpos, 0);
      assert (ftell (fptr) == hdrpos);
SEEKFIX
      fgets (tmpbf2, LINESIZE, fptr);
      if (strcmp(tmpbf1,tmpbf2) != 0 || BRINCMP(tmpbf1,PATTERN,patlen) != 0) {
         fprintf (stderr,
            "brik: error: Bad seek on %s, abandoning this file\n", fname);
         return;
      }
      fseek (fptr, hdrpos, 0);
SEEKFIX
   }
#endif /* CHECKSEEK */

#ifdef DEBUG
   if (debugging) {  /* zap newline, print buffer, restore newline */
      int nlpos; char savech;
      nlpos = strlen(buf) - 1;  savech = buf[nlpos];  buf[nlpos] = '\0';
      fprintf (stderr, "read header  [%s]\n", buf);
      buf[nlpos] = savech;
   }
#endif

   buflen = strlen (buf);
#ifdef DEBUG
   if (debugging)  /* need chars_to_print plus one trailing space or newline */
      fprintf(stderr,"need %d chars, have %d\n",chars_to_print+1,buflen-patlen);
#endif
   if (buflen - patlen > chars_to_print) {      /* if enough space */
      char sprbuf[1+CHARSINCRC+1+1+6];  /* blank+CRC+suffix+null+fudge */
      char *ptr;
      int i;
      ptr = &buf[patlen];                 /* point to beyond header */
      sprintf (sprbuf, " %10lu%c", crccode, 'T');
      for (i = 0;  i < chars_to_print;  i++) /* trailing 'T' possibly ignored */
         ptr[i] = sprbuf[i];
      if (ptr[i] != '\n')
         ptr[i] = ' ';           /* terminate with newline or blank */
      fseek (fptr, 0L, 1);       /* after read, must seek before write */
      if (fwrite (buf, 1, buflen, fptr) != buflen) {
         fprintf(stderr,
            "brik: error: Write failed while writing CRC to %s\n",fname);
      } else if (verbose)
         printf (fmtstr, crccode, suffix(), blank, fname);
         /* printf ("%10lu      %s\n", crccode, fname); */
#ifdef DEBUG
      buf[strlen(buf)-1] = '\0'; /* zap trailing newline */
      if (debugging)
         fprintf (stderr, "wrote header [%s]\n", buf);
#endif
   } else {
      fprintf(stderr,"brik: error: Not enough space for CRC in %s\n",fname);
      return;
   }
}

void longhelp()
{
printf ("     Brik 1.0 (%s), a free CRC-32 program by Rahul Dhesi\n\n",
                        DATESTAMP);

printf ("Usage:  brik -cCgGsvWfbT [ file ] ...\n\n");

printf ("Brik 1.0 generates and verifies 32-bit CRC values (checksums).  Optionally\n");
printf ("it will read or update a \"Checksum: xxxxxxxxxx\" header at the beginning of\n");
printf ("a line in which xxxxxxxxxx represents the CRC of all lines in the file\n");
printf ("*after* this header.  One of -c, -C, -g, or -G is required.  If no\n");
#ifdef WILDCARD
printf ("filename is given, or if a filename is -, standard input is read.\n");
printf ("Wildcards are allowed on the command line and in files read with -f.\n\n");
#else
/* extra newline */
printf ("filename is given, or if a filename is -, standard input is read.\n\n");
#endif

printf ("   -g     look for Checksum: header, generate CRC for rest of file\n");
printf ("   -c     get CRC from header, verify CRC of rest of file\n");
printf ("   -G     generate CRC for entire file (add -b for binary files)\n");
printf ("   -C     verify all file CRCs from output of -G (-f is not needed)\n");
printf ("   -b     use binary mode--read file byte by byte, not line by line\n");

#ifdef WILDCARD
printf ("   -f     read filenames (wildcards ok) from specified files\n");
#else
printf ("   -f     read filenames from specified files\n");
#endif

printf ("   -v     be verbose, report all results (else only errors are reported)\n");
printf ("   -s     be silent, say nothing, just return status code\n");
printf ("   -W     after generating CRC with -g, write it to original header\n");
printf ("   -T     include trailing empty lines, normally ignored (text mode only)\n\n");

#ifndef EXITBUG
printf ("The exit code is the number of files with bad or missing CRCs.\n");
#endif

exit (1);
}

/*
**   Generates CRC of an open file, from current file position to end
**   Except in -T mode, will ignore all trailing empty lines in text
**   files.  Algorithm for this is:
**      1.   After each nonempty line, save crccode so far.
**      2.   At end of file, if last line was empty, use saved crccode rather
**           than current one.
*/

long findcrc (fptr, fname)
FILE *fptr;
char *fname;
{
   int count;
   char buf[LINESIZE];
   extern long crccode;
   int warned = 0;
   long savedcrc;       /* save crccode here to handle trailing empty lines */
   int buflen;

   savedcrc = crccode = INITCRC;

   if (binary) {                                   /* binary */
      while ((count = fread (buf, 1, LINESIZE, fptr)) > 0) {
#ifdef BRKTST
         extern void brktst(); brktst();
#endif
         addbfcrc (buf, count);
      }
      return (crccode);
   } else {                                           /* text */
      buflen = 1;                   /* assume empty lines so far */
      while (fgets (buf, LINESIZE, fptr) != NULL) {
         char *p;
         char *limit;
#ifdef BRKTST
         extern void brktst(); brktst();
#endif
         buflen = strlen (buf);
         limit = buf + buflen;
         for (p = buf;  p != limit;  p++) {
            if (!warned && BINCHAR(*p)) {
               fprintf (stderr,
                  "brik: warning: File %s is probably binary, don't trust text mode CRC\n",
                  fname);
               warned = 1;
            }
            if (*p == '\n')
               *p = MYNL;
         }
         addbfcrc (buf, buflen);
         if (buflen != 1)
            savedcrc = crccode;
      }
      if (!trailing && buflen == 1)
         crccode = savedcrc;
      return (crccode);
   }
}

printhdr ()
{
   static int firsttime = 1;
   if (firsttime) {
      printf ("%c Whole file CRCs generated by Brik v1.0.  Use \"brik -C\" to verify them.\n\n",
         CMTCH);
        printf ("%c CRC-32        filename\n", CMTCH);
        printf ("%c ------        --------\n\n", CMTCH);
      firsttime = 0;
   }
}

/*
**   Prints error message via perror().  The message is printed in the
**   format "brik: %s: %s" where the first %s is the level text ("warning",
**   "error", or "fatal") and the second %s is the string supplied by
**   perror().
**
**   superfluous right now because it is only called from efopen()
**   and only with level = LVL_ERR.
*/

void showerr (errmsg, level)
char *errmsg;
int level;
{
#define ERRSTRMAX  40         /* don't copy more than this many chars */
   static char leveltext[][7] =   {"warning", "error", "fatal"};
   char errbuf[ERRBUFSIZ];       /* buffer for error message */
   strcpy (errbuf, "brik: ");
   assert (level >= LVL_WARN && level <= LVL_FATAL);
   strncat (errbuf, leveltext[level], ERRSTRMAX);
   strcat (errbuf, ": ");
   strncat (errbuf, errmsg, ERRSTRMAX);
   perror (errbuf);
}

void shorthelp()
{
   fprintf (stderr,
   "brik: fatal: One of -cCgG required, -svWfbT are optional (-h for help)\n");
   exit (1);
}
