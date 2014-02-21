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

#include "miscsam.h"
#include "readcd.h"
#include "tcpipsock.h"
#include "http.h"
#include "cddb.h"
static PFNWP wpCT;

#define NUM_FIELDS 13
FIELDINFO *fieldInfo[NUM_FIELDS];

/* for multiple match dialog */
struct FUZZYMATCHCREATEPARAMS
{
   CDDBQUERY_DATA *matches, *chosen;
};

char *formatFilename(char *out, int size, char *format, CDTRACKRECORD *record, HWND hwnd);

/* keeps a list of loaded encoders and grabbers, and their settings */
GRABBER *grabbers;
int countGrabbers;

/* container icons */
static HPOINTER dataIco, trackIco;

/* for WM_ENDMENU */
static removeSourceEmphasisInfo sourceEmphasisInfo;
static BOOL miniIcons = 0;

static BOOL loadConfig(char *file, GRABBER **grabbers, int *countGrabbers)
{
	FILE *config;
	config = fopen(file,"r");

	if(config)
	{
		char buffer[512], param[256], value[256], *comment;
		int e;

		e = 0;
		while(fgets(buffer,sizeof(buffer),config))
		{
			uncomment(buffer);
			sscanf(buffer,"%[^=]=%[^=]",param,value);
			strip(param);

			if(!stricmp(param,"grabberid")) e++;
		}

		*grabbers = (GRABBER *) calloc(e, sizeof(GRABBER));

		rewind(config);

		e = -1;
		while(fgets(buffer,sizeof(buffer),config))
		{
			uncomment(buffer);
			sscanf(buffer,"%[^=]=%[^=]",param,value);
			strip(param);
			strip(value);

			if(!stricmp(param,"grabberid"))
			{
				e++;
				strcpy((*grabbers)[e].id, value);
			}
			else if(e >=0)
			{
				if(!stricmp(param,"grabberexe"))
					strcpy((*grabbers)[e].exe, value);
				else if(!stricmp(param,"grabbertype"))
				{
					if(!stricmp(value,"win32os2"))
						(*grabbers)[e].type = WIN32OS2;
					else if(!stricmp(value,"vio"))
						(*grabbers)[e].type = VIO;
					else if(!stricmp(value,"vdm"))
						(*grabbers)[e].type = VDM;
				}
				else if(!stricmp(param,"grabberin"))
					strcpy((*grabbers)[e].input, value);
				else if(!stricmp(param,"grabberout"))
					strcpy((*grabbers)[e].output, value);
				else if(!stricmp(param,"grabbermangling"))
					strcpy((*grabbers)[e].mangling, value);
				else if(!stricmp(param,"grabberbefore"))
					strcpy((*grabbers)[e].before, value);
				else if(!stricmp(param,"grabberafter"))
					strcpy((*grabbers)[e].after, value);
				else if(!stricmp(param,"grabberacceptsoutput"))
				{
					if(!stricmp(value,"true"))
						(*grabbers)[e].acceptsOutput = TRUE;
					else
						(*grabbers)[e].acceptsOutput = FALSE;
				}
				else if(!stricmp(param,"grabbernodrive"))
				{
					if(!stricmp(value,"true"))
						(*grabbers)[e].noDrive = TRUE;
					else
						(*grabbers)[e].noDrive = FALSE;
				}
			}
		}
		*countGrabbers = e+1;

		fclose(config);
		return TRUE;
	}
	return FALSE;
}

static void loadIni(HWND hwnd)
{
	char buffer[512];
	CNRINFO cnrInfo;
	HINI inifile;

	inifile = openProfile(INIFILE);

	if(inifile)
	{
		if(readProfile(inifile,"CD","Grabber",buffer,sizeof(buffer), TRUE))
			setText(WinWindowFromID(hwnd, CB_GRABBER), CBID_EDIT, buffer);
		if(readProfile(inifile,"CD","Drive",buffer,sizeof(buffer), TRUE))
			setText(WinWindowFromID(hwnd, CB_DRIVE), CBID_EDIT, buffer);
		if(readProfile(inifile,"CD","TempDir",buffer,sizeof(buffer), TRUE))
			setText(hwnd, EF_TEMPDIR, buffer);
		if(readProfile(inifile,"CD","Custom",buffer,sizeof(buffer), TRUE))
			setText(hwnd, EF_CUSTOMCD, buffer);

		if(readProfile(inifile,"CD","SuperGrab",buffer,1, FALSE))
			setCheck(hwnd, CB_SUPERGRAB, *buffer);
		if(readProfile(inifile,"CD","Use CDDB",buffer,1, FALSE))
			setCheck(hwnd, CB_USECDDB, *buffer);

		if(readProfile(inifile,"CD","Title",buffer,sizeof(buffer), TRUE))
			setText(hwnd, EF_TITLE, buffer);
		if(readProfile(inifile,"CD","Artist",buffer,sizeof(buffer), TRUE))
			setText(hwnd, EF_ARTIST, buffer);
		if(readProfile(inifile,"CD","Comments",buffer,sizeof(buffer), TRUE))
			setText(hwnd, EF_COMMENTS, buffer);
		if(readProfile(inifile,"CD","Category",buffer,sizeof(buffer), TRUE))
			setText(hwnd, EF_GENRE, buffer);

		if(readProfile(inifile,"CD","ContainerInfo",&cnrInfo,sizeof(cnrInfo), FALSE))
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_SETCNRINFO, MPFROMP(&cnrInfo),
				MPFROMLONG(CMA_XVERTSPLITBAR | CMA_FLWINDOWATTR));

		miniIcons = (cnrInfo.flWindowAttr & CV_MINI) ? TRUE : FALSE;

		closeProfile(inifile);
	}
}

