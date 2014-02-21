/* CD2MP3 PM 1.14 (C) 1998-2001 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <types.h>
#include <sys\socket.h>

#include "prfsam.h"
#include "pmsam.h"
#include "cd2mp3pm.h"
#include "cd2mp3.h"

#include "wav2mp3.h"
#include "cd2wav.h"

#include "dlgwav.h"
#include "dlgmp3.h"
#include "dlgcd.h"
#include "dlgcddb.h"

#include "miscsam.h"
#include "readcd.h"
#include "tcpipsock.h"
#include "http.h"
#include "cddb.h"

#include "id3tag.h"

HMQ mainhmq;
HAB mainhab;
HWND mainHwnd, frameHwnd;
CDDB_socket *cddbSocket = NULL;

/* notebook page information, see pmsam.h */

NBPAGE nbpage[] =
{
   {wpCD,  "Page 1 of 2  Tab 1 of 4", "~CD",    DLG_CD,    FALSE, BKA_MAJOR | BKA_MINOR, 0, 0},
   {wpCDDB,"Page 2 of 2  Tab 1 of 4", "~CDDB",  DLG_CDDB,  FALSE, BKA_MINOR, 0, 0},
   {wpWAV, "Tab 2 of 4", "~WAV",   DLG_WAV,   FALSE, BKA_MAJOR, 0, 0},
   {wpMP3, "Tab 3 of 4", "~MP3",   DLG_MP3,   FALSE, BKA_MAJOR, 0, 0},
   {NULL,  "Tab 4 of 4", "~About", DLG_ABOUT, FALSE, BKA_MAJOR, 0, 0}
};

/* some constants so that we don't have to modify code if we add pages */
#define PAGE_COUNT (sizeof( nbpage ) / sizeof( NBPAGE ))
#define PAGE_CD     0
#define PAGE_CDDB   1
#define PAGE_WAV    2
#define PAGE_MP3    3
#define PAGE_ABOUT  4

void smackID3tag(char *mp3file, CDTRACKRECORD *record)
{
   ID3TAG tag;
   FILE *file;

   memset(&tag,' ',sizeof(tag)-1);
   memcpy(tag.tag,"TAG",3);

   tag.genre = 255;
   for(int i = 0; i < GENRE_COUNT; i++)
      if(!stricmp(GENRES[i],record->genre))
      {
         tag.genre = i;
         break;
      }

   memcpy(tag.title,record->title,min(strlen(record->title),sizeof(tag.title)));
   memcpy(tag.artist,record->artist,min(strlen(record->artist),sizeof(tag.artist)));
   memcpy(tag.album,record->album,min(strlen(record->album),sizeof(tag.album)));
   memcpy(tag.year,record->year,min(strlen(record->year),sizeof(tag.year)));
   memcpy(tag.comment,record->comment,min(strlen(record->comment),sizeof(tag.comment)));

   file = fopen(mp3file,"ab");
   if(file)
   {
      fwrite(&tag,sizeof(tag),1,file);
      fclose(file);
   }
}

/* stuff used during conversion */
ENCODER useEncoder;
GRABBER useGrabber;
int bitrate;
char *wavfilelist;
int wavcount;
int tracklist[256];
int trackcount;
char tempdir[256];
char mp3dir[256];
char basename[256];
char customcd[256];
char custommp3[256];
BOOL hq, lowprio, remove_deselect;
BOOL superGrab;
char cddrive;

/* currently running stuff */
HAPP runningMP3, runningCD;
GRABBER *runningGrabber;
ENCODER *runningEncoder;

/* variable to abort all operations */
BOOL abortEncoding;

