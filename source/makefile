#
#  cd2mp3pm.exe makefile for NMAKE and VAC++
#

CC = icc
AS = wasm
RC = rc

.ASM.OBJ:
   $(AS) $(AFLAGS) $*.ASM

.RC.RES:
   $(RC) -r $*.rc

all: cd2mp3pm.exe

CFLAGS = /Tdp /Gm /Ss /Q /B "/PM:PM" /DTCPV40HDRS

!IFDEF DEBUG
CFLAGS = /Ti $(CFLAGS)
!ENDIF

!IFNDEF NOTOPT
CFLAGS = /B "/PACKCODE /PACKDATA" /Gf /Gi /O /Ol /G5 $(CFLAGS)
!ENDIF

OBJECTS = cd2mp3pm.obj cd2wav.obj dlgcd.obj dlgmp3.obj dlgwav.obj \
		  miscsam.obj pmsam.obj prfsam.obj readcd.obj spawner.obj \
		  wav2mp3.obj wav.obj dlgcddb.obj nerrno.obj cddb.obj http.obj \
		  tcpipsock.obj cd2mp3pm.def

RESOURCE = cd2mp3.res
LIBS = so32dll.lib tcp32dll.lib ciscgi\ciscgi.lib

cd2mp3pm.exe: $(OBJECTS) $(RESOURCE)
   $(CC) $(CFLAGS) /Fm$* /Fe$@ $(OBJECTS) $(LIBS)
   $(RC) $(RESOURCE) $@

miscsam.obj:
prfsam.obj:
pmsam.obj: prfsam.h pmsam.h
wavcheck.obj: wavcheck.h
readcd.obj: readcd.h pmsam.h miscsam.h
nerrno.obj:
tcpipsock.obj: nerrno.h miscsam.h tcpipsock.h pmsam.h
http.obj: nerrno.h miscsam.h tcpipsock.h http.h pmsam.h
cddb.obj: pmsam.h miscsam.h readcd.h tcpipsock.h http.h cddb.h cgicis.h

cd2mp3pm.obj: prfsam.h pmsam.h cd2mp3pm.h cd2mp3.h wav2mp3.h cd2wav.h \
			  dlgwav.h dlgmp3.h dlgcd.h dlgcddb.h miscsam.h readcd.h  \
			  tcpipsock.h http.h cddb.h id3tag.h

spawner.obj: pmsam.h cd2mp3pm.h spawner.h
wav2mp3.obj: pmsam.h miscsam.h cd2mp3pm.h spawner.h wav.h wav2mp3.h
cd2wav.obj: spawner.h cd2mp3pm.h cd2wav.h

dlgmp3.obj: prfsam.h pmsam.h cd2mp3pm.h cd2mp3.h miscsam.h
dlgcd.obj: prfsam.h pmsam.h cd2mp3pm.h cd2mp3.h miscsam.h readcd.h \
		   tcpipsock.h http.h cddb.h
dlgcddb.obj: prfsam.h pmsam.h cd2mp3pm.h cd2mp3.h miscsam.h readcd.h \
			 tcpipsock.h http.h cddb.h
dlgwav.obj: prfsam.h pmsam.h cd2mp3pm.h cd2mp3.h wav.h

cd2mp3.res: cd2mp3.h data.ico play.ico qmark.ico stop.ico track.ico wav.ico \
            refresh.ico break.ico convert.ico unplug.ico