static void saveIni(HWND hwnd)
{
	char buffer[512];
	CNRINFO cnrInfo;
	HINI inifile;

	inifile = openProfile(INIFILE);

	if(inifile)
	{
		getText(WinWindowFromID(hwnd, CB_GRABBER), CBID_EDIT, buffer, sizeof(buffer));
		writeProfile(inifile,"CD","Grabber", buffer,0);
		getText(WinWindowFromID(hwnd, CB_DRIVE), CBID_EDIT, buffer, sizeof(buffer));
		writeProfile(inifile,"CD","Drive", buffer,0);
		getText(hwnd, EF_TEMPDIR, buffer, sizeof(buffer));
		writeProfile(inifile,"CD","TempDir", buffer,0);
		getText(hwnd, EF_CUSTOMCD, buffer, sizeof(buffer));
		writeProfile(inifile,"CD","Custom", buffer,0);

		*buffer = getCheck(hwnd, CB_SUPERGRAB);
		writeProfile(inifile,"CD","SuperGrab", buffer, 1);
		*buffer = getCheck(hwnd, CB_USECDDB);
		writeProfile(inifile,"CD","Use CDDB", buffer, 1);

		getText(hwnd, EF_TITLE, buffer, sizeof(buffer));
		writeProfile(inifile,"CD","Title", buffer,0);
		getText(hwnd, EF_ARTIST, buffer, sizeof(buffer));
		writeProfile(inifile,"CD","Artist", buffer,0);
		getText(hwnd, EF_COMMENTS, buffer, sizeof(buffer));
		writeProfile(inifile,"CD","Comments", buffer,0);
		getText(hwnd, EF_GENRE, buffer, sizeof(buffer));
		writeProfile(inifile,"CD","Category", buffer,0);

		cnrInfo.cb = sizeof(cnrInfo);
		WinSendDlgItemMsg(hwnd,CT_TRACK,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
		writeProfile(inifile,"CD","ContainerInfo", &cnrInfo, sizeof(cnrInfo));

		closeProfile(inifile);
	}
}


/* refresh MP3 size with new bitrate */
static void refreshFieldInfo(HWND hwnd, LONG id)
{
	CDTRACKRECORD *record;
	USHORT bitrate = getBitRate();

	record = (CDTRACKRECORD *) enumRecords(hwnd, id, NULL, CMA_FIRST);
	while(record && record != (CDTRACKRECORD *) -1)
	{
		if(!bitrate)
			record->mp3size = 0;
		else if(!record->data)
			record->mp3size = record->size /
				((record->channels*2*44100) / ((double) getBitRate() * 1000/8)) + 0.5;
		record = (CDTRACKRECORD *) enumRecords(hwnd, id, (RECORDCORE *) record, CMA_NEXT);
	}

	WinSendDlgItemMsg(hwnd, id, CM_INVALIDATEDETAILFIELDINFO,0,0);
}


static MRESULT processControl(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
	switch(SHORT2FROMMP(mp1))
	{
		case CN_CONTEXTMENU:
		{
			RECORDCORE *record = (RECORDCORE *) PVOIDFROMMP(mp2);

			processPopUp(hwnd, CT_TRACK, record,
				PUM_CDRECORD, PUM_CDCONTAINER, &sourceEmphasisInfo);

			WinSendMsg(sourceEmphasisInfo.PUMHwnd, MM_SETITEMATTR,
				MPFROM2SHORT(IDM_MINIICONS, FALSE),
				MPFROM2SHORT(MIA_CHECKED, miniIcons ? MIA_CHECKED : 0));

			return 0;
		}

		case CN_BEGINEDIT: break;

		case CN_REALLOCPSZ:
		{
			CNREDITDATA *data = (CNREDITDATA *) mp2;
			CDTRACKRECORD *posRecord = (CDTRACKRECORD *) data->pRecord;

			if(data->pFieldInfo->offStruct == FIELDOFFSET(RECORDCORE,pszIcon))
			{
				posRecord->record.pszIcon = posRecord->record.pszText =
				posRecord->record.pszName = posRecord->record.pszTree =
					(char*) realloc(posRecord->record.pszIcon,data->cbText);
				return (MRESULT) TRUE;
			}

			else if(data->pFieldInfo->offStruct == FIELDOFFSET(CDTRACKRECORD,titleptr))
				{if(data->cbText <= sizeof(posRecord->title)) return (MRESULT) TRUE;}
			else if(data->pFieldInfo->offStruct == FIELDOFFSET(CDTRACKRECORD,artistptr))
				{if(data->cbText <= sizeof(posRecord->artist)) return (MRESULT) TRUE;}
			else if(data->pFieldInfo->offStruct == FIELDOFFSET(CDTRACKRECORD,albumptr))
				{if(data->cbText <= sizeof(posRecord->album)) return (MRESULT) TRUE;}
			else if(data->pFieldInfo->offStruct == FIELDOFFSET(CDTRACKRECORD,yearptr))
				{if(data->cbText <= sizeof(posRecord->year)) return (MRESULT) TRUE;}
			else if(data->pFieldInfo->offStruct == FIELDOFFSET(CDTRACKRECORD,genreptr))
				{if(data->cbText <= sizeof(posRecord->genre)) return (MRESULT) TRUE;}
			else if(data->pFieldInfo->offStruct == FIELDOFFSET(CDTRACKRECORD,commentptr))
				{if(data->cbText <= sizeof(posRecord->comment)) return (MRESULT) TRUE;}

			return 0;
		}

		/* rebuilds the filename */
		case CN_ENDEDIT:
		{
			CNREDITDATA *data = (CNREDITDATA *) mp2;
			CDTRACKRECORD *posRecord = (CDTRACKRECORD *) data->pRecord;

         if(data->pFieldInfo->offStruct == FIELDOFFSET(RECORDCORE,pszIcon))
         {
            makeValidLFN(posRecord->record.pszIcon);
         }
         else
			{
				char buffer2[256];
				char buffer[256];

				getFilenameFormat(buffer2,sizeof(buffer2));
				formatFilename(buffer,sizeof(buffer),buffer2,posRecord,hwnd);

				posRecord->record.pszIcon = posRecord->record.pszText =
				posRecord->record.pszName = posRecord->record.pszTree =
					(char*) realloc(posRecord->record.pszIcon,strlen(buffer)+1);
            strcpy(posRecord->record.pszIcon,makeValidLFN(buffer));
			}
			return 0;
		}

	}

	return 0;
}


static MRESULT processCommand(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
	switch(SHORT1FROMMP(mp1))
	{
		case IDM_ICONVIEW:
		{
			CNRINFO cnrInfo;
			cnrInfo.cb = sizeof(cnrInfo);
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
			cnrInfo.flWindowAttr |= CV_ICON;
			cnrInfo.flWindowAttr &= ~CV_NAME & ~CV_DETAIL & ~CV_TREE;
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_SETCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
			return 0;
		}
		case IDM_NAMEVIEW:
		{
			CNRINFO cnrInfo;
			cnrInfo.cb = sizeof(cnrInfo);
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
			cnrInfo.flWindowAttr |= CV_NAME | CV_FLOW;
			cnrInfo.flWindowAttr &= ~CV_ICON & ~CV_DETAIL & ~CV_TREE;
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_SETCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
			return 0;
		}
		case IDM_DETAILVIEW:
		{
			CNRINFO cnrInfo;
			cnrInfo.cb = sizeof(cnrInfo);
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
			cnrInfo.flWindowAttr |= CV_DETAIL | CA_DETAILSVIEWTITLES;
			cnrInfo.flWindowAttr &= ~CV_ICON & ~CV_NAME & ~CV_TREE;
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_SETCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
			return 0;
		}
		case IDM_MINIICONS:
		{
			CNRINFO cnrInfo;
			miniIcons = !miniIcons;

			cnrInfo.cb = sizeof(cnrInfo);
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
			cnrInfo.flWindowAttr = (miniIcons) ? (cnrInfo.flWindowAttr | CV_MINI) : (cnrInfo.flWindowAttr & ~CV_MINI);
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_SETCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
			return 0;
		}

		case IDM_SELECTALL:
			selectAllRecords(hwnd, CT_TRACK, TRUE);
			return 0;
		case IDM_DESELECTALL:
			selectAllRecords(hwnd, CT_TRACK, FALSE);
			return 0;

		case PB_CDPLAY:
			sourceEmphasisInfo.sourceRecord =
				searchRecords(hwnd, CT_TRACK, (RECORDCORE *) CMA_FIRST, CRA_CURSORED);
		case IDM_PLAY:
		{
			CDTRACKRECORD *record;
			char buffer[512], drive[4];
			CD_drive cdDrive;
			record = (CDTRACKRECORD *) sourceEmphasisInfo.sourceRecord;

			if(record)
			{
				getText(WinWindowFromID(hwnd, CB_DRIVE), CBID_EDIT, buffer, sizeof(buffer));
				drive[0] = buffer[0];
				drive[1] = ':';
				drive[2] = 0;

				if(cdDrive.open(drive))
				{
					cdDrive.stop();
					cdDrive.readCDInfo();
					cdDrive.play(record->track);
					cdDrive.close();
				}
			}
			return 0;
		}

		case PB_CDSTOP:
		case IDM_STOP:
		{
			char buffer[512], drive[4];
			CD_drive cdDrive;

			getText(WinWindowFromID(hwnd, CB_DRIVE), CBID_EDIT, buffer, sizeof(buffer));
			drive[0] = buffer[0];
			drive[1] = ':';
			drive[2] = 0;

			if(cdDrive.open(drive))
			{
				cdDrive.stop();
				cdDrive.close();
			}
			return 0;
		}

		case PB_REFRESH:
		case IDM_REFRESH:
			WinPostMsg(workerhwnd,DLGCD_REFRESH,(void *) hwnd, NULL);
			return 0;

	}
	return 0;
}


MRESULT EXPENTRY wpMatch(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
   static CDDBQUERY_DATA *chosen; /* somebody find me a better way to do this */

	switch(msg)
	{
		case WM_INITDLG:
		{
			HWND focusHwnd = (HWND) mp1;
         FUZZYMATCHCREATEPARAMS *data = (FUZZYMATCHCREATEPARAMS *) mp2;
			int i = 0;

			WinSetFocus(HWND_DESKTOP, focusHwnd);

			while(data->matches[i].discid)
			{
            int inserted = insertItemText(hwnd,LB_MATCHES,LIT_END,data->matches[i].title);
				setItemHandle(hwnd, LB_MATCHES,inserted,&data->matches[i]);
				i++;
			}
			chosen = data->chosen;
         memset(chosen,0,sizeof(*chosen));
         return 0;
		}

		case WM_COMMAND:
		{
			switch(SHORT1FROMMP(mp1))
			{
            case DID_OK:
				{
					int selected = getSelectItem(hwnd, LB_MATCHES, LIT_FIRST);
               if(selected != LIT_NONE)
                  memcpy(chosen,getItemHandle(hwnd,LB_MATCHES,selected),sizeof(*chosen));
					WinPostMsg(hwnd,WM_CLOSE,0,0);
				}
            case DID_CANCEL:
					WinPostMsg(hwnd,WM_CLOSE,0,0);
			}
			return 0;
		}
	}
	return WinDefDlgProc(hwnd,msg,mp1,mp2);
}


static CD_drive	 *cdDrive = NULL;
static CDDB_socket *cddbSocket = NULL;
static CDDBQUERY_DATA queryData;


void dlgcd_refresh(void *mp1, void **mp2)
{
	int i;
	char drive[4];
	HWND hwnd = (HWND) mp1;

	cdDrive = new CD_drive;

	getText(WinWindowFromID(hwnd, CB_DRIVE), CBID_EDIT, drive, sizeof(drive));
	drive[1] = ':';
	drive[2] = 0;

	/* find info on CD tracks */
   if(!cdDrive->open(drive) ||
      !cdDrive->readCDInfo() ||
      !cdDrive->fillTrackInfo() ||
      !cdDrive->close()
     )
   {
      delete cdDrive;
      cdDrive = NULL;
      return;
   }

	if(getCheck(hwnd, CB_USECDDB))
	{
		char server[512];
		BOOL success = FALSE;
		ULONG serverType;
		SHORT index = LIT_FIRST;

		cddbSocket = new CDDB_socket;
		*mp2 = cddbSocket; /* to be able to cancel */

		do
		{
			serverType = getNextCDDBServer(server,sizeof(server),&index);
			if(serverType != NOSERVER)
			{
				if(serverType == CDDB)
				{
					updateStatus("Contacting: %s", server);

					char host[512]=""; int port=8880;
					sscanf(server,"%[^:\n\r]:%d",host,&port);
					if(host[0])
						cddbSocket->tcpip_socket::connect(host,port);
				}
				else
				{
               char host[512], path[512], proxy[512], *slash = NULL, *http = NULL;

               http = strstr(server,"http://");
               if(http)
                  slash = strchr(http+7,'/');
					if(slash)
					{
						strncpy(host,server,slash-server);
						strcpy(path,slash);
                  getProxy(proxy,sizeof(proxy));
                  if(*proxy)
                     updateStatus("Contacting: %s", proxy);
                  else
                     updateStatus("Contacting: %s", server);
                  cddbSocket->connect(host,proxy,path);
					}
               else
                  updateError("Invalid URL: %s", server);
            }

				if(!cddbSocket->isOnline())
				{
					if(cddbSocket->getSocketError() == SOCEINTR)
						goto end;
					else
						continue;
				}
				updateStatus("Looking at CDDB Server Banner at %s", server);
				if(!cddbSocket->banner_req())
				{
					if(cddbSocket->getSocketError() == SOCEINTR)
						goto end;
					else
						continue;
				}
				updateStatus("Handshaking with CDDB Server %s", server);
				char user[128]="",host[128]="";
				getUserHost(user,sizeof(user),host,sizeof(host));
				if(!cddbSocket->handshake_req(user,host))
				{
					if(cddbSocket->getSocketError() == SOCEINTR)
						goto end;
					else
						continue;
				}

				updateStatus("Looking for CD information on %s", server);
            int rc = cddbSocket->query_req(cdDrive, &queryData);
				if(!rc)
				{
					if(cddbSocket->getSocketError() == SOCEINTR)
						goto end;
					else
						continue;
				}
				else if(rc == -1)
					continue; /* no match for this server */
				else if(rc == COMMAND_MORE)
				{
					CDDBQUERY_DATA *matches = (CDDBQUERY_DATA *) malloc(sizeof(CDDBQUERY_DATA));
               int i = 0;

               while(cddbSocket->get_query_req(&matches[i]))
					{
						i++;
                  matches = (CDDBQUERY_DATA *) realloc(matches, (i+1)*sizeof(CDDBQUERY_DATA));
					}
               memset(&matches[i],0,sizeof(matches[i]));
					/* don't try to open a window in this thread */
               WinSendMsg(hwnd,CDDB_FUZZYMATCH,matches,&queryData);
					free(matches);
				}
				/* else there is only one match */

            if(queryData.discid)
            {
               updateStatus("Requesting CD information from %s", server);
               if(!cddbSocket->read_req(queryData.category, queryData.discid))
               {
                  if(cddbSocket->getSocketError() == SOCEINTR)
                     goto end;
                  else
                     continue;
               }
               else
                  success = TRUE;
            }
			}
		} while(serverType != NOSERVER && !success);

		end:

		if(!success)
		{
			delete cddbSocket;
			cddbSocket = NULL;
		}

		updateStatus("");
	}

	return;
}

char *formatFilename(char *out, int size, char *format, CDTRACKRECORD *record, HWND hwnd)
{
	char buffer[256];
	char *inpos = format;
	char *outpos = out;

	while(*inpos && (outpos-out)+1 < size)
	{
		if(*inpos == '%')
		{
			inpos++;
			switch(*inpos)
			{
				case 't':
					strncpy(outpos, record->title, size-(outpos-out)-1);
					outpos += strlen(record->title); break;
            case 'a':
               strncpy(outpos, record->artist, size-(outpos-out)-1);
               outpos += strlen(record->artist); break;
				case 'l':
               strncpy(outpos, record->album, size-(outpos-out)-1);
               outpos += strlen(record->album); break;
            case 'y':
               strncpy(outpos, record->year, size-(outpos-out)-1);
               outpos += strlen(record->year); break;
				case 'c':
					strncpy(outpos, record->comment, size-(outpos-out)-1);
					outpos += strlen(record->comment); break;
            case 'g':
               strncpy(outpos, record->genre, size-(outpos-out)-1);
               outpos += strlen(record->genre); break;

				case 'k':
					if(size-(outpos-out)+1 > 2)
					{
						sprintf(outpos,"%.2d",record->track);
						outpos += strlen(outpos);
					}
					break;
				case 'C':
					getText(hwnd,EF_COMMENTS,buffer,sizeof(buffer));
					strncpy(outpos, buffer, size-(outpos-out)-1);
					outpos += strlen(buffer); break;

				case 'm':
					_itoa(record->minutes,buffer,10);
					strncpy(outpos, buffer, size-(outpos-out)-1);
					outpos += strlen(buffer); break;
				case 's':
					_itoa(record->seconds,buffer,10);
					if(strlen(buffer) < 2)
						{ buffer[2] = 0; buffer[1] = buffer[0]; buffer[0] = '0'; }
					strncpy(outpos, buffer, size-(outpos-out)-1);
					outpos += strlen(buffer); break;
				case 'S':
					_itoa(record->minutes*60+record->seconds,buffer,10);
					strncpy(outpos, buffer, size-(outpos-out)-1);
					outpos += strlen(buffer); break;

				/* double % */
				case '%':
					*outpos = *inpos;
					outpos++;
					break;

				/* unknown tags, leave spot empty */
				default: break;
			}
		}
		else
		{
			*outpos = *inpos;
			outpos++;
		}
		inpos++;
	}

	*outpos = 0;
	return out;
}

void dlgcd_refresh2(void *mp1, void *mp2)
{
	int i;
	HWND hwnd = (HWND) mp1;
	CDTRACKRECORD *record;
	CDTRACKRECORD *firstRecord, *posRecord;
	RECORDINSERT recordInfo;
	char buffer[256];
	char buffer2[256];
	USHORT bitrate = getBitRate();

   if(!cdDrive)
      return;

	if(cddbSocket)
	{
		setText(hwnd,EF_ARTIST,cddbSocket->get_disc_title(0));
		setText(hwnd,EF_TITLE,cddbSocket->get_disc_title(1));
		setText(hwnd,EF_COMMENTS,cddbSocket->get_disc_title(2));
		setText(hwnd,EF_GENRE,queryData.category);
	}

	/* delete all current records */
	record = (CDTRACKRECORD *) enumRecords(hwnd, CT_TRACK, NULL, CMA_FIRST);
	while(record && record != (CDTRACKRECORD *) -1)
	{
		free(record->record.pszIcon);
		record = (CDTRACKRECORD *) enumRecords(hwnd, CT_TRACK, (RECORDCORE *) record, CMA_NEXT);
	}

	removeRecords(hwnd, CT_TRACK, NULL, 0);

	/* allocate and fill container with CD Track Info */
	firstRecord = posRecord = (CDTRACKRECORD *)
		allocaRecords(hwnd, CT_TRACK, cdDrive->getCount(), sizeof(CDTRACKRECORD) - sizeof(RECORDCORE));

	for (i = 0; i < cdDrive->getCount(); i++)
	{
      UCHAR track = cdDrive->getCDInfo()->firstTrack+i;

		/* setting kludges and other weird things */
		posRecord->timepointer = posRecord->time;
		posRecord->typepointer = posRecord->type;
		posRecord->titleptr = posRecord->title;
		posRecord->artistptr = posRecord->artist;
		posRecord->albumptr = posRecord->album;
		posRecord->yearptr = posRecord->year;
		posRecord->genreptr = posRecord->genre;
		posRecord->commentptr = posRecord->comment;
		posRecord->record.hptrMiniIcon = 0; /* ask God */

		/* loading info fields */

      if(cdDrive->getTrackInfo(track)->data)
		{
			strcat(posRecord->time,"N/A");
			strcat(posRecord->type,"Data");
			posRecord->mp3size = 0;
			posRecord->record.hptrIcon = dataIco;
		}
		else
		{
         posRecord->minutes = cdDrive->getTrackInfo(track)->length.minute;
         posRecord->seconds = cdDrive->getTrackInfo(track)->length.second;

         sprintf(posRecord->time,"%d:%.2d",cdDrive->getTrackInfo(track)->length.minute,
                                           cdDrive->getTrackInfo(track)->length.second);
			strcat(posRecord->type,"Audio");
			if(bitrate)
            posRecord->mp3size = cdDrive->getTrackInfo(track)->size /
                  ((cdDrive->getTrackInfo(track)->channels*2*44100) /
						((double) bitrate * 1000/8)) + 0.5;
			else
				posRecord->mp3size = 0;
			posRecord->record.hptrIcon = trackIco;
		}

      posRecord->track = cdDrive->getTrackInfo(track)->number;
      posRecord->size = cdDrive->getTrackInfo(track)->size;
      posRecord->channels = cdDrive->getTrackInfo(track)->channels;
      posRecord->data = cdDrive->getTrackInfo(track)->data;

		if(cddbSocket)
		{
			strncpy(posRecord->title, cddbSocket->get_track_title(i,0), sizeof(posRecord->title)-1);
			strncpy(posRecord->artist, cddbSocket->get_disc_title(0), sizeof(posRecord->artist)-1);
			strncpy(posRecord->album, cddbSocket->get_disc_title(1), sizeof(posRecord->album)-1);
			strncpy(posRecord->comment, cddbSocket->get_track_title(i,1), sizeof(posRecord->comment)-1);
			strncpy(posRecord->genre, queryData.category, sizeof(posRecord->genre)-1);
// 		  cddbSocket->get_disc_title(2)

			getFilenameFormat(buffer2,sizeof(buffer2));
		}
		else
      {
         getText(hwnd,EF_ARTIST,buffer,sizeof(buffer));
         strncpy(posRecord->artist, buffer, sizeof(posRecord->artist)-1);
         getText(hwnd,EF_TITLE,buffer,sizeof(buffer));
         strncpy(posRecord->album, buffer, sizeof(posRecord->album)-1);
         getText(hwnd,EF_COMMENTS,buffer,sizeof(buffer));
         strncpy(posRecord->comment, buffer, sizeof(posRecord->comment)-1);
         getText(hwnd,EF_GENRE,buffer,sizeof(buffer));
         strncpy(posRecord->genre, buffer, sizeof(posRecord->genre)-1);

         getFallbackFF(buffer2, sizeof(buffer2));
      }
		formatFilename(buffer,sizeof(buffer),buffer2,posRecord,hwnd);

		posRecord->record.pszIcon = posRecord->record.pszText =
		posRecord->record.pszName = posRecord->record.pszTree =
			(char*) malloc(strlen(buffer)+1);
      strcpy(posRecord->record.pszIcon,makeValidLFN(buffer));

		/* "next!" */
		posRecord = (CDTRACKRECORD *) posRecord->record.preccNextRecord;
	}

	recordInfo.cb = sizeof(recordInfo);
	recordInfo.pRecordOrder = (RECORDCORE *) CMA_END;
	recordInfo.pRecordParent = NULL;
	recordInfo.fInvalidateRecord = TRUE;
	recordInfo.zOrder = CMA_TOP;
	recordInfo.cRecordsInsert = cdDrive->getCount();

	insertRecords(hwnd, CT_TRACK, (RECORDCORE *) firstRecord, &recordInfo);

	delete cdDrive; cdDrive = NULL;
	delete cddbSocket; cddbSocket = NULL;
}




MRESULT EXPENTRY wpCTTrack(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
	switch(msg)
	{
		case WM_CHAR:
		{
			SHORT fsflags = SHORT1FROMMP(mp1);
			SHORT usvkey = SHORT2FROMMP(mp2);
			if(fsflags & KC_KEYUP) break;

			if((fsflags & KC_SHIFT) && (usvkey == VK_F10))
			{
				RECORDCORE *record;
				record = searchRecords(hwnd, 0, (RECORDCORE *) CMA_FIRST, CRA_SELECTED);

				processPopUp(WinQueryWindow(hwnd, QW_PARENT), WinQueryWindowUShort(hwnd,QWS_ID),
					record, PUM_CDRECORD, PUM_CDCONTAINER, &sourceEmphasisInfo);

				WinSendMsg(sourceEmphasisInfo.PUMHwnd, MM_SETITEMATTR,
					MPFROM2SHORT(IDM_MINIICONS, FALSE),
					MPFROM2SHORT(MIA_CHECKED, miniIcons ? MIA_CHECKED : 0));
				return 0;
			}

			else if((fsflags & KC_SHIFT) && (usvkey == VK_F9))
			{
				CNREDITDATA editdata;
				CNRINFO cnrInfo;

				WinSendMsg(hwnd,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));

				editdata.cb = sizeof(editdata);
				editdata.hwndCnr = hwnd;
				editdata.pRecord = searchRecords(hwnd, 0, (RECORDCORE *) CMA_FIRST, CRA_CURSORED);
				editdata.ppszText = NULL;
				editdata.cbText = 0;

				if(cnrInfo.flWindowAttr & CV_DETAIL)
				{
					editdata.pFieldInfo = (FIELDINFO*) WinSendMsg(hwnd, CM_QUERYDETAILFIELDINFO, MPFROMP(NULL), MPFROMSHORT(CMA_FIRST));
					editdata.pFieldInfo = (FIELDINFO*) WinSendMsg(hwnd, CM_QUERYDETAILFIELDINFO, MPFROMP(editdata.pFieldInfo), MPFROMSHORT(CMA_NEXT));
					editdata.id = CID_LEFTDVWND;
				}
				else
				{
					editdata.pFieldInfo = NULL;
					editdata.id = WinQueryWindowUShort(hwnd,QWS_ID);
				}

				WinSendMsg(hwnd,CM_OPENEDIT,MPFROMP(&editdata),0);

				return 0;
			}

			break;
		}
	}

	return wpCT( hwnd, msg, mp1, mp2 );
}