/* hApp -> the app that just terminated, or -1 if new */
void doConversion(HAPP hApp, ULONG rc, HWND hwnd)
{
/* structures to keep alive for when a running app terminates */
	static WAV2MP3_END runningMP3end;
	static CD2WAV_END runningCDend;

/* for deselecting a track when finished */
	static int grabbingTrack;
	static int encodingTrack;

/* for removing wav file from the container when finished */
	static char encodingWavFile[512];

/* current wav file in wavfilelist */
	static char *listpos;

/* current track from tracklist */
	static int trackpos;

/* buffers to manage who's doing what in CD grabbing/encoding */
   static char filetoencode[512], fileencoding[512], filegrabbing[512],
               fileencoded[512];

/* to know what just terminated */
	enum {NONE, WAV2MP3, CD2WAV} finishing = NONE;


CDTRACKRECORD *record=(CDTRACKRECORD *) -1;



	if(hApp == (HAPP) -1) /* first call */
	{
		listpos = wavfilelist;
		trackpos = 0;

		finishing = WAV2MP3; /* fake */
		runningMP3 = NULLHANDLE;
		runningCD = NULLHANDLE;

		*encodingWavFile = 0;
		encodingTrack = 0;
		grabbingTrack = 0;

		*filetoencode = 0;
		*fileencoding = 0;
		*filegrabbing = 0;
	}
	else if(hApp == runningMP3)
	{
      wav2mp3_end(&runningMP3end); /* might free memory */

      /* win32-os/2 app return garbage */
      if(rc != 0 && runningEncoder->type != WIN32OS2)
		{
			updateError("%s returned with errorlevel %d, aborting MP3 encoding.",runningEncoder->exe, rc);
			WinTerminateApp(runningCD);
         abortEncoding = TRUE;
		}
      else
      {
         runningMP3 = NULLHANDLE;
         finishing = WAV2MP3;
         if(*encodingWavFile && remove_deselect)
         {
            removeTitleFromContainer(nbpage[PAGE_WAV].hwnd,CT_WAV,encodingWavFile);
            *encodingWavFile = 0;
         }

         if(encodingTrack)
         {
            record = (CDTRACKRECORD *) enumRecords(nbpage[PAGE_CD].hwnd, CT_TRACK, NULL, CMA_FIRST);
            while(record && record != (CDTRACKRECORD *) -1)
            {
               if(record->track == encodingTrack)
               {
                  if(remove_deselect)
                     selectRecord(nbpage[PAGE_CD].hwnd, CT_TRACK, (RECORDCORE *) record, FALSE);
                  break;
               }
               record = (CDTRACKRECORD *) enumRecords(nbpage[PAGE_CD].hwnd,CT_TRACK, (RECORDCORE *) record, CMA_NEXT);
            }
            encodingTrack = 0;

            remove(fileencoding); /* deletes the temp WAV file */
            *fileencoding = 0;
            /* extension for Ogg Vorbis support -ch */
            /* if(strstr(fileencoded,".mp3") != NULL)  only add ID3 tag to MP3s */

            /* only add ID3 tag to mp3s if not done by the encoder - kiwi */
            /* if(input->encoder->doestagging == FALSE && strstr(fileencoded,".mp3") != NULL) */
            if(useEncoder.DoesTagging == FALSE && strstr(fileencoded,".mp3") != NULL)
              smackID3tag(fileencoded,record);
         }
      }
   }
	else if(hApp == runningCD)
	{
      cd2wav_end(&runningCDend); /* might free memory */

      /* win32-os/2 app return garbage */
      if(rc != 0 && runningGrabber->type != WIN32OS2)
		{
			updateError("%s returned with errorlevel %d, aborting MP3 encoding.",runningGrabber->exe, rc);
			WinTerminateApp(runningMP3);
         abortEncoding = TRUE;
		}
      else
      {
         runningCD = NULLHANDLE;
         finishing = CD2WAV;
         strcpy(filetoencode,filegrabbing);
         *filegrabbing = 0;
      }
   }

	/* aborting */
	if(abortEncoding)
	{
		WinPostMsg(hwnd,WM_COMMAND, MPFROMSHORT(CD2MP3_STOPPED),0);
		return;
	}

	/* do new stuff */
	if(listpos && *listpos && finishing != NONE)
	{
		WAV2MP3_IN input;
		input.hwnd = hwnd;
		runningEncoder = &useEncoder;
		input.encoder = &useEncoder;
		input.wavfile = listpos;
		input.mp3dir = mp3dir;
		input.custom = custommp3;
		input.bitrate = bitrate;
		input.hq = hq;
		input.lowprio = lowprio;
		/* runningMP3 = wav2mp3(&input, &runningMP3end); changed for encoder info tagging - kiwi */
		runningMP3 = wav2mp3(&input, &runningMP3end, record);
      strcpy(fileencoded,runningMP3end.mp3file);
		strcpy(encodingWavFile,listpos);
		listpos = strchr(listpos,0)+1;
		if(!listpos)
			free(wavfilelist);
	}
	else
	{
		if( (finishing == CD2WAV && !runningMP3) ||
			 (superGrab && finishing == WAV2MP3 && *filetoencode) )
		{
			WAV2MP3_IN input;


 			strcpy(fileencoding,filetoencode);
			*filetoencode = 0;

         /* inserted for encoder info tagging - kiwi */

         record = (CDTRACKRECORD *) enumRecords(nbpage[PAGE_CD].hwnd, CT_TRACK, NULL, CMA_FIRST);
         while(record && record != (CDTRACKRECORD *) -1)
         {
            if(record->track == grabbingTrack)
               break;
            record = (CDTRACKRECORD *) enumRecords(nbpage[PAGE_CD].hwnd,CT_TRACK, (RECORDCORE *) record, CMA_NEXT);
         }

         /* till here */

			input.hwnd = hwnd;
			runningEncoder = &useEncoder;
			input.encoder = &useEncoder;
			input.wavfile = fileencoding;
			input.mp3dir = mp3dir;
			input.custom = custommp3;
			input.bitrate = bitrate;
			input.hq = hq;
			input.lowprio = lowprio;
			/* runningMP3 = wav2mp3(&input, &runningMP3end); changed for encoder info tagging - kiwi */
 			runningMP3 = wav2mp3(&input, &runningMP3end, record);
         strcpy(fileencoded,runningMP3end.mp3file);
         encodingTrack = grabbingTrack;
		}

		if(trackpos < trackcount && ( (finishing == WAV2MP3 && !runningCD) ||
			(superGrab && finishing == CD2WAV && !*filetoencode) ) )
		{
			CD2WAV_IN input;
         /* CDTRACKRECORD *record; */
         record = (CDTRACKRECORD *) enumRecords(nbpage[PAGE_CD].hwnd, CT_TRACK, NULL, CMA_FIRST);
         while(record && record != (CDTRACKRECORD *) -1)
         {
            if(record->track == tracklist[trackpos])
            {
               sprintf(filegrabbing, "%s%s.wav", tempdir, record->record.pszIcon);
               break;
            }
            record = (CDTRACKRECORD *) enumRecords(nbpage[PAGE_CD].hwnd,CT_TRACK, (RECORDCORE *) record, CMA_NEXT);
         }

			input.hwnd = hwnd;
			runningGrabber = &useGrabber;
			input.grabber = &useGrabber;
			input.wavname = filegrabbing;
			input.custom = customcd;
			input.track = tracklist[trackpos];
			grabbingTrack = input.track;
			if(superGrab)
				input.lowprio = lowprio;
			else
				input.lowprio = FALSE;
			input.cddrive = cddrive;
			runningCD = cd2wav(&input, &runningCDend);

			trackpos++;
		}

      if(!runningCD && !runningMP3)
			WinPostMsg(hwnd,WM_COMMAND, MPFROMSHORT(CD2MP3_STOPPED),0);
	}
}

