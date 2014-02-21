CD2MP3 1.14b
============

CD2MP3 has it's own code to produce the info tagging for mp3 files, instead
of using the encoders comandline parameters. this made it independet from
encoder restrictions.
to get tags with other formats I added the ability to have the encoder
called with given parameters and the infos from the CDDB, instead of
calculating the mp3 tag. therefor certain new parameters could be set
within cd2mp3pm.cfg:  (example for OggEnc)

EncoderDoesTagging = TRUE    # info tagging should be done by the encoder,
                               using the following parameters - default is false!
EncoderArtist = -a           # encoder commandline parameter for CD artist name
EncoderGenre = -G            # ... CD genre
EncoderYear = -d             # ... publication year
EncoderAlbum = -l            # ... CD/album title
EncoderTitle = -t            # ... track title
EncoderComment = -c          # ... comment (often used for year at CDDB)

(unfortunately the OS/2 port of OggEnc only supports artist and track title
at the moment. check for updates at http://silk.apana.org.au/vorbis )
if something goes wrong and you want to see the encoder calling sequenze, run
CD2MP3PM from console and redirect the output to a file like 
cd2mp3pm > log
(compare "usage" in cd2mp3pm.txt)

CDDB servers (spring 2009)
------------
The default CDDB server is no longer available. You may add:
CDDB servers:                 freedb.freedb.org:8880
CDDB servers through HTTP:    http://freedb.freedb.org:80/~cddb/cddb.cgi


code changes
------------

look for "tagging" in the files 
cd2mp3pm.h cd2mp3pm.c dlgmp3.c wav2mp3.c wav2mp3.h

thanks to
---------

Samuel Audet for this great piece of software and the permission to release
   new changes (unfortunately, he don't plan to do any more upgrades on CD2MP3)
Christian Hennecke for the Ogg Vorbis extension, tips and infos!
Volker Behr for the joint programming sessions
   (this was my first C experience - sort of an experiment ;-) 
Alexander Wagner for his help installing VA C++

----------
Ilja Kaufmann
kiwi <at> perelin <dot> de
Mar. 2003



CD2MP3 1.14a
============

This is a slightly extended version of CD2MP3 1.14 so one can use it to
encode to audio formats other than MP3. I mainly did this to have a nice
front-end for the Ogg Vorbis encoder. Samuel Audet will be away from his
computer for a while and so he allowed me to assemble this package.


The new feature is an additional option for CD2MP3's configuration file
cd2mp3pm.cfg. Specifying the statement

EncoderExtension = blah

in an encoder's definition in cd2mp3pm.cfg will change the extension to
be used for the encoded files to "blah" instead of "mp3". Moreover, if you
define anything else than "mp3", ID3 tagging will be turned OFF. Note that
you don't have to use this command. I have added a definition for the
Ogg Vorbis enocder "oggenc" to the configuration file.

Ogg Vorbis uses a different way for tagging files. Unfortunately, I don't
know enough C to implement this. In fact, the changes done to CD2MP3 were
my first lines of C code ever. But it works. It would be nice if somebody
else would add the appropriate tagging stuff.


What's Ogg Vorbis?
------------------

Ogg Vorbis is an OpenSourced audio format that is both royalty and patent free.
It offers high sound quality (a bit better than MP3 in my opinion), supports
a wide range of encoding rates and streaming. Visit http://www.vorbis.com and
http://www.os2voice.org/VNL/past_issues/VNL0401H/vnewsf5.htm for more information.

You can download the latest OS/2 binaries from Brian Havard's site:
http://silk.apana.org.au/vorbis/


Code Changes
------------

To find the changes I made to the code, grep the files for "ogg".


Thanks to
---------

Samuel Audet for writing this nice front-end,
Brian Havard for porting Ogg Vorbis, and
Christian Langanke for help with debugging.

===============================
Christian Hennecke
christian.hennecke@os2voice.org
September 9, 2001
