/* CD2MP3 PM 1.14 (C) 1998-2001 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "prfsam.h"
#include "pmsam.h"
#include "cd2mp3pm.h"
#include "cd2mp3.h"
#include "wav.h"

static PFNWP wpCT;

/* for WM_ENDMENU */
static removeSourceEmphasisInfo sourceEmphasisInfo;
static BOOL miniIcons = 0;

/* container icons */
static HPOINTER qmarkIco, wavIco;

static void fillFieldInfo(WAVRECORD *record)
{
	int hfile;
	RIFF RIFFheader;
	struct stat fi;
	int samplerate, channels, bits, format, size;
   USHORT bitrate = getBitRate();
                      
	hfile = openWAV(record->record.pszIcon, READ, &RIFFheader,
						 &samplerate, &channels, &bits, &format, &size);

	if(hfile > 0)
	{
		ULONG seconds = 0;

		record->size = size;
		record->samplerate = samplerate;
		record->channels = channels;
		record->bits = bits;
      record->format = format;
      record->mp3size = 0;
      record->record.hptrIcon = wavIco;
      record->record.hptrMiniIcon = 0; /* ask God */

		switch(format)
		{
			case WAVE_FORMAT_PCM:
				strcpy(record->type,"PCM");
				seconds = record->size/(samplerate*channels*bits/8);
            if(bitrate != 0)
               record->mp3size = record->size /
                       ((samplerate*channels*bits/8) /
                        ((double) bitrate * 1000/8)) + 0.5;
				break;

			case WAVE_FORMAT_ADPCM:
			case IBM_FORMAT_ADPCM:
			case WAVE_FORMAT_OKI_ADPCM:
				strcpy(record->type,"ADPCM");
				seconds = record->size/(samplerate*channels*bits/8)*4;
            if(bitrate != 0)
               record->mp3size = record->size * 4 /
                       ((samplerate*channels*bits/8) /
                        ((double) bitrate * 1000/8)) + 0.5;
				break;

			case WAVE_FORMAT_ALAW:
			case IBM_FORMAT_ALAW:
				strcpy(record->type,"ALAW");
				seconds = record->size/(samplerate*channels*bits/8);
            if(bitrate != 0)
               record->mp3size = record->size /
                       ((samplerate*channels*bits/8) /
                        ((double) bitrate * 1000/8)) + 0.5;
				break;

			case WAVE_FORMAT_MULAW:
			case IBM_FORMAT_MULAW:
				strcpy(record->type,"MULAW");
				seconds = record->size/(samplerate*channels*bits/8);
            if(bitrate != 0)
               record->mp3size = record->size /
                       ((samplerate*channels*bits/8) /
                        ((double) bitrate * 1000/8)) + 0.5;
				break;

			case WAVE_FORMAT_DIGISTD:
			case WAVE_FORMAT_DIGIFIX:
				strcpy(record->type,"DIGI?!");
				record->mp3size = 0;
				break;

			default:
				strcpy(record->type,"Unknown");
				record->mp3size = 0;
				break;
		}

		sprintf(record->time, "%d:%.2d",seconds / 60, seconds % 60);

		closeWAV(hfile);
	}
	else
	{
		int hfile = open(record->record.pszIcon,O_RDONLY | O_BINARY,S_IWRITE);
		strcpy(record->type,"Unknown");
		if(hfile > 0)
		{
			fstat(hfile, &fi);
			record->size = fi.st_size;
			close(hfile);
		}
		else
			record->size = 0;

		record->samplerate = 0;
		record->channels = 0;
		record->bits = 0;
      record->format = 0;
		record->mp3size = 0;
      record->record.hptrIcon = qmarkIco;
      record->record.hptrMiniIcon = 0; /* ask God */

		strcat(record->time, "N/A");
	}

	record->timepointer = record->time; /* kludge */
	record->typepointer = record->type; /* kludge */
}