void beginConverter(HWND hwnd)
{
	int i;
	char buffer[512];
	CDTRACKRECORD *selectedCDRecord;
	RECORDCORE *WAVRecord;

	/* looking at MP3 settings */

	hq = getCheck(nbpage[PAGE_MP3].hwnd, CB_HQ);
	lowprio = getCheck(nbpage[PAGE_MP3].hwnd, CB_LOWPRIO);
	getText(nbpage[PAGE_MP3].hwnd, EF_MP3DIR, mp3dir, sizeof(mp3dir));
	if(*mp3dir && *(strchr(mp3dir,0)-1) != '\\') strcat(mp3dir,"\\");
	getText(nbpage[PAGE_MP3].hwnd, EF_CUSTOMMP3, custommp3, sizeof(custommp3));

	*useEncoder.id = 0;
	getText(WinWindowFromID(nbpage[PAGE_MP3].hwnd, CB_ENCODER), CBID_EDIT, buffer, sizeof(buffer));
	for(i = 0; i < countEncoders; i++)
	{
		if(!strcmp(encoders[i].id,buffer))
		{
			useEncoder = encoders[i];
			break;
		}
	}
	if(!*useEncoder.id)
		updateError("No valid Encoder chosen.");

	getText(WinWindowFromID(nbpage[PAGE_MP3].hwnd, CB_BITRATE), CBID_EDIT, buffer, sizeof(buffer));
	bitrate = atoi(buffer);

	remove_deselect = getCheck(nbpage[PAGE_MP3].hwnd, CB_REMOVE);

	/* looking at CD settings */

	getText(nbpage[PAGE_CD].hwnd, EF_TITLE, basename, sizeof(basename));
	getText(nbpage[PAGE_CD].hwnd, EF_TEMPDIR, tempdir, sizeof(tempdir));
	if(*tempdir && *(strchr(tempdir,0)-1) != '\\') strcat(tempdir,"\\");
	getText(nbpage[PAGE_CD].hwnd, EF_CUSTOMCD, customcd, sizeof(customcd));
	getText(WinWindowFromID(nbpage[PAGE_CD].hwnd, CB_DRIVE), CBID_EDIT, buffer, sizeof(buffer));
	cddrive = buffer[0];

	*useGrabber.id = 0;
	getText(WinWindowFromID(nbpage[PAGE_CD].hwnd, CB_GRABBER), CBID_EDIT, buffer, sizeof(buffer));
	for(i = 0; i < countGrabbers; i++)
	{
		if(!strcmp(grabbers[i].id,buffer))
		{
			useGrabber = grabbers[i];
			break;
		}
	}
	if(!*useGrabber.id)
		updateError("No valid Grabber chosen.");

	trackcount = 0;
	selectedCDRecord = (CDTRACKRECORD *) searchRecords(nbpage[PAGE_CD].hwnd, CT_TRACK,
							 (RECORDCORE *) CMA_FIRST, CRA_SELECTED);
	while(selectedCDRecord && selectedCDRecord != (CDTRACKRECORD *) -1)
	{
		tracklist[trackcount++] = selectedCDRecord->track;
		selectedCDRecord = (CDTRACKRECORD *)searchRecords(nbpage[PAGE_CD].hwnd, CT_TRACK,
								 (RECORDCORE *) selectedCDRecord, CRA_SELECTED);
	}

	superGrab = getCheck(nbpage[PAGE_CD].hwnd, CB_SUPERGRAB);

	/* looking at WAV settings */
	makeWavList(nbpage[PAGE_WAV].hwnd, CT_WAV, &wavfilelist);

	abortEncoding = FALSE;
	doConversion(-1, 0, hwnd);
}

