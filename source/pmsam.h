#ifndef __cplusplus
	#define inline _Inline	// that's for VAC++, use static at worst
#endif


#ifndef BKS_TABBEDDIALOG
 #define BKS_TABBEDDIALOG			 0x00000800  /* Tabbed dialog 		  */
#endif
#ifndef BKS_BUTTONAREA
 #define BKS_BUTTONAREA 			 0x00000200  /* Reserve space for	  */
#endif

HWND createClientFrame(HAB hab, HWND *hwndClient, char *classname,
				 ULONG *frameFlags, PFNWP winproc, char *title);
BOOL loadPosition(HWND hwnd, char *inifilename);
BOOL savePosition(HWND hwnd, char *inifilename);

/* calling thread needs a message queue */
void _System updateError(char *fmt, ...);
void initError(HWND mainhwnd, LONG x, LONG y, LONG cx, LONG cy);
void unInitError(void);

void _System updateStatus(char *fmt, ...);
void initStatus(HWND mainhwnd, LONG x, LONG y, LONG cx, LONG cy);
void unInitStatus(void);

/* put in WM_CHAR */
void doControlNavigation(HWND hwnd, MPARAM mp1, MPARAM mp2);

/********************************/
/* notebook page info structure */
/********************************/

typedef struct _NBPAGE
{
	 PFNWP	 pfnwpDlg;					/* Window procedure address for the dialog */
	 PSZ		 szStatusLineText;		/* Text to go on status line */
	 PSZ		 szTabText; 				/* Text to go on major tab */
	 ULONG	 idDlg;						/* ID of the dialog box for this page */
	 BOOL 	 skip;						/* skip this page (for major pages with minor pages) */
	 USHORT	 usTabType; 				/* BKA_MAJOR or BKA_MINOR */
	 ULONG	 ulPageId;					/* notebook page ID */
	 HWND 	 hwnd;						/* set when page frame is loaded */

} NBPAGE, *PNBPAGE;

HWND createNotebook(HWND hwnd, NBPAGE *nbpage, ULONG pageCount);
BOOL loadNotebookDlg(HWND hwndNotebook, NBPAGE *nbpage, ULONG pageCount);

/* very common PM stuff which I couldn't care less to remember by heart */

inline BOOL initPM(HAB *hab, HMQ *hmq)
{
	*hmq = *hab = 0;

	*hab = WinInitialize(0);
	if(*hab)
	  *hmq = WinCreateMsgQueue(*hab, 0);
	if(*hmq)
	  return TRUE;

	return FALSE;
}

inline void runPM(HAB hab)
{
	QMSG qmsg;

	while( WinGetMsg( hab, &qmsg, 0L, 0, 0 ) )
		WinDispatchMsg( hab, &qmsg );
}

inline void closePM(HAB hab, HMQ hmq)
{
	WinDestroyMsgQueue( hmq );
	WinTerminate( hab );
}

inline BOOL isWarpSans(void)
{
	LONG fontcounter = 0;
	HPS hps;
	BOOL rc;

	hps = WinGetPS(HWND_DESKTOP);
	rc = GpiQueryFonts(hps, QF_PUBLIC,"WarpSans", &fontcounter, 0, NULL);
	WinReleasePS(hps);
	return rc;
}

inline HWND loadDlg(HWND parent, PFNWP winproc, ULONG id)
{
	return WinLoadDlg(parent, parent, winproc, 0, id, NULL);
}

/*****************/
/* check buttons */
/*****************/

inline USHORT getCheck(HWND hwnd, LONG id)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, BM_QUERYCHECK, 0, 0));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, BM_QUERYCHECK, 0, 0));
}

inline USHORT setCheck(HWND hwnd, LONG id, USHORT state)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, BM_SETCHECK, MPFROMSHORT(state), 0));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, BM_SETCHECK, MPFROMSHORT(state), 0));
}

/******************/
/* General Window */
/******************/

inline ULONG getText(HWND hwnd, LONG id, char *buffer, LONG size)
{
	if(id)
		return WinQueryDlgItemText(hwnd,id,size,buffer);
	else
		return WinQueryWindowText(hwnd,size,buffer);
}