MRESULT EXPENTRY wpCD(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
	switch (msg)
	{
		case WM_INITDLG:
		{
			ULONG rc, i;
			HFILE CDDevice;
			ULONG action;
			ULONG len;
			struct
			{
				USHORT CountCD;
				USHORT FirstCD;
			} CDInfo;
			FIELDINFO *firstFieldInfo, *posFieldInfo, *splitFieldInfo;
			FIELDINFOINSERT fieldInfoInsert;
			CNRINFO cnrInfo;
			cnrInfo.cb = sizeof(cnrInfo);

			firstFieldInfo = posFieldInfo = allocaFieldInfo(hwnd, CT_TRACK, NUM_FIELDS);
			posFieldInfo->flData = CFA_BITMAPORICON | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Icon";
			posFieldInfo->offStruct = FIELDOFFSET(RECORDCORE,hptrIcon);
			fieldInfo[0] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Filename";
			posFieldInfo->offStruct = FIELDOFFSET(RECORDCORE,pszIcon);
			fieldInfo[1] = posFieldInfo;

			cnrInfo.pFieldInfoLast = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_ULONG | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Track #";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,track);
			fieldInfo[2] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR | CFA_FIREADONLY;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Type";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,typepointer);
			fieldInfo[3] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR | CFA_FIREADONLY;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Time";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,timepointer);
			fieldInfo[4] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_ULONG | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Size";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,size);
			fieldInfo[5] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_ULONG | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "MP3 Size";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,mp3size);
			fieldInfo[6] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Title";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,titleptr);
			fieldInfo[7] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Artist";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,artistptr);
			fieldInfo[8] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Album";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,albumptr);
			fieldInfo[9] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Year";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,yearptr);
			fieldInfo[10] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Genre";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,genreptr);
			fieldInfo[11] = posFieldInfo;

			posFieldInfo = posFieldInfo->pNextFieldInfo;
			posFieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			posFieldInfo->flTitle = CFA_CENTER;
			posFieldInfo->pTitleData = "Comment";
			posFieldInfo->offStruct = FIELDOFFSET(CDTRACKRECORD,commentptr);
			fieldInfo[12] = posFieldInfo;

			fieldInfoInsert.cb = sizeof(fieldInfoInsert);
			fieldInfoInsert.pFieldInfoOrder = (FIELDINFO *) CMA_FIRST;
			fieldInfoInsert.fInvalidateFieldInfo = TRUE;
			fieldInfoInsert.cFieldInfoInsert = NUM_FIELDS;

			insertFieldInfo(hwnd, CT_TRACK, firstFieldInfo, &fieldInfoInsert);

			cnrInfo.xVertSplitbar = 100;
			cnrInfo.flWindowAttr = CV_DETAIL | CA_DETAILSVIEWTITLES;
			WinSendDlgItemMsg(hwnd,CT_TRACK,CM_SETCNRINFO, MPFROMP(&cnrInfo),
				MPFROMLONG(CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR | CMA_FLWINDOWATTR));


			loadConfig(CFGFILE, &grabbers, &countGrabbers);
			for(i = 0; i < countGrabbers; i++)
				insertItemText(hwnd,CB_GRABBER,LIT_END,grabbers[i].id);
			selectItem(hwnd,CB_GRABBER,0);

			setText(hwnd, EF_TITLE, "Title");

			/* wohw, this is too powerful, need cooling */

			len = sizeof(CDInfo);
			if(!DosOpen("\\DEV\\CD-ROM2$", &CDDevice, &action, 0,
							FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
							OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY, NULL))
			{
				if(!DosDevIOCtl(CDDevice, 0x82, 0x60, NULL, 0, NULL, &CDInfo, len, &len))
				{
					for(i = 0; i < CDInfo.CountCD; i++)
					{
						char driveLetter[3] = { (char) ('A' + CDInfo.FirstCD + i), ':', 0};
						insertItemText(hwnd,CB_DRIVE,LIT_END,driveLetter);
					}
				}
				DosClose(CDDevice);
			}

			selectItem(hwnd,CB_DRIVE,0);

			wpCT = WinSubclassWindow(WinWindowFromID(hwnd,CT_TRACK),wpCTTrack);

			dataIco = WinLoadPointer(HWND_DESKTOP, NULLHANDLE, ICO_DATA);
			trackIco = WinLoadPointer(HWND_DESKTOP, NULLHANDLE, ICO_TRACK);
			loadIni(hwnd);
			return 0;
		}
		case WM_COMMAND:
			return processCommand(hwnd,mp1,mp2);
		case WM_CONTROL:
			return processControl(hwnd,mp1,mp2);
		case WM_ADJUSTFRAMEPOS:
		{
			SWP *pos = (SWP*) PVOIDFROMMP(mp1);
			static int bitRateCheck = 0;
			if(pos->fl & SWP_SIZE)
			{
				SWP ctpos;
				WinQueryWindowPos(WinWindowFromID(hwnd, CT_TRACK), &ctpos);
				WinSetWindowPos  (WinWindowFromID(hwnd, CT_TRACK), 0, 0, ctpos.y,
									  pos->cx, pos->cy - ctpos.y,
									  SWP_SIZE | SWP_SHOW | SWP_MOVE);
			}

			if((pos->fl & SWP_SHOW) && bitRateChanged != bitRateCheck)
			{
				bitRateCheck = bitRateChanged;
				refreshFieldInfo(hwnd, CT_TRACK);
			}
			break;
		}

		case WM_MENUEND:
			removeSourceEmphasis(HWNDFROMMP(mp2),&sourceEmphasisInfo);
			return 0;

		case WM_CLOSE:
		{
			WinDestroyPointer(dataIco);
			WinDestroyPointer(trackIco);
			free(grabbers);
			saveIni(hwnd);

			/* delete all current records */
			CDTRACKRECORD *record = (CDTRACKRECORD *) enumRecords(hwnd, CT_TRACK, NULL, CMA_FIRST);
			while(record && record != (CDTRACKRECORD *) -1)
			{
				free(record->record.pszIcon);
				record = (CDTRACKRECORD *) enumRecords(hwnd, CT_TRACK, (RECORDCORE *) record, CMA_NEXT);
			}

			removeRecords(hwnd, CT_TRACK, NULL, 0);

			removeFieldInfo(hwnd,CT_TRACK, NULL, 0);
			return 0;
		}

		case WM_CHAR:
			if(SHORT2FROMMP(mp2) == VK_ESC)
				return 0;
			else
				break;

		/* back from worker thread */
		case DLGCD_REFRESH:
			dlgcd_refresh2(mp1,mp2);
			return 0;

      case CDDB_FUZZYMATCH:
		{
			CDDBQUERY_DATA *matches = (CDDBQUERY_DATA *) mp1,
								*chosen	= (CDDBQUERY_DATA *) mp2;
         FUZZYMATCHCREATEPARAMS data = {matches,chosen};

			WinDlgBox(HWND_DESKTOP, hwnd, wpMatch, NULLHANDLE, DLG_MATCH, &data);
              
			return 0;
		}

	}

	return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}