/* refresh MP3 size with new bitrate */
void refreshFieldInfo(HWND hwnd, LONG id)
{
   WAVRECORD *record;
   USHORT bitrate = getBitRate();

   record = (WAVRECORD *) enumRecords(hwnd, id, NULL, CMA_FIRST);
   while(record && record != (WAVRECORD *) -1)
   {
      switch(record->format)
		{
			case WAVE_FORMAT_PCM:
            if(bitrate)
               record->mp3size = record->size /
                       ((record->samplerate*record->channels*record->bits/8) /
                        ((double) getBitRate() * 1000/8)) + 0.5;
            else
               record->mp3size = 0;
            break;

			case WAVE_FORMAT_ADPCM:
			case IBM_FORMAT_ADPCM:
			case WAVE_FORMAT_OKI_ADPCM:
            if(bitrate)
               record->mp3size = record->size * 4 /
                       ((record->samplerate*record->channels*record->bits/8) /
                        ((double) getBitRate() * 1000/8)) + 0.5;
            else
               record->mp3size = 0;
            break;

			case WAVE_FORMAT_ALAW:
			case IBM_FORMAT_ALAW:
            if(bitrate)
               record->mp3size = record->size /
                       ((record->samplerate*record->channels*record->bits/8) /
                        ((double) getBitRate() * 1000/8)) + 0.5;
            else
               record->mp3size = 0;
            break;

			case WAVE_FORMAT_MULAW:
			case IBM_FORMAT_MULAW:
            if(bitrate)
               record->mp3size = record->size /
                       ((record->samplerate*record->channels*record->bits/8) /
                        ((double) getBitRate() * 1000/8)) + 0.5;
            else
               record->mp3size = 0;
            break;
			default:
				record->mp3size = 0;
				break;
		}

      record = (WAVRECORD *) enumRecords(hwnd, id, (RECORDCORE *) record, CMA_NEXT);
   }

   WinSendDlgItemMsg(hwnd, id, CM_INVALIDATEDETAILFIELDINFO,0,0);
}


static void loadIni(HWND hwnd)
{
	char *list;
   CNRINFO cnrInfo;
   HINI inifile;
	int listsize;

	inifile = openProfile(INIFILE);

   if(inifile)
   {

   #if 0
      if(readProfile(inifile,"WAV","Wav",buffer,sizeof(buffer), TRUE))
         setText(hwnd, EF_WAV, buffer);
   #endif

      listsize = getProfileSize(inifile,"WAV","WavList");
      list = (char*) alloca(listsize);

      if(readProfile(inifile,"WAV","WavList",list, listsize, FALSE))
      {
         char *listpos = list;
         int count = 0, i = 0;
         WAVRECORD *firstRecord, *posRecord;
         RECORDINSERT recordInfo;

         while(*listpos) { count++; listpos = strchr(listpos,0)+1; }

         firstRecord = posRecord = (WAVRECORD *) allocaRecords(hwnd, CT_WAV, count,
                             sizeof(WAVRECORD) - sizeof(RECORDCORE));

         listpos = list;
         for (i = 0; i < count; i++)
         {
            posRecord->record.pszIcon = posRecord->record.pszText =
            posRecord->record.pszName = posRecord->record.pszTree =
               (char*) malloc(strlen(listpos)+1);
            strcpy(posRecord->record.pszIcon,listpos);
            fillFieldInfo(posRecord);

            posRecord = (WAVRECORD *) posRecord->record.preccNextRecord;
            listpos = strchr(listpos,0)+1;
         }

         recordInfo.cb = sizeof(recordInfo);
         recordInfo.pRecordOrder = (RECORDCORE *) CMA_END;
         recordInfo.pRecordParent = NULL;
         recordInfo.fInvalidateRecord = TRUE;
         recordInfo.zOrder = CMA_TOP;
         recordInfo.cRecordsInsert = count;

         insertRecords(hwnd, CT_WAV, (RECORDCORE *) firstRecord, &recordInfo);
      }

      if(readProfile(inifile,"WAV","ContainerInfo",&cnrInfo,sizeof(cnrInfo), FALSE))
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_SETCNRINFO, MPFROMP(&cnrInfo),
            MPFROMLONG(CMA_XVERTSPLITBAR | CMA_FLWINDOWATTR));

      miniIcons = (cnrInfo.flWindowAttr & CV_MINI) ? TRUE : FALSE;


      closeProfile(inifile);
   }
}