inline BOOL setText(HWND hwnd, LONG id, char *buffer)
{
	if(id)
		return WinSetDlgItemText(hwnd, id, buffer);
	else
		return WinSetWindowText(hwnd, buffer);
}

inline BOOL enable(HWND hwnd, LONG id)
{
	if(id)
		return WinEnableWindow(WinWindowFromID(hwnd,id), TRUE);
	else
		return WinEnableWindow(hwnd, TRUE);
}

inline BOOL disable(HWND hwnd, LONG id)
{
	if(id)
		return WinEnableWindow(WinWindowFromID(hwnd,id), FALSE);
	else
		return WinEnableWindow(hwnd, FALSE);
}

/***********/
/* listbox */
/***********/

inline SHORT getItemCount(HWND hwnd, LONG id)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id,LM_QUERYITEMCOUNT,0,0));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYITEMCOUNT,0,0));
}

inline SHORT getItemText(HWND hwnd, LONG id, SHORT item, char *buffer, LONG size)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id,LM_QUERYITEMTEXT,MPFROM2SHORT(item,size),MPFROMP(buffer)));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYITEMTEXT,MPFROM2SHORT(item,size),MPFROMP(buffer)));
}

inline SHORT getItemTextSize(HWND hwnd, LONG id, SHORT item)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id,LM_QUERYITEMTEXTLENGTH,MPFROMSHORT(item),0));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYITEMTEXTLENGTH,MPFROMSHORT(item),0));
}

inline SHORT insertItemText(HWND hwnd, LONG id, SHORT item, char *buffer)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, LM_INSERTITEM, MPFROMSHORT(item), MPFROMP(buffer)));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, LM_INSERTITEM, MPFROMSHORT(item), MPFROMP(buffer)));
}

inline BOOL setItemHandle(HWND hwnd, LONG id, SHORT item, void *pointer)
{
	if(id)
		return LONGFROMMR(WinSendDlgItemMsg(hwnd, id, LM_SETITEMHANDLE, MPFROMSHORT(item), MPFROMP(pointer)));
	else
		return LONGFROMMR(WinSendMsg(hwnd, LM_SETITEMHANDLE, MPFROMSHORT(item), MPFROMP(pointer)));
}

inline void *getItemHandle(HWND hwnd, LONG id, SHORT item)
{
	if(id)
		return PVOIDFROMMR(WinSendDlgItemMsg(hwnd, id, LM_QUERYITEMHANDLE, MPFROMSHORT(item), 0));
	else
		return PVOIDFROMMR(WinSendMsg(hwnd, LM_QUERYITEMHANDLE, MPFROMSHORT(item), 0));
}

inline SHORT deleteItem(HWND hwnd, LONG id, SHORT item)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_DELETEITEM, MPFROMSHORT(item),0));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, LM_DELETEITEM, MPFROMSHORT(item),0));
}

inline SHORT deleteAllItems(HWND hwnd, LONG id)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_DELETEALL, 0,0));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, LM_DELETEALL, 0,0));
}

inline SHORT selectItem(HWND hwnd, LONG id, SHORT item)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_SELECTITEM, MPFROMSHORT(item),MPFROMLONG(TRUE)));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, LM_SELECTITEM, MPFROMSHORT(item),MPFROMLONG(TRUE)));
}

inline SHORT deSelectItem(HWND hwnd, LONG id, SHORT item)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_SELECTITEM, MPFROMSHORT(item),MPFROMLONG(FALSE)));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, LM_SELECTITEM, MPFROMSHORT(item),MPFROMLONG(FALSE)));
}

inline SHORT getSelectItem(HWND hwnd, LONG id, SHORT startitem)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_QUERYSELECTION, MPFROMSHORT(startitem),0));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, LM_QUERYSELECTION, MPFROMSHORT(startitem),0));
}

inline SHORT searchItemText(HWND hwnd, LONG id, SHORT startitem, char *string)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_SEARCHSTRING, MPFROM2SHORT(0,startitem),MPFROMP(string)));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, LM_SEARCHSTRING, MPFROM2SHORT(0,startitem),MPFROMP(string)));
}


/**************/
/* Containers */
/**************/

