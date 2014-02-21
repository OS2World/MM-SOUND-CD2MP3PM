/* CD2MP3 PM 1.14 (C) 1998-2001 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "pmsam.h"
#include "miscsam.h"
#include "spawner.h"
#include "wav.h"
#include "cd2mp3pm.h"
#include "wav2mp3.h"

BOOL SFNCheckPath(char *fullpath, char *newpath, int sizenew)
{
	char expandfullpath[512];
	char drive[_MAX_DRIVE];
	char firstdir[_MAX_DIR];
	char lfn[_MAX_FNAME], sfn[16];
	char ext[_MAX_EXT];
	char *adir = firstdir;
	BOOL changed = FALSE;

	_fullpath(expandfullpath,fullpath,sizeof(expandfullpath));
	_splitpath(expandfullpath, drive, firstdir, lfn, ext);
	strcat(lfn,ext);

   strncpy(newpath,drive,sizenew);
   newpath[sizenew-1] = 0;

   /* when the leading slash is the only character */
   if(*adir == '\\') adir++;

   /* check if all the dir are 8.3 name compatible */
   while(*adir)
   {
      char onedir[_MAX_FNAME] = {0};

      strncpy(onedir,adir,strchr(adir,'\\')-adir);
      LFN2SFN(onedir,sfn);
      if(stricmp(onedir,sfn))
         /* the path is not 8.3 name compatible */
         break;
      adir = strchr(adir,'\\')+1;
   }

	if(*adir)
	{
		/* changing to root directory */
      strncat(newpath,"\\",sizenew-(strchr(newpath,0)-newpath)-1);
		changed = TRUE;
	}
	else
      strncat(newpath,firstdir,sizenew-(strchr(newpath,0)-newpath)-1);

	LFN2SFN(lfn,sfn);
   strncat(newpath,sfn,sizenew-(strchr(newpath,0)-newpath)-1);
	if(stricmp(lfn,sfn))
		changed = TRUE;

#if 0
   /* if the file exists */
   if(!access(newpath,0))
   {
   }
#endif

	return changed;
}