SHORT makeWavList(HWND hwnd, LONG id, char **wavfilelist)
{
	WAVRECORD *WAVRecord;
	SHORT arraysize = 0;
   char *listpos = *wavfilelist;

	WAVRecord = (WAVRECORD *) enumRecords(hwnd, id, NULL, CMA_FIRST);

	if(WAVRecord && WAVRecord != (WAVRECORD *) -1)
	{
		while(WAVRecord && WAVRecord != (WAVRECORD *) -1)
		{
			arraysize += strlen(WAVRecord->record.pszIcon)+1;
			WAVRecord = (WAVRECORD *) enumRecords(hwnd, id, (RECORDCORE *) WAVRecord, CMA_NEXT);
		}
		arraysize++;
      *wavfilelist = listpos = (char *) calloc(arraysize, 1);

		WAVRecord = (WAVRECORD *) enumRecords(hwnd, id, NULL, CMA_FIRST);
		while(WAVRecord && WAVRecord != (WAVRECORD *) -1)
		{
			strcpy(listpos,WAVRecord->record.pszIcon);
			listpos += strlen(listpos)+1;
			WAVRecord = (WAVRECORD *) enumRecords(hwnd, id, (RECORDCORE *) WAVRecord, CMA_NEXT);
		}
	}
	else
      *wavfilelist = NULL;

	return arraysize;
}