static MRESULT processCommand(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
	switch(SHORT1FROMMP(mp1))
	{
		case PB_CONVERT:
			disable(hwnd,PB_CONVERT);
			enable(hwnd,PB_STOP);
			beginConverter(hwnd);
			break;

		case PB_STOP:
			abortEncoding = TRUE;
			if(runningCD)
				if(!WinTerminateApp(runningCD))
					updateError("Could not kill %s, sorry.",runningGrabber->exe);
			if(runningMP3)
				if(!WinTerminateApp(runningMP3))
					updateError("Could not kill %s, sorry.",runningEncoder->exe);
			break;

		case CD2MP3_STOPPED:
			enable(hwnd,PB_CONVERT);
			disable(hwnd,PB_STOP);
			break;

		case PB_CANCEL:
			if(cddbSocket)
				cddbSocket->cancel();
			break;
	}
	return 0;
}

static MRESULT processControl(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
#if 0
	switch(SHORT1FROMMP(mp1))
	{
	}
#endif
	return 0;
}

SHORT NBy; /* reserve some client window space for buttons and errors */

static MRESULT processInitdlg(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
	int i;
	HWND hwndNB;
	ULONG xspace;
	POINTL size[3];
	LONG color;
	ACCELTABLE *acceltable;
	ULONG tablesize;
	HACCEL hAccel;
	char *font;

	/* those fuckin "/$%@* accElERATor (drunk) tables.. removing shift-f10
		and shift-f9 for PROPER processing further along in notebook dialogs */
	hAccel = WinQueryAccelTable(mainhab, frameHwnd);
	tablesize = WinCopyAccelTable(hAccel,NULL,0);
	acceltable = (ACCELTABLE *) alloca(tablesize);
	WinCopyAccelTable(hAccel,acceltable,tablesize);

	for(i = 0; i < acceltable->cAccel; i++)
	{
		ACCEL *accelentry = &acceltable->aaccel[i];

		if((accelentry->fs & KC_SHIFT) && accelentry->key == VK_F10)
			accelentry->fs = accelentry->key = accelentry->cmd = 0;

		else if((accelentry->fs & KC_SHIFT) && accelentry->key == VK_F9)
			accelentry->fs = accelentry->key = accelentry->cmd = 0;
	}

	WinSetAccelTable(mainhab, NULLHANDLE, frameHwnd);
	hAccel = WinCreateAccelTable(mainhab, acceltable);
	WinSetAccelTable(mainhab, hAccel, frameHwnd);

	/* creating and loading all the nice stuff */
	hwndNB = createNotebook(hwnd, nbpage, PAGE_COUNT);
	if(!hwndNB) return (MRESULT) 1;

	if(!loadNotebookDlg(hwndNB, nbpage, PAGE_COUNT))
		return (MRESULT) 1;

   size[0].x = 34; size[0].y = 20;
   size[1].x = 26; size[1].y = 20;
   size[2].x = 34; size[2].y = 20;
	WinMapDlgPoints(HWND_DESKTOP, size, sizeof(size)/sizeof(size[0]), TRUE);
   NBy = size[2].y+3;

	xspace = 5 + size[0].x + 5 + size[1].x + 5 + size[2].x + 5;
	initError(hwnd,xspace,0,400,NBy/2);
	initStatus(hwnd,xspace,NBy/2,400,NBy/2);

	xspace = 5 + size[0].x + 5 + size[1].x + 5;
   WinCreateWindow(hwnd, WC_BUTTON, "#1009\tC~ancel",
                   BS_PUSHBUTTON | WS_TABSTOP | WS_VISIBLE | BS_TEXT | BS_MINIICON,
						 xspace, 0, size[2].x, size[2].y,
						 hwnd, HWND_TOP, PB_CANCEL, NULL, NULL);


	xspace = 5 + size[0].x + 5;
   WinCreateWindow(hwnd, WC_BUTTON, "#1007\t~Stop",
                   BS_PUSHBUTTON | WS_TABSTOP | WS_VISIBLE | BS_TEXT | BS_MINIICON,
						 xspace, 0, size[1].x, size[1].y,
						 hwnd, HWND_TOP, PB_STOP, NULL, NULL);

	xspace = 5;
   WinCreateWindow(hwnd, WC_BUTTON, "#1008\t~Convert",
                   BS_PUSHBUTTON | WS_TABSTOP | WS_VISIBLE | BS_TEXT | BS_MINIICON,
						 xspace, 0, size[0].x, size[0].y,
						 hwnd, HWND_TOP, PB_CONVERT, NULL, NULL);

	enable(hwnd,PB_CONVERT);
	disable(hwnd,PB_STOP);
	disable(hwnd,PB_CANCEL);

	color = SYSCLR_FIELDBACKGROUND;
	WinSetPresParam(hwnd, PP_BACKGROUNDCOLORINDEX, sizeof(color), &color);

   if(isWarpSans())
		font = "9.WarpSans";
	else
		font = "8.Helv";

	WinSetPresParam(hwnd, PP_FONTNAMESIZE, strlen(font)+1, font);
	for(i = 0; i < PAGE_COUNT; i++)
		WinSetPresParam(nbpage[i].hwnd, PP_FONTNAMESIZE, strlen(font)+1, font);

	return 0;
}