/* HAPP wav2mp3(WAV2MP3_IN *input, WAV2MP3_END *output)  changed for encoder info tagging - kiwi */
HAPP wav2mp3(WAV2MP3_IN *input, WAV2MP3_END *output, CDTRACKRECORD *record)
{
	char mp3file[512];
   char *wavfile;
	char *wavname,*dot;
	char cmdline[1024] = {0};
	SPAWNER_PARAM spawnerParam;

   memset(output,0,sizeof(*output));

   /* naming our MP3 file to be */

	strcpy(mp3file, input->mp3dir);
	wavname = strrchr(input->wavfile, '\\');
	if(wavname == NULL)
		wavname = input->wavfile;
	else
		wavname++;
	strcat(mp3file,wavname);
	dot = strrchr(mp3file,'.');
	if(dot != NULL) *dot = 0;
	/* extension for Ogg Vorbis support - ch */
	if(strlen(input->encoder->extension) == 0) /* add appropriate extension to filename */
		strcat(mp3file,".mp3");
	else
	{
		strcat(mp3file,".");
		strcat(mp3file,input->encoder->extension);

	}

   /* for ID3 tag smacker */
	output->mp3file = (char*)malloc(strlen(mp3file)+1);
	strcpy(output->mp3file,mp3file);


   /* prepare filenames for LFN->8.3 kludge for DOS encoders */
   wavfile = input->wavfile;

   if(input->encoder->type == VDM)
   {
      char sfnmp3file[512],
           sfnwavfile[512];
      BOOL mp3changed = SFNCheckPath(mp3file,sfnmp3file,sizeof(sfnmp3file));
      BOOL wavchanged = SFNCheckPath(input->wavfile,sfnwavfile,sizeof(sfnwavfile));
      if(mp3changed || wavchanged)
      {
         output->renamefrom[0] = (char*)malloc(strlen(sfnmp3file)+1);
         output->renameto[0] = (char*)malloc(strlen(mp3file)+1);
         output->renamefrom[1] = (char*)malloc(strlen(sfnwavfile)+1);
         output->renameto[1] = (char*)malloc(strlen(input->wavfile)+1);

         strcpy(output->renamefrom[0],sfnmp3file);
         strcpy(output->renameto[0],mp3file);
         strcpy(output->renamefrom[1],sfnwavfile);
         strcpy(output->renameto[1],input->wavfile);

         strcpy(mp3file,sfnmp3file);
         wavfile = output->renamefrom[1];

         rename(input->wavfile,sfnwavfile);
      }
   }


   /* building the command line */

	strcpy(cmdline, input->encoder->before);
	strcat(cmdline, " ");

   sprintf(strchr(cmdline,0),"%s\"%s\" ", input->encoder->input, wavfile);

	if(input->encoder->acceptsOutput)
		sprintf(strchr(cmdline,0),"%s\"%s\" ", input->encoder->output, mp3file);

	if(input->bitrate)
	{
		if(input->encoder->bitratePerChannel)
		{
			RIFF RIFFheader;
			int hfile;
			int samplerate, channels, bits, format, size;

         hfile = openWAV(wavfile, READ, &RIFFheader,
								 &samplerate, &channels, &bits, &format, &size);
			if(hfile)
			{
				if(hfile != -1 && channels)
					input->bitrate /= channels;
				closeWAV(hfile);
			}
		}

		sprintf(strchr(cmdline,'\0'), "%s%d ",input->encoder->bitrate,
			input->bitrate*input->encoder->bitrateMath);
	}

	if(input->hq)
		strcat(cmdline,input->encoder->hq);
	else
		strcat(cmdline,input->encoder->lq);
	strcat(cmdline," ");

	strcat(cmdline,input->encoder->after);
	strcat(cmdline," ");
	strcat(cmdline,input->custom);

 /* add appropriate cmd-line-parameters for encoder info tagging - kiwi */

	if(input->encoder->DoesTagging && record != (CDTRACKRECORD *) -1)
	{
   	if(strlen(input->encoder->title) != 0)		
      {
   	   strcat(cmdline," ");
   	   strcat(cmdline,input->encoder->title);
   		strcat(cmdline," \"");
   	 	strcat(cmdline,record->title);
   		strcat(cmdline,"\"");
      }
   	if(strlen(input->encoder->album) != 0)		
      {
   	   strcat(cmdline," ");
   	   strcat(cmdline,input->encoder->album);
   		strcat(cmdline," \"");
   		strcat(cmdline,record->album);
   		strcat(cmdline,"\"");
      }
   	if(strlen(input->encoder->artist) != 0)		
      {
   	   strcat(cmdline," ");
   	   strcat(cmdline,input->encoder->artist);
   		strcat(cmdline," \"");
   	 	strcat(cmdline,record->artist);
  		strcat(cmdline,"\"");
      }
   	if(strlen(input->encoder->year) != 0)		
      {
   	   strcat(cmdline," ");
   	   strcat(cmdline,input->encoder->year);
   		strcat(cmdline," \"");
   		strcat(cmdline,record->year);
   		strcat(cmdline,"\"");
      }
   	if(strlen(input->encoder->comment) != 0)		
      {
   	   strcat(cmdline," ");
   	   strcat(cmdline,input->encoder->comment);
   		strcat(cmdline," \"");
   		strcat(cmdline,record->comment);
   		strcat(cmdline,"\"");
      }
   	if(strlen(input->encoder->genre) != 0)		
      {
   	   strcat(cmdline," ");
   	   strcat(cmdline,input->encoder->genre);
   		strcat(cmdline," \"");
   		strcat(cmdline,record->genre);
   		strcat(cmdline,"\"");
      }

   }



	spawnerParam.program = input->encoder->exe;
   fprintf(stdout,"%s%s",spawnerParam.program,cmdline); /* write comandline to log (info tagging) - kiwi */
	spawnerParam.cmdline = cmdline;
	if(input->lowprio)
	{
		spawnerParam.prioClass = PRTYC_IDLETIME;
		spawnerParam.prioDelta = 1; /* wee bit higher than grabber */
	}
	else
	{
		spawnerParam.prioClass = PRTYC_REGULAR;
		spawnerParam.prioDelta = 1;
	}
	spawnerParam.hwnd = input->hwnd;

   switch(input->encoder->type)
	{
		case WIN32OS2:
			return doWin32OS2(&spawnerParam);
		case VDM:
			spawnerParam.sessionType = PROG_WINDOWEDVDM;
			return doNormal(&spawnerParam);
		case VIO:
			spawnerParam.sessionType = PROG_WINDOWABLEVIO;
			return doNormal(&spawnerParam);
		default:
			return NULLHANDLE;
	}
}

BOOL wav2mp3_end(WAV2MP3_END *input)
{
   int rc = 0;
   int i = 0;

   while(input->renamefrom[i] && input->renameto[i])
   {
      rc |= rename(input->renamefrom[i],input->renameto[i]);
      free(input->renamefrom[i]);
      free(input->renameto[i]);
      i++;
   }

	free(input->mp3file);
   return !rc;
}
