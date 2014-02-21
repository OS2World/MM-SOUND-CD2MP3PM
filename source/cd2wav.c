/* CD2MP3 PM 1.14 (C) 1998-2001 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "spawner.h"
#include "cd2mp3pm.h"
#include "cd2wav.h"

HAPP cd2wav(CD2WAV_IN *input, CD2WAV_END *output)
{
   char cmdline[1024];
   char wavdir[256];
   char oldname[256];
   char *backslash;
   SPAWNER_PARAM spawnerParam;

   backslash=strrchr(input->wavname,'\\');
   if(!backslash)
      strcpy(wavdir,".");
   else
   {
      memset(wavdir,0,sizeof(wavdir));
      memcpy(wavdir,input->wavname,backslash-input->wavname);
   }

   strcpy(oldname,wavdir);
   strcat(oldname,"\\");
   sprintf(strchr(oldname,0),input->grabber->mangling,input->track);

   /* building the command line */
   strcpy(cmdline, input->grabber->before);
   strcat(cmdline, " ");

   if(!input->grabber->noDrive)
   {
      *(strchr(cmdline, 0)+2) = 0;
      *(strchr(cmdline, 0)+1) = ':';
      *strchr(cmdline, 0) = input->cddrive;
      strcat(cmdline, " ");
   }

   sprintf(strchr(cmdline,0),"%s%d ", input->grabber->input, input->track);

   if(input->grabber->acceptsOutput)
      sprintf(strchr(cmdline,0),"%s%s\\Track ", input->grabber->output, wavdir);

   strcat(cmdline,input->grabber->after);
   strcat(cmdline," ");
   strcat(cmdline,input->custom);

   output->oldname = (char*) malloc(strlen(oldname)+1);
   output->wavname = (char*) malloc(strlen(input->wavname)+1);
   strcpy(output->oldname, oldname);
   strcpy(output->wavname, input->wavname);

   spawnerParam.program = input->grabber->exe;
   spawnerParam.cmdline = cmdline;
   if(input->lowprio)
   {
      spawnerParam.prioClass = PRTYC_IDLETIME;
      spawnerParam.prioDelta = 0; /* wee bit lower than the encoder */
   }
   else
   {
      spawnerParam.prioClass = PRTYC_REGULAR;
      spawnerParam.prioDelta = 0;
   }
   spawnerParam.hwnd = input->hwnd;

   switch(input->grabber->type)
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

BOOL cd2wav_end(CD2WAV_END *input)
{
   int rc;

   rc = rename(input->oldname,input->wavname);
   free(input->oldname);
   free(input->wavname);

   return !rc;
}
