/* CD2MP3 PM 1.14 (C) 1998-2001 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "prfsam.h"
#include "pmsam.h"
#include "cd2mp3pm.h"
#include "cd2mp3.h"

#include "miscsam.h"

/* keeps a list of loaded encoders and grabbers, and their settings */
ENCODER *encoders;
int countEncoders;

static BOOL loadConfig(char *file, ENCODER **encoders, int *countEncoders)
{
	FILE *config;
	config = fopen(file,"r");

	if(config)
	{
		char buffer[512], param[256], value[256], *comment;
		int i;

		i = 0;
		while(fgets(buffer,sizeof(buffer),config))
		{
			uncomment(buffer);
			sscanf(buffer,"%[^=]=%[^=]",param,value);
			strip(param);

			if(!stricmp(param,"encoderid")) i++;
		}

		*encoders = (ENCODER *) calloc(i, sizeof(ENCODER));

		rewind(config);

		i = -1;
		while(fgets(buffer,sizeof(buffer),config))
		{
			uncomment(buffer);
			sscanf(buffer,"%[^=]=%[^=]",param,value);
			strip(param);
			strip(value);

			if(!stricmp(param,"encoderid"))
			{
				i++;
				strcpy((*encoders)[i].id, value);
            (*encoders)[i].DoesTagging = FALSE;  /* default for not tagging by encoder - kiwi */
			}
			else if(i >=0)
			{
 				if(!stricmp(param,"encoderexe"))
					strcpy((*encoders)[i].exe, value);
				else if(!stricmp(param,"encodertype"))
				{
					if(!stricmp(value,"win32os2"))
						(*encoders)[i].type = WIN32OS2;
					else if(!stricmp(value,"vio"))
						(*encoders)[i].type = VIO;
					else if(!stricmp(value,"vdm"))
						(*encoders)[i].type = VDM;
				}
				else if(!stricmp(param,"encoderin"))
					strcpy((*encoders)[i].input, value);
				else if(!stricmp(param,"encoderout"))
					strcpy((*encoders)[i].output, value);
				else if(!stricmp(param,"encoderlq"))
					strcpy((*encoders)[i].lq, value);
				else if(!stricmp(param,"encoderhq"))
					strcpy((*encoders)[i].hq, value);
				else if(!stricmp(param,"encoderbefore"))
					strcpy((*encoders)[i].before, value);
				else if(!stricmp(param,"encoderafter"))
					strcpy((*encoders)[i].after, value);
				else if(!stricmp(param,"encoderbitrate"))
					strcpy((*encoders)[i].bitrate, value);
				else if(!stricmp(param,"encoderbrmath"))
					(*encoders)[i].bitrateMath = atoi(value);
				else if(!stricmp(param,"encoderbrpc"))
				{
					if(!stricmp(value,"true"))
						(*encoders)[i].bitratePerChannel = TRUE;
					else
						(*encoders)[i].bitratePerChannel = FALSE;
				}
				else if(!stricmp(param,"encoderacceptsoutput"))
				{
					if(!stricmp(value,"true"))
						(*encoders)[i].acceptsOutput = TRUE;
					else
						(*encoders)[i].acceptsOutput = FALSE;
				}
				/* extension for Ogg Vorbis support - ch */
				else if(!stricmp(param,"encoderextension"))
					strcpy((*encoders)[i].extension, value);

            /* extension for encoder info tagging support - kiwi */
				else if(!stricmp(param,"encoderdoestagging"))
				{
					if(!stricmp(value,"true"))
						(*encoders)[i].DoesTagging = TRUE;

				}

				else if(!stricmp(param,"encoderartist"))
					strcpy((*encoders)[i].artist, value);
				else if(!stricmp(param,"encodergenre"))
					strcpy((*encoders)[i].genre, value);
				else if(!stricmp(param,"encoderyear"))
					strcpy((*encoders)[i].year, value);
				else if(!stricmp(param,"encoderalbum"))
					strcpy((*encoders)[i].album, value);
				else if(!stricmp(param,"encodertitle"))
					strcpy((*encoders)[i].title, value);
				else if(!stricmp(param,"encodercomment"))
					strcpy((*encoders)[i].comment, value);

			}
		}
		*countEncoders = i+1;

		fclose(config);
		return TRUE;
	}
	return FALSE;
}