MRESULT EXPENTRY wpMain(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
	switch (msg)
	{
		case WM_CREATE:
			return processInitdlg(hwnd,mp1,mp2);
		case WM_COMMAND:
			return processCommand(hwnd,mp1,mp2);
		case WM_CONTROL:
			return processControl(hwnd,mp1,mp2);
		case WM_ERASEBACKGROUND:
			WinFillRect(LONGFROMMP(mp1), (RECTL*) PVOIDFROMMP(mp2), SYSCLR_FIELDBACKGROUND);
			return (MRESULT) FALSE;
#if 0
		case WM_ERASEBACKGROUND:
		{
			RECTL *rect = PVOIDFROMMP(mp2);
			POINTL pos;
			HBITMAP hbm;
			BITMAPINFOHEADER info;

			hbm = GpiLoadBitmap(HWNDFROMMP(mp1),0,BM_BACKGROUND,0,0);
			info.cbFix = sizeof(info);
			if(GpiQueryBitmapParameters(hbm,&info))
			{
				pos.x = 0;
				while(pos.x < rect->xRight)
				{
					pos.y = 0;
					while(pos.y < rect->yTop)
					{
						WinDrawBitmap(HWNDFROMMP(mp1), hbm, NULL, &pos, 0, 0, DBM_NORMAL);
						pos.y += info.cy;
					}
					pos.x += info.cx;
				}
			}
			return 0;
		}
#endif
		case WM_PAINT:
			WinDefWindowProc(hwnd,msg,mp1,mp2);
/* ?? */
			return 0;

		case WM_SIZE:

				// Size the notebook with the client window

				WinSetWindowPos( WinWindowFromID( hwnd, FID_CLIENT ), 0, 0, NBy,
									  SHORT1FROMMP( mp2 ), SHORT2FROMMP( mp2 ) - NBy,
									  SWP_SIZE | SWP_SHOW | SWP_MOVE);

				break;

		case WM_CHAR:
			doControlNavigation(hwnd,mp1,mp2);
			break;

		case WM_APPTERMINATENOTIFY:
			doConversion(LONGFROMMP(mp1), LONGFROMMP(mp2), hwnd);
			return 0;

		case WM_CLOSE:
			{
				int i;
				for(i = PAGE_COUNT-1; i >= 0; i--)
					WinSendMsg(nbpage[i].hwnd,msg,mp1,mp2);
				WinPostMsg(hwnd,WM_QUIT,0,0);
				return 0;
			}
	}

	return WinDefWindowProc(hwnd,msg,mp1,mp2);
}