static void saveIni(HWND hwnd)
{  
	char buffer[512];
   CNRINFO cnrInfo;
   HINI inifile;
	char *list;
	SHORT listsize;

	inifile = openProfile(INIFILE);

   if(inifile)
   {
      listsize = makeWavList(hwnd, CT_WAV, &list);
      writeProfile(inifile,"WAV","WavList", list, listsize);
      free(list);

      cnrInfo.cb = sizeof(cnrInfo);
      WinSendDlgItemMsg(hwnd,CT_WAV,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
      writeProfile(inifile,"WAV","ContainerInfo", &cnrInfo, sizeof(cnrInfo));

   #if 0
      getText(hwnd, EF_WAV, buffer, sizeof(buffer));
      writeProfile(inifile,"WAV","Wav", buffer,0);
   #endif

      closeProfile(inifile);
   }
}


static MRESULT processControl(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
	switch(SHORT2FROMMP(mp1))
	{
		case CN_CONTEXTMENU:
		{
         WAVRECORD *record = (WAVRECORD*) PVOIDFROMMP(mp2);

#if 0
			/* resulted from a mouse event */
			ULONG iSysValue;
			if(WinQuerySysValue(HWND_DESKTOP,&iSysValue) && (iSysValue & SV_CONTEXTMENU))
#endif

			processPopUp(hwnd, CT_WAV, (RECORDCORE *) record,
				PUM_WAVRECORD, PUM_WAVCONTAINER, &sourceEmphasisInfo);

         WinSendMsg(sourceEmphasisInfo.PUMHwnd, MM_SETITEMATTR,
            MPFROM2SHORT(IDM_MINIICONS, FALSE),
            MPFROM2SHORT(MIA_CHECKED, miniIcons ? MIA_CHECKED : 0));
			return 0;
		}

		case CN_DRAGOVER:
		{
         CNRDRAGINFO *cnrdraginfo = (CNRDRAGINFO *) PVOIDFROMMP(mp2);
			if(cnrdraginfo->pRecord)
            return (MRFROM2SHORT(DOR_NODROP, DO_LINK));
			else
			{
				DRAGINFO *draginfo = cnrdraginfo->pDragInfo;
				USHORT cItems,i,accept;
				DRAGITEM *dragitem;

				DrgAccessDraginfo(draginfo);
				cItems = DrgQueryDragitemCount(draginfo);
				accept = DOR_DROP;

				for (i = 0; i < cItems; i++)
				{
					dragitem = DrgQueryDragitemPtr(draginfo, i);

					/* Check the rendering format */
					if (!DrgVerifyRMF(dragitem, "DRM_OS2FILE", NULL))
						accept = DOR_NEVERDROP;
				}

				DrgFreeDraginfo(draginfo);

            return (MRFROM2SHORT(accept, DO_LINK));
			}
		}

		case CN_DROP:
		{
         CNRDRAGINFO *cnrdraginfo = (CNRDRAGINFO *) PVOIDFROMMP(mp2);
			if(cnrdraginfo->pRecord)
				return 0;
			else
			{
				DRAGINFO *draginfo = cnrdraginfo->pDragInfo;
				USHORT cItems,i;
				DRAGITEM *dragitem;
				char buffer[2*CCHMAXPATH] = {0};
				WAVRECORD *firstRecord, *posRecord;
				RECORDINSERT recordInfo;

				DrgAccessDraginfo(draginfo);
				cItems = DrgQueryDragitemCount(draginfo);

            firstRecord = posRecord = (WAVRECORD *) allocaRecords(hwnd, CT_WAV, cItems,
                                sizeof(WAVRECORD) - sizeof(RECORDCORE));

				for (i = 0; i < cItems; i++, buffer[0] = '\0')
				{
					dragitem = DrgQueryDragitemPtr(draginfo, i);

					/* Check the rendering format */
					if (DrgVerifyRMF(dragitem, "DRM_OS2FILE", NULL))
					{
						DrgQueryStrName(dragitem->hstrContainerName,CCHMAXPATH,buffer);
						DrgQueryStrName(dragitem->hstrSourceName,CCHMAXPATH,strchr(buffer,'\0'));
					}

					posRecord->record.pszIcon = posRecord->record.pszText =
					posRecord->record.pszName = posRecord->record.pszTree =
                  (char*) malloc(strlen(buffer)+1);
					strcpy(posRecord->record.pszIcon,buffer);
					fillFieldInfo(posRecord);

					posRecord = (WAVRECORD *) posRecord->record.preccNextRecord;
				}

				recordInfo.cb = sizeof(recordInfo);
				recordInfo.pRecordOrder = (RECORDCORE *) CMA_END;
				recordInfo.pRecordParent = NULL;
				recordInfo.fInvalidateRecord = TRUE;
				recordInfo.zOrder = CMA_TOP;
				recordInfo.cRecordsInsert = cItems;

				insertRecords(hwnd, CT_WAV, (RECORDCORE *) firstRecord, &recordInfo);

				DrgFreeDraginfo(draginfo);

				return 0;
			}
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
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
         cnrInfo.flWindowAttr |= CV_ICON;
         cnrInfo.flWindowAttr &= ~CV_NAME & ~CV_DETAIL & ~CV_TREE;
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_SETCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
			return 0;
		}
		case IDM_NAMEVIEW:
		{
			CNRINFO cnrInfo;
			cnrInfo.cb = sizeof(cnrInfo);
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
         cnrInfo.flWindowAttr |= CV_NAME | CV_FLOW;
         cnrInfo.flWindowAttr &= ~CV_ICON & ~CV_DETAIL & ~CV_TREE;
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_SETCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
			return 0;
		}
		case IDM_DETAILVIEW:
		{
			CNRINFO cnrInfo;
			cnrInfo.cb = sizeof(cnrInfo);
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
         cnrInfo.flWindowAttr |= CV_DETAIL | CA_DETAILSVIEWTITLES;
         cnrInfo.flWindowAttr &= ~CV_ICON & ~CV_NAME & ~CV_TREE;
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_SETCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
			return 0;
		}
      case IDM_MINIICONS:
      {
         CNRINFO cnrInfo;
         miniIcons = !miniIcons;

			cnrInfo.cb = sizeof(cnrInfo);
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(sizeof(cnrInfo)));
         cnrInfo.flWindowAttr = (miniIcons) ? (cnrInfo.flWindowAttr | CV_MINI) : (cnrInfo.flWindowAttr & ~CV_MINI);
         WinSendDlgItemMsg(hwnd,CT_WAV,CM_SETCNRINFO, MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
			return 0;
      }

      case IDM_SELECTALL:
         selectAllRecords(hwnd, CT_WAV, TRUE);
         return 0;
      case IDM_DESELECTALL:
         selectAllRecords(hwnd, CT_WAV, FALSE);
         return 0;

		case IDM_ADD:
		{
			int i;
			FILEDLG filedlginfo = {0};
			WAVRECORD *firstRecord, *posRecord;
			RECORDINSERT recordInfo;

			filedlginfo.cbSize = sizeof(FILEDLG);
			filedlginfo.fl = FDS_CENTER | FDS_OPEN_DIALOG | FDS_MULTIPLESEL;
			filedlginfo.pszTitle = "Add WAV Files";
			strcpy(filedlginfo.szFullFile,"*.WAV");
			filedlginfo.pszOKButton = "Add";

			WinFileDlg(HWND_DESKTOP,hwnd,&filedlginfo);

         firstRecord = posRecord = (WAVRECORD *) allocaRecords(hwnd, CT_WAV, filedlginfo.ulFQFCount,
                             sizeof(WAVRECORD) - sizeof(RECORDCORE));

         for(i = 0; i < filedlginfo.ulFQFCount ; i++)
			{
				posRecord->record.pszIcon = posRecord->record.pszText =
				posRecord->record.pszName = posRecord->record.pszTree =
               (char*) malloc(strlen(*filedlginfo.papszFQFilename[i])+1);
				strcpy(posRecord->record.pszIcon,*filedlginfo.papszFQFilename[i]);
				fillFieldInfo(posRecord);

				posRecord = (WAVRECORD *) posRecord->record.preccNextRecord;
			}

			recordInfo.cb = sizeof(recordInfo);
			recordInfo.pRecordOrder = (RECORDCORE *) CMA_END;
			recordInfo.pRecordParent = NULL;
			recordInfo.fInvalidateRecord = TRUE;
			recordInfo.zOrder = CMA_TOP;
			recordInfo.cRecordsInsert = filedlginfo.ulFQFCount;

			insertRecords(hwnd, CT_WAV, (RECORDCORE *) firstRecord, &recordInfo);

			WinFreeFileDlgList(filedlginfo.papszFQFilename);
			break;
		}

		case IDM_DELETE:
		{
			WAVRECORD *record[1];
			if(isSourceSelected(WinWindowFromID(hwnd,CT_WAV),sourceEmphasisInfo.sourceRecord))
			{
				record[0] = (WAVRECORD *) searchRecords(hwnd, CT_WAV,
								(RECORDCORE *) CMA_FIRST, CRA_SELECTED);

				while(record[0] && record[0] != (WAVRECORD *) -1)
				{
					free(record[0]->record.pszIcon);
					removeRecords(hwnd, CT_WAV, (RECORDCORE **) record, 1);
					record[0] = (WAVRECORD *) searchRecords(hwnd, CT_WAV,
									(RECORDCORE *) CMA_FIRST, CRA_SELECTED);
				}
			}
			else
			{
				record[0] = (WAVRECORD *) sourceEmphasisInfo.sourceRecord;
				if(record[0])
				{
					free(record[0]->record.pszIcon);
					removeRecords(hwnd, CT_WAV, (RECORDCORE **) record, 1);
				}
			}
			break;
		}

#if 0
		case IDM_CLEAR:
		{
			WAVRECORD *record;
			record = enumRecords(hwnd, CT_WAV, NULL, CMA_FIRST);
			while(record && record != (WAVRECORD *) -1)
			{
				free(selectedRecord->pszIcon);
				record = enumRecords(hwnd, CT_WAV, record, CMA_NEXT);
			}

			removeRecord(hwnd, CT_WAV, NULL, 0);

			break;
		}
#endif
	}
	return 0;
}


MRESULT EXPENTRY wpCTWav(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
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
               record, PUM_WAVRECORD, PUM_WAVCONTAINER, &sourceEmphasisInfo);

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

         else if(SHORT2FROMMP(mp2) == VK_DELETE)
			{
				sourceEmphasisInfo.sourceRecord = searchRecords(hwnd, 0, (RECORDCORE *) CMA_FIRST, CRA_CURSORED);
				WinSendMsg(WinQueryWindow(hwnd,QW_PARENT),WM_COMMAND,MPFROMSHORT(IDM_DELETE),0);
				return 0;
			}

         else if(SHORT2FROMMP(mp2) == VK_INSERT)
			{
				WinSendMsg(WinQueryWindow(hwnd,QW_PARENT),WM_COMMAND,MPFROMSHORT(IDM_ADD),0);
				return 0;
			}

			break;
		}
	}

	return wpCT( hwnd, msg, mp1, mp2 );
}