inline RECORDCORE *allocaRecords(HWND hwnd, LONG id, USHORT count, USHORT custom)
{
	if(id)
		return (RECORDCORE *) PVOIDFROMMR(WinSendDlgItemMsg(hwnd,id, CM_ALLOCRECORD, MPFROMSHORT(custom), MPFROMSHORT(count)));
	else
		return (RECORDCORE *) PVOIDFROMMR(WinSendMsg(hwnd, CM_ALLOCRECORD, MPFROMSHORT(custom), MPFROMSHORT(count)));
}

inline BOOL freeRecords(HWND hwnd, LONG id, RECORDCORE *records, ULONG count)
{
	if(id)
		return LONGFROMMR(WinSendDlgItemMsg(hwnd,id, CM_FREERECORD, MPFROMP(records), MPFROMLONG(count)));
	else
		return LONGFROMMR(WinSendMsg(hwnd, CM_FREERECORD, MPFROMP(records), MPFROMLONG(count)));
}

inline ULONG insertRecords(HWND hwnd, LONG id, RECORDCORE *records, RECORDINSERT *info)
{
	if(id)
		return LONGFROMMR(WinSendDlgItemMsg(hwnd,id, CM_INSERTRECORD, MPFROMP(records), MPFROMP(info)));
	else
		return LONGFROMMR(WinSendMsg(hwnd, CM_INSERTRECORD, MPFROMP(records), MPFROMP(info)));
}

inline RECORDCORE *searchRecords(HWND hwnd, LONG id, RECORDCORE *record, USHORT emphasis)
{
	if(id)
		return (RECORDCORE *) PVOIDFROMMR(WinSendDlgItemMsg(hwnd,id, CM_QUERYRECORDEMPHASIS, MPFROMP(record), MPFROMLONG(emphasis)));
	else
		return (RECORDCORE *) PVOIDFROMMR(WinSendMsg(hwnd, CM_QUERYRECORDEMPHASIS, MPFROMP(record), MPFROMLONG(emphasis)));
}

inline RECORDCORE *enumRecords(HWND hwnd, LONG id, RECORDCORE *record, USHORT cmd)
{
	if( (ULONG) record == CMA_FIRST) cmd = CMA_FIRST;
	else if( (ULONG) record == CMA_LAST) cmd = CMA_LAST;

	if(id)
		return (RECORDCORE *) PVOIDFROMMR(WinSendDlgItemMsg(hwnd,id, CM_QUERYRECORD, MPFROMP(record), MPFROM2SHORT(cmd,CMA_ITEMORDER)));
	else
		return (RECORDCORE *) PVOIDFROMMR(WinSendMsg(hwnd, CM_QUERYRECORD, MPFROMP(record), MPFROM2SHORT(cmd,CMA_ITEMORDER)));
}

inline ULONG removeRecords(HWND hwnd, LONG id, RECORDCORE *records[], ULONG count)
{
	if(id)
		return LONGFROMMR(WinSendDlgItemMsg(hwnd,id, CM_REMOVERECORD,
		MPFROMP(records), MPFROM2SHORT(count, CMA_INVALIDATE | CMA_FREE)));
	else
		return LONGFROMMR(WinSendMsg(hwnd, CM_REMOVERECORD,
		MPFROMP(records), MPFROM2SHORT(count, CMA_INVALIDATE | CMA_FREE)));
}

inline BOOL getRecordPosition(HWND hwnd, LONG id, RECORDCORE *record, RECTL *pos, ULONG fsExtent)
{
	QUERYRECORDRECT query;

	query.cb = sizeof(query);
	query.pRecord = record;
	query.fRightSplitWindow = FALSE;
//   query.fsExtent = CMA_ICON | CMA_TEXT;
	query.fsExtent = fsExtent;

	if(id)
		return LONGFROMMR(WinSendDlgItemMsg(hwnd,id, CM_QUERYRECORDRECT, MPFROMP(pos), MPFROMP(&query)));
	else
		return LONGFROMMR(WinSendMsg(hwnd, CM_QUERYRECORDRECT, MPFROMP(pos), MPFROMP(&query)));
}

inline FIELDINFO *allocaFieldInfo(HWND hwnd, LONG id, USHORT count)
{
	if(id)
		return (FIELDINFO *) PVOIDFROMMR(WinSendDlgItemMsg(hwnd, id, CM_ALLOCDETAILFIELDINFO, MPFROMSHORT(count), 0));
	else
		return (FIELDINFO *) PVOIDFROMMR(WinSendMsg(hwnd, CM_ALLOCDETAILFIELDINFO, MPFROMSHORT(count), 0));
}

