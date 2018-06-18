# Makefile for brik -- for VAX/VMS "mms".  If you don't have mms,
#  use the "makebrik.com" command file instead.
#
# Don't forget to define a symbol "brik" like this:
#  $ brik:==$disk:[dir]brik.exe
# where "disk" is your device name and "dir" is the directory in which
# brik.exe is kept.

# ::[[ @(#) descrip.mms 1.1 89/03/09 20:46:04 ]]::

# Please see brik.h for configuration options.

CFLAGS = /define=VMS
OBJS = brik.obj, addbfcrc.obj, getopt.obj, vms.obj
CC = cc

.c.obj :
	$(CC) $(CFLAGS) $*.c

brik.exe : $(OBJS)
	link/executable=brik.exe  $(OBJS), options/opt

brik.obj : brik.c brik.h assert.h

addbfcrc.obj : addbfcrc.c

vms.obj : vms.c

getopt.obj : getopt.c