MRESULT EXPENTRY wpWAV(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
	switch (msg)
	{
		case WM_INITDLG:
		{
			FIELDINFO *firstFieldInfo, *fieldInfo, *splitFieldInfo;
			FIELDINFOINSERT fieldInfoInsert;
			CNRINFO cnrInfo;
			cnrInfo.cb = sizeof(cnrInfo);

			firstFieldInfo = fieldInfo = allocaFieldInfo(hwnd, CT_WAV, 9);
			fieldInfo->flData = CFA_BITMAPORICON | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			fieldInfo->flTitle = CFA_CENTER;
			fieldInfo->pTitleData = "Icon";
         fieldInfo->offStruct = FIELDOFFSET(RECORDCORE,hptrIcon);

			fieldInfo = fieldInfo->pNextFieldInfo;
			fieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			fieldInfo->flTitle = CFA_CENTER;
         fieldInfo->pTitleData = "Filename";
			fieldInfo->offStruct = FIELDOFFSET(RECORDCORE,pszIcon);

			cnrInfo.pFieldInfoLast = fieldInfo;

			fieldInfo = fieldInfo->pNextFieldInfo;
         fieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR | CFA_FIREADONLY;
			fieldInfo->flTitle = CFA_CENTER;
			fieldInfo->pTitleData = "Type";
			fieldInfo->offStruct = FIELDOFFSET(WAVRECORD,typepointer);

			fieldInfo = fieldInfo->pNextFieldInfo;
			fieldInfo->flData = CFA_ULONG | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			fieldInfo->flTitle = CFA_CENTER;
			fieldInfo->pTitleData = "Size";
			fieldInfo->offStruct = FIELDOFFSET(WAVRECORD,size);

			fieldInfo = fieldInfo->pNextFieldInfo;
			fieldInfo->flData = CFA_ULONG | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			fieldInfo->flTitle = CFA_CENTER;
			fieldInfo->pTitleData = "Bits";
			fieldInfo->offStruct = FIELDOFFSET(WAVRECORD,bits);

			fieldInfo = fieldInfo->pNextFieldInfo;
			fieldInfo->flData = CFA_ULONG | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			fieldInfo->flTitle = CFA_CENTER;
			fieldInfo->pTitleData = "Sample rate";
			fieldInfo->offStruct = FIELDOFFSET(WAVRECORD,samplerate);

			fieldInfo = fieldInfo->pNextFieldInfo;
			fieldInfo->flData = CFA_ULONG | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			fieldInfo->flTitle = CFA_CENTER;
			fieldInfo->pTitleData = "Channels";
			fieldInfo->offStruct = FIELDOFFSET(WAVRECORD,channels);

			fieldInfo = fieldInfo->pNextFieldInfo;
         fieldInfo->flData = CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR | CFA_FIREADONLY;
			fieldInfo->flTitle = CFA_CENTER;
			fieldInfo->pTitleData = "Time";
			fieldInfo->offStruct = FIELDOFFSET(WAVRECORD,timepointer);

			fieldInfo = fieldInfo->pNextFieldInfo;
			fieldInfo->flData = CFA_ULONG | CFA_HORZSEPARATOR | CFA_SEPARATOR;
			fieldInfo->flTitle = CFA_CENTER;
			fieldInfo->pTitleData = "MP3 Size";
			fieldInfo->offStruct = FIELDOFFSET(WAVRECORD,mp3size);

			fieldInfoInsert.cb = sizeof(fieldInfoInsert);
			fieldInfoInsert.pFieldInfoOrder = (FIELDINFO *) CMA_FIRST;
			fieldInfoInsert.fInvalidateFieldInfo = TRUE;
			fieldInfoInsert.cFieldInfoInsert = 9;

			insertFieldInfo(hwnd, CT_WAV, firstFieldInfo, &fieldInfoInsert);

			cnrInfo.xVertSplitbar = 100;
         cnrInfo.flWindowAttr = CV_DETAIL | CA_DETAILSVIEWTITLES;
			WinSendDlgItemMsg(hwnd,CT_WAV,CM_SETCNRINFO, MPFROMP(&cnrInfo),
				MPFROMLONG(CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR | CMA_FLWINDOWATTR));

			wpCT = WinSubclassWindow(WinWindowFromID(hwnd,CT_WAV),wpCTWav);

         qmarkIco = WinLoadPointer(HWND_DESKTOP, NULLHANDLE, ICO_QMARK);
         wavIco = WinLoadPointer(HWND_DESKTOP, NULLHANDLE, ICO_WAV);
         loadIni(hwnd);
			return 0;
		}

      /* check for new bitrate */
		case WM_ADJUSTFRAMEPOS:
		{
			static int bitRateCheck = 0;
         SWP *pos = (SWP*) PVOIDFROMMP(mp1);
			if((pos->fl & SWP_SHOW) && bitRateChanged != bitRateCheck)
			{
            bitRateCheck = bitRateChanged;
            refreshFieldInfo(hwnd, CT_WAV);
			}
			break;
		}

		case WM_COMMAND:
			return processCommand(hwnd,mp1,mp2);
		case WM_CONTROL:
			return processControl(hwnd,mp1,mp2);

      case WM_MENUEND:
         removeSourceEmphasis(HWNDFROMMP(mp2),&sourceEmphasisInfo);
         return 0;

      case WM_CLOSE:
         WinDestroyPointer(qmarkIco);
         WinDestroyPointer(wavIco);
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

