/* @(#) assert.h 1.5 89/03/08 14:59:02 */

/* I claim no copyright over the contents of this file.  -- Rahul Dhesi */

/*
Checksum: 3376323427      (check or update this with "brik")
*/

/*
**   Assertions are enabled only if NDEBUG is not defined.
**   Not all compilers define __FILE__ and __LINE__.  If yours
**   doesn't, you can just let NDEBUG remain #defined in brik.h and no
**   assertions will be compiled in.  Or, if you want to enable
**   assertions, redefine the assert() macro so it works with your
**   compiler.
*/

#ifndef OK_STDIO
# include <stdio.h>
# define OK_STDIO
#endif

#ifndef NDEBUG
#define assert(E) \
{if(!(E))fprintf(stderr,"Assertion error in %s line %d\n",__FILE__,__LINE__);}
#else
#define assert(E)
#endif /* NDEBUG */