static void loadIni(HWND hwnd)
{
	char buffer[512];
	char *list;
	HINI inifile;
	int listsize;

	inifile = openProfile(INIFILE);

   if(inifile)
   {
      if(readProfile(inifile,"MP3","Encoder",buffer,sizeof(buffer), TRUE))
         setText(WinWindowFromID(hwnd, CB_ENCODER), CBID_EDIT, buffer);
      if(readProfile(inifile,"MP3","Bitrate",buffer,sizeof(buffer), TRUE))
         setText(WinWindowFromID(hwnd, CB_BITRATE), CBID_EDIT, buffer);
      if(readProfile(inifile,"MP3","HighQuality",buffer,1, FALSE))
         setCheck(hwnd, CB_HQ, *buffer);
      if(readProfile(inifile,"MP3","LowPriority",buffer,1, FALSE))
         setCheck(hwnd, CB_LOWPRIO, *buffer);
      if(readProfile(inifile,"MP3","MP3Dir",buffer,sizeof(buffer), TRUE))
         setText(hwnd, EF_MP3DIR, buffer);
      if(readProfile(inifile,"MP3","Custom",buffer,sizeof(buffer), TRUE))
         setText(hwnd, EF_CUSTOMMP3, buffer);
      if(readProfile(inifile,"MP3","RemoveDeSelect",buffer,1, FALSE))
         setCheck(hwnd, CB_REMOVE, *buffer);

      closeProfile(inifile);
   }
}

static void saveIni(HWND hwnd)
{
	char buffer[512];
	HINI inifile;
	int listcount;
	char *listpos, *list;
	int i;
	SHORT listsize;

	inifile = openProfile(INIFILE);

   if(inifile)
   {
      getText(WinWindowFromID(hwnd, CB_ENCODER), CBID_EDIT, buffer, sizeof(buffer));
      writeProfile(inifile,"MP3","Encoder", buffer,0);
      getText(WinWindowFromID(hwnd, CB_BITRATE), CBID_EDIT, buffer, sizeof(buffer));
      writeProfile(inifile,"MP3","Bitrate", buffer,0);
      *buffer = getCheck(hwnd, CB_HQ);
      writeProfile(inifile,"MP3","HighQuality", buffer, 1);
      *buffer = getCheck(hwnd, CB_LOWPRIO);
      writeProfile(inifile,"MP3","LowPriority", buffer, 1);
      getText(hwnd, EF_MP3DIR, buffer, sizeof(buffer));
      writeProfile(inifile,"MP3","MP3Dir", buffer,0);
      getText(hwnd, EF_CUSTOMMP3, buffer, sizeof(buffer));
      writeProfile(inifile,"MP3","Custom", buffer,0);
      *buffer = getCheck(hwnd, CB_REMOVE);
      writeProfile(inifile,"MP3","RemoveDeSelect", buffer, 1);

      closeProfile(inifile);
   }
}

static MRESULT processControl(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
	switch(SHORT1FROMMP(mp1))
	{
		case CB_BITRATE:
			if(SHORT2FROMMP(mp1) == CBN_EFCHANGE)
				bitRateChanged++;
			break;
	}

	return 0;
}


MRESULT EXPENTRY wpMP3(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
	switch (msg)
	{
		case WM_INITDLG:
		{
			int i;

			insertItemText(hwnd,CB_BITRATE,LIT_END,"Default");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"8");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"16");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"24");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"32");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"40");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"48");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"56");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"64");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"80");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"96");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"112");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"128");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"144");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"160");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"192");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"224");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"256");
			insertItemText(hwnd,CB_BITRATE,LIT_END,"320");

			selectItem(hwnd,CB_BITRATE,12);

			loadConfig(CFGFILE, &encoders, &countEncoders);
			for(i = 0; i < countEncoders; i++)
				insertItemText(hwnd,CB_ENCODER,LIT_END,encoders[i].id);
			selectItem(hwnd,CB_ENCODER,0);

			setCheck(hwnd,CB_LOWPRIO,TRUE);

			loadIni(hwnd);

			return 0;
		}

		case WM_CONTROL:
			return processControl(hwnd,mp1,mp2);

      case WM_CLOSE:
			free(encoders);
			saveIni(hwnd);
			return 0;

		case WM_CHAR:
			if(SHORT2FROMMP(mp2) == VK_ESC)
				return 0;
			else
				break;
	}
	return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}
