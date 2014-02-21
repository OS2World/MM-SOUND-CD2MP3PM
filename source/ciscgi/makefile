#
#  ciscgi.lib makefile for NMAKE and VAC++
#

CC = icc
AS = wasm
RC = rc

.ASM.OBJ:
   $(AS) $(AFLAGS) $*.ASM

.RC.RES:
   $(RC) -r $*.rc

all: ciscgi.lib

CFLAGS = /Gm /Ss /Q /B "/PM:PM"

!IFDEF DEBUG
CFLAGS = /Ti $(CFLAGS)
!ENDIF

!IFNDEF NOTOPT
CFLAGS = /B "/PACKCODE /PACKDATA" /Gf /Gi /O /Ol /G5 $(CFLAGS)
!ENDIF

OBJECTS = cgifldld.obj cgiupper.obj urldcode.obj urlncode.obj urlvalue.obj
RESOURCE =
LIBS =

ciscgi.lib: $(OBJECTS)
   ilib $@ $(OBJECTS) , ,

cgifldld.obj: cisrcl.h
cgiupper.obj: cisrcl.h
urldcode.obj: cisrcl.h
urlncode.obj: cisrcl.h
urlvalue.obj: cisrcl.h