HWND workerhwnd;
TID workertid;

MRESULT EXPENTRY wpWorker(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
	switch(msg)
	{
		/* from CD dialog */
		case DLGCD_REFRESH:
		{
			enable(mainHwnd,PB_CANCEL);

			HWND dlgcdhwnd = (HWND) mp1;
			dlgcd_refresh(mp1,(void**)&cddbSocket);
			WinPostMsg(dlgcdhwnd, DLGCD_REFRESH, mp1, mp2);

			cddbSocket = NULL;
			disable(mainHwnd,PB_CANCEL);
			return 0;
		}

		/* from CDDB dialog */
		case DLGCDDB_UPDATE:
		{
			enable(mainHwnd,PB_CANCEL);

			HWND dlgcddbhwnd = (HWND) mp1;
			dlgcddb_update(mp1,(void**)&cddbSocket);
			WinPostMsg(dlgcddbhwnd, DLGCDDB_UPDATE, mp1, mp2);

			cddbSocket = NULL;
			disable(mainHwnd,PB_CANCEL);
			return 0;
		}
	}

	return WinDefWindowProc(hwnd,msg,mp1,mp2);
}


void _Optlink workerThread(void *param)
{
	HMQ hmq;
	HAB hab;

	if(initPM(&hab, &hmq))
	{
		WinRegisterClass(hab, "Worker", wpWorker, 0, 0 );

		workerhwnd = WinCreateWindow(HWND_OBJECT,"Worker", NULL, 0, 0, 0, 0, 0, NULL,
									  HWND_BOTTOM, 0, NULL, NULL);

		runPM(hab);

		closePM(hab,hmq);
	}
}


