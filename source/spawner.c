/* CD2MP3 PM 1.14 (C) 1998-2001 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>

#include "pmsam.h"
#include "cd2mp3pm.h"
#include "spawner.h"

HSWITCH APIENTRY16 WINHSWITCHFROMHAPP(HAPP happ);

typedef struct
{
	HAPP hApp;
	HWND hwnd;
} WIN32OS2WAIT;

/* I wonder why WinStartApp() is capable of finding an unrelated session
   PID while DosStartSession() can't... maybe because WinStartApp() finds
   from the PMSHELL process..?? */
void _Optlink win32os2Wait(void *arg)
{
   HMQ hmqthread = WinCreateMsgQueue(mainhab, 0);

	WIN32OS2WAIT *info = (WIN32OS2WAIT *) arg;
	HSWITCH hswitch;
	SWCNTRL swctl;
	BOOL finished;

	finished = FALSE;
	do
	{
		DosSleep(500);

		hswitch = WINHSWITCHFROMHAPP(info->hApp);
		WinQuerySwitchEntry(hswitch,&swctl);
		if(hswitch)
		{
			if(strstr(swctl.szSwtitle, "Completed:") == swctl.szSwtitle)
			{
				/* seems to do the same thing as WinTerminateApp() */
// 			  DosKillProcess(DKP_PROCESS,swctl.idProcess);
				/* Win32-OS/2 sessions return weird return codes, let's always
               report 0 before the real message gets there */
            /* but this gets the whole thing confused, due to timing
               problems so let's ignore it in the main routine instead */
//            WinSendMsg(info->hwnd, WM_APPTERMINATENOTIFY, MPFROMLONG(info->hApp),0);
            WinTerminateApp(info->hApp);
				finished = TRUE;
			}
		}
		else
			finished = TRUE;
	} while(!finished);

	free(info);
	WinDestroyMsgQueue(hmqthread);
}


HAPP doWin32OS2(SPAWNER_PARAM *input)
{
	PROGDETAILS programDetails;
	HAPP hApp;
	APIRET rc;
	int thread;
	WIN32OS2WAIT *threadfood = (WIN32OS2WAIT *) malloc(sizeof(WIN32OS2WAIT));
	ULONG savePrioClass;
	LONG savePrioDelta;
	PTIB ptib;
	PPIB ppib;

	DosGetInfoBlocks(&ptib, &ppib);

	memset(&programDetails,0,sizeof(programDetails));
	programDetails.Length = sizeof(programDetails);
	programDetails.progt.progc = PROG_PM;
	programDetails.progt.fbVisible = SHE_VISIBLE;
//   programDetails.pszTitle = title;
	programDetails.pszExecutable = input->program;
	programDetails.pszParameters = input->cmdline;
	programDetails.pszStartupDir = "";
//   programDetails.pszICON = "??";
	programDetails.pszEnvironment = "NOWIN32LOG=anything\0";
	programDetails.swpInitial.fl = SWP_ACTIVATE;
//   programDetails.swpInitial.x = 0;
//   programDetails.swpInitial.y = 0;
//   programDetails.swpInitial.cx = 0;
//   programDetails.swpInitial.cy = 0;
	programDetails.swpInitial.hwndInsertBehind = HWND_TOP;
	programDetails.swpInitial.hwnd = input->hwnd;

	savePrioClass = ptib->tib_ptib2->tib2_ulpri>>8 & 0xFF;
	savePrioDelta = ptib->tib_ptib2->tib2_ulpri & 0xFF;

	DosSetPriority(PRTYS_PROCESS, input->prioClass, input->prioDelta, 0);
	hApp = WinStartApp(input->hwnd, &programDetails, NULL, NULL, SAF_INSTALLEDCMDLINE);
	DosSetPriority(PRTYS_PROCESS, savePrioClass, savePrioDelta, 0);

	if(!hApp)
      updateError("Could not start %s. WinGetLastError = %x", input->program, WinGetLastError(mainhab));
	else
	{
		threadfood->hApp = hApp;
		threadfood->hwnd = input->hwnd;

		thread = _beginthread(win32os2Wait,0,16384,(void *) threadfood);
		if(thread == -1)
			updateError("Could not start kludge thread for %s. errno = %d", input->program, errno);
	}

	return hApp;
}

HAPP doNormal(SPAWNER_PARAM *input)
{
	PROGDETAILS programDetails;
	HAPP hApp;
	char title[1024];
	ULONG savePrioClass;
	LONG savePrioDelta;
	PTIB ptib;
	PPIB ppib;

	DosGetInfoBlocks(&ptib, &ppib);

	strcpy(title, input->program);
	strcat(title, " ");
	strcat(title, input->cmdline);

	memset(&programDetails,0,sizeof(programDetails));
	programDetails.Length = sizeof(programDetails);
	programDetails.progt.progc = input->sessionType;
	programDetails.progt.fbVisible = SHE_VISIBLE;
	programDetails.pszTitle = title;
	programDetails.pszExecutable = input->program;
	programDetails.pszParameters = input->cmdline;
	programDetails.pszStartupDir = "";
//   programDetails.pszICON = "??";
//   programDetails.pszEnvironment = "VARIABLE=anything\0";
	programDetails.swpInitial.fl = SWP_ACTIVATE;
//   programDetails.swpInitial.x = 0;
//   programDetails.swpInitial.y = 0;
//   programDetails.swpInitial.cx = 0;
//   programDetails.swpInitial.cy = 0;
	programDetails.swpInitial.hwndInsertBehind = HWND_TOP;
	programDetails.swpInitial.hwnd = input->hwnd;

	savePrioClass = ptib->tib_ptib2->tib2_ulpri>>8 & 0xFF;
	savePrioDelta = ptib->tib_ptib2->tib2_ulpri & 0xFF;

	DosSetPriority(PRTYS_PROCESS, input->prioClass, input->prioDelta, 0);
	hApp = WinStartApp(input->hwnd, &programDetails, NULL, NULL, SAF_STARTCHILDAPP | SAF_INSTALLEDCMDLINE);
	DosSetPriority(PRTYS_PROCESS, savePrioClass, savePrioDelta, 0);

	if(!hApp)
      updateError("Could not start %s. WinGetLastError = %x", input->program, WinGetLastError(mainhab));

	return hApp;
}