inline USHORT insertFieldInfo(HWND hwnd, LONG id, FIELDINFO *records, FIELDINFOINSERT *info)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, CM_INSERTDETAILFIELDINFO, MPFROMP(records), MPFROMP(info)));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, CM_INSERTDETAILFIELDINFO, MPFROMP(records), MPFROMP(info)));
}

inline SHORT removeFieldInfo(HWND hwnd, LONG id, FIELDINFO *fields[], USHORT count)
{
	if(id)
		return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, CM_REMOVEDETAILFIELDINFO, MPFROMP(fields), MPFROM2SHORT(count,CMA_FREE)));
	else
		return SHORT1FROMMR(WinSendMsg(hwnd, CM_REMOVEDETAILFIELDINFO, MPFROMP(fields), MPFROM2SHORT(count,CMA_FREE)));
}


inline BOOL setRecordSource(HWND hwnd, LONG id, RECORDCORE *record, BOOL on)
{
	if(id)
		return LONGFROMMR(WinSendDlgItemMsg(hwnd, id, CM_SETRECORDEMPHASIS, MPFROMP(record), MPFROM2SHORT(on, CRA_SOURCE)));
	else
		return LONGFROMMR(WinSendMsg(hwnd, CM_SETRECORDEMPHASIS, MPFROMP(record), MPFROM2SHORT(on, CRA_SOURCE)));
}

inline BOOL selectRecord(HWND hwnd, LONG id, RECORDCORE *record, BOOL on)
{
	if(id)
		return LONGFROMMR(WinSendDlgItemMsg(hwnd, id, CM_SETRECORDEMPHASIS, MPFROMP(record), MPFROM2SHORT(on, CRA_SELECTED)));
	else
		return LONGFROMMR(WinSendMsg(hwnd, CM_SETRECORDEMPHASIS, MPFROMP(record), MPFROM2SHORT(on, CRA_SELECTED)));
}

inline BOOL selectAllRecords(HWND hwnd, LONG id, BOOL on)
{
	BOOL returnBool = FALSE;
	RECORDCORE *record;
	record = enumRecords(hwnd, id, NULL, CMA_FIRST);
	while(record && record != (RECORDCORE *) -1)
	{
		returnBool = selectRecord(hwnd, id, record, on);
		record = enumRecords(hwnd, id, record, CMA_NEXT);
	}
	return returnBool;
}

inline BOOL removeTitleFromContainer(HWND hwnd, LONG id, char *title)
{
   RECORDCORE *record[1];

   *record = enumRecords(hwnd, id, NULL, CMA_FIRST);

	while(*record && *record != (RECORDCORE *) -1)
	{
		if(!strcmp(title,(*record)->pszIcon))
		{
			removeRecords(hwnd, id, record, 1);
			return TRUE;
		}
		*record = enumRecords(hwnd, id, *record, CMA_NEXT);
	}
	return FALSE;
}


/****************************************************/
/* processing of popup menu for containers, fiuf... */
/****************************************************/

typedef struct
{
	HWND PUMHwnd, CTHwnd;
	RECORDCORE *sourceRecord; /* record at the origin of the popup */
} removeSourceEmphasisInfo;

/* check if the source record is selected, and if so, return TRUE */
BOOL isSourceSelected(HWND hwnd, RECORDCORE *record);

/* sets source emphasis for all selected records */
void setSelectedSourceEmphasis(HWND hwnd, BOOL on);

/* for WM_ENDMENU, use return info from processPopUp */
BOOL removeSourceEmphasis(HWND hwnd, removeSourceEmphasisInfo *info);

/* hwnd = dialog who receives notification
	id = container ID
	record = source record	-	0 for container
	recordIDM = record menu ID
	container IDM = container menu ID
	info = returned info for use with removeSourceEmphasis and others

	use in CN_CONTEXTMENU */
void processPopUp(HWND hwnd, LONG id, RECORDCORE *record,
	  LONG recordIDM, LONG containerIDM, removeSourceEmphasisInfo *info);