int main(int argc, char *argv[])
{
	ULONG frameFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_MINBUTTON |/* FCF_MAXBUTTON | */
							 FCF_SIZEBORDER  |/* FCF_MENU | FCF_ACCELTABLE | */ FCF_ICON |
						 /* FCF_SHELLPOSITION | */ FCF_TASKLIST /*| FCF_DLGBORDER*/;

   setvbuf(stdout,NULL,_IONBF, 0);

	if(initPM(&mainhab, &mainhmq) &&
		(frameHwnd = createClientFrame(mainhab, &mainHwnd, "CD2MP3 PM", &frameFlags,
												 wpMain, "CD2MP3 PM")))
	{
		if(!loadPosition(frameHwnd, INIFILE))
		{
         POINTL size = {350, 229};
			WinMapDlgPoints(HWND_DESKTOP, &size, 1, TRUE);

			WinSetWindowPos( frameHwnd, NULLHANDLE,
								  30, 30, size.x, size.y,
								  SWP_SIZE | SWP_MOVE | SWP_SHOW);
		}

		workertid = _beginthread(workerThread,0,64*1024,(void *) NULL);

		runPM(mainhab);

		savePosition(frameHwnd, INIFILE);

		closePM(mainhab, mainhmq);

		return 0;
	}

	return 1;
}

/* for external use */
int bitRateChanged = 0;
USHORT getBitRate(void)
{
	char buffer[8];
	getText(WinWindowFromID(nbpage[PAGE_MP3].hwnd, CB_BITRATE), CBID_EDIT, buffer, sizeof(buffer));
	return atoi(buffer);
}

ULONG getNextCDDBServer(char *server, ULONG size, SHORT *index)
{
	if(getCheck(nbpage[PAGE_CDDB].hwnd, CB_USEHTTP))
	{
		if(getCheck(nbpage[PAGE_CDDB].hwnd, CB_TRYALL))
		{
			(*index)++;
			if(*index < getItemCount(nbpage[PAGE_CDDB].hwnd, LB_HTTPCDDBSERVERS))
			{
				getItemText(nbpage[PAGE_CDDB].hwnd, LB_HTTPCDDBSERVERS, *index, server, size);
				return HTTP;
			}
			else
				return NOSERVER;
		}
		else
		{
			*index = getSelectItem(nbpage[PAGE_CDDB].hwnd, LB_HTTPCDDBSERVERS, *index);
			if(*index != LIT_NONE)
			{
				getItemText(nbpage[PAGE_CDDB].hwnd, LB_HTTPCDDBSERVERS, *index, server, size);
				return HTTP;
			}
			else
				return NOSERVER;
		}
	}
	else
	{
		if(getCheck(nbpage[PAGE_CDDB].hwnd, CB_TRYALL))
		{
			(*index)++;
			if(*index < getItemCount(nbpage[PAGE_CDDB].hwnd, LB_CDDBSERVERS))
			{
				getItemText(nbpage[PAGE_CDDB].hwnd, LB_CDDBSERVERS, *index, server, size);
				return CDDB;
			}
			else
				return NOSERVER;
		}
		else
		{
			*index = getSelectItem(nbpage[PAGE_CDDB].hwnd, LB_CDDBSERVERS, *index);
			if(*index != LIT_NONE)
			{
				getItemText(nbpage[PAGE_CDDB].hwnd, LB_CDDBSERVERS, *index, server, size);
				return CDDB;
			}
			else
				return NOSERVER;
		}
	}
}

ULONG getFilenameFormat(char *buffer, LONG size)
{
   return getText(nbpage[PAGE_CDDB].hwnd, EF_FORMAT, buffer, size);
}

ULONG getFallbackFF(char *buffer, LONG size)
{
   return getText(nbpage[PAGE_CDDB].hwnd, EF_FALLBACKFF, buffer, size);
}


ULONG getUserHost(char *user, int sizeuser, char *host, int sizehost)
{
   char buffer[128];
   char *hostpos;
   ULONG rc = getText(nbpage[PAGE_CDDB].hwnd, EF_EMAIL, buffer, sizeof(buffer));
   if(rc)
   {
      hostpos = strchr(buffer,'@');
      *hostpos++ = 0;

      strncpy(user,buffer,sizeuser);
      user[sizeuser-1] = 0;
      strncpy(host,hostpos,sizehost);
      host[sizehost-1] = 0;
   }
   return rc;
}

ULONG getProxy(char *proxy, int size)
{
   return getText(nbpage[PAGE_CDDB].hwnd, EF_PROXY, proxy, size);
}

