$! This command file compiles brik for VAX/VMS and also defines a
$! symbol so you can type "brik" to execute the program.
$!
$! ::[[ @(#) makebrik.com 1.1 89/03/09 15:25:56 ]]::
$!
$ cc /define=VMS addbfcrc
$ cc /define=VMS brik.c
$ cc /define=VMS vms.c
$ cc /define=VMS getopt.c
$ link/executable=brik.exe  brik,addbfcrc,vms,getopt, options/opt
$ brik:==$'f$trnlnm("sys$disk")''f$directory()'brik.exe
