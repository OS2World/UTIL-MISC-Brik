/* ::[[ @(#) brik.h 1.26 89/03/11 16:54:08 ]]:: */

/*
Checksum:   48859975      (check or update this with "brik")
*/

/*
The contents of this file are hereby released to the public domain.

                                   -- Rahul Dhesi 1989/03/10
*/

/*
Brik assumes eight-bit bytes and the ASCII character set.  There may also
be some implicit assumptions that the parity bit in characters in text
files is always zero.

Options for compiling brik.c on various systems.

TRAIL_TB    Define this if a trailing "t" or "b" is needed in the fopen mode
            string to open files in text or binary mode respectively.  Can
            probably be safely defined even for System V and 4.3BSD, since
            compilers will typically just ignore it.
WILDCARD    Define this if wildcards are to be expanded by this program.
            If WILDCARD is defined, then a function nextfile() must also
            be available that conforms to the specifications in turboc.c.
USEINDEX    Define this symbol to make brik use index() instead of strchr().
            Probably needed only 4.2BSD and earlier.
BRKTST      If defined, brik will explicitly test for user interrupts in
            all long loops, so that the program can easily be interrupted
            on microcomputers that don't accept user interrupts
            asynchronously.  If BRKTST is defined, brik will call the
            function brktst() periodically.  This function should check
            for a user interrupt and abort the program if it has occurred.
NIXSEEK     If seeks are UNIX-like, i.e., seeks are possible at any byte
            offset even in text files, then NIXSEEK may be defined to make
            the -gW option perform faster.  If NIXSEEK is not defined, all
            seeks will be to line boundaries using an offset already
            obtained from ftell().  Even on non-UNIX-like systems, it *may*
            be possible to define NIXSEEK, since when brik seeks to an
            arbitrary byte boundary, it always immediately reads
            sequentially forward to a line boundary.  Seeks are needed only
            for the -gW option, which causes brik to seek back to where
            it found the Checksum: header so it can update the stored CRC.
CHECKSEEK   If seeks are flaky despite NIXSEEK being defined, it may help
            to define CHECKSEEK.  In this case brik will seek, read a line,
            seek again, read the line again, compare the two, and proceed
            only if both reads gave the checksum header it was looking for,
            thus confirming that the seeks are working right.  This is
            a very conservative strategy to minimize the risk of corrupting
            a file by overwriting it at the wrong place due to a faulty
            seek.
BUG1,       If ftell() just after fgets() does not return the correct seek
BUG2        offset of the next line, one of these two symbols can be defined
            (but not both).  Each adds different bug fix code and one of them
            may work for you.
ANSIPROTO   If defined, ANSI-style function prototypes will be used.
            If using this symbol creates trouble because your stdio.h
            has slightly different prototypes, don't define it.
NDEBUG      If this symbol is defined, assert() macros throughout the
            brik code will get nulled out, making the executable code
            slightly smaller.
DEBUG       If this symbol is defined, an undocumented -d switch will
            be accepted that will cause information about Checksum:
            header reads and writes to be printed.
EXITBUG     Define this symbol if the exit() function has a bug causing
            anomalous results if the exit code is not exactly 1.
AVOID_MACROS  Brik uses macros for speed in case-insensitive string
            comparisons.  If you get "macro too long" or "expression too
            complex" or similar compilation errors, you can define the
            symbol AVOID_MACROS.  This will cause slower but more compact
            code to be used that does not use long macros, possibly
            allowing compilation.
LOWERIT     If a fast macro or function call is available that will accept
            a single parameter of type "int" and return its lowercase
            value, the symbol LOWERIT may be defined to invoke it.  This
            macro or function must accept any int value, whether or not
            it represents an uppercase character.  Since LOWERIT is
            never called with side-effects, it can safely be a macro.
            If any include file is needed, include it here.  For example,
            if a tolower() macro or function is available that requires
            <ctype.h> to be included, use "#include <ctype.h>" followed
            by "#define LOWERIT tolower" somewhere in brik.h.
STRNICMP    If a case-insensitive implementation of strncmp is available,
            define STRNICMP to be equivalent to it.  If STRNICMP is not
            defined, brik uses its own case-insensitive string comparison
            function.  STRNICMP must accept the same arguments as strncmp.
BINCHAR     Brik uses a simple range test to detect binary characters and
            warn the user if a text mode CRC is being used on a binary file.
            The user may optionally define his own BINCHAR(c) macro, which
            must return nonzero if c should be considered a binary character.
NOCASE      This symbol should be defined if the filesystem is case-
            insensitive.  It will cause all filenames printed to be in
            lowercase.  This will help make a list of files generated by
            the -G option to be more easily usable on systems with case-
            sensitive filesystems, as most file transfer mechanisms (e.g.
            zmodem, kermit, zoo archives) will be compatible with this.

                                     -- Rahul Dhesi
                                        1989/03/11
                                        UUCP:      iuvax!bsu-cs!dhesi
                                        Internet:  dhesi@bsu-cs.bsu.edu
*/

#ifdef TURBOC
# define TRAIL_T
# define WILDCARD
# define ANSIPROTO
# define BRKTST
# define BUG1
# define NOCASE
# define STRNICMP    strnicmp
  int strnicmp (char *, char *, unsigned);
# include <ctype.h>
# define LOWERIT tolower
#endif /* TURBOC */

#ifdef SYS_V
# define NIXSEEK
#endif /* SYS_V */

#ifdef BSD
# define NIXSEEK
# define USEINDEX
#endif /* BSD */

#ifdef VMS
# define WILDCARD
# define CHECKSEEK
# define BUG2
# define EXITBUG
# define NOCASE
#endif /* VMS */

#ifdef BUG1
# define SEEKFIX  \
 fgetc(fptr);while(lowerit(fgetc(fptr))!='c')fseek(fptr,-2L,1);fseek(fptr,-1L,1);
#endif


#ifdef BUG2
# define SEEKFIX  \
   fseek(fptr,-2L,1);while(fgetc(fptr)!='\n');
#endif

#ifndef BUG1
# ifndef BUG2
#  define SEEKFIX
# endif
#endif

/* another thing to try */
/* fseek(fptr,-2L,1);while(lowerit(fgetc(fptr))!='C');fseek(fptr,-1L,1); */

#ifdef EXITBUG
# define exit bugexit
#endif
