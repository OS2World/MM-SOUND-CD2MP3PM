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


static void loadIni(HWND hwnd)
{
   int i;
	char buffer[512];
   CNRINFO cnrInfo;
	HINI inifile;

	inifile = openProfile(INIFILE);

   if(inifile)
   {
      int datasize, datasize2;

      datasize = getProfileSize(inifile,"CDDB","Servers");
      datasize2 = getProfileSize(inifile,"CDDB","Selected servers");
      if(datasize)
      {
         char *servers = (char*) alloca(datasize);
         char *selected = NULL;
         if(datasize2)
            selected = (char*)alloca(datasize2);

         if(readProfile(inifile,"CDDB","Servers",servers,datasize, FALSE))
         {
            if(selected)
               readProfile(inifile,"CDDB","Selected servers", selected, datasize2, FALSE);

            i = 0;
            do
            {
               insertItemText(hwnd,LB_CDDBSERVERS,LIT_END,servers);
               if(selected && ((*(selected+i/8) >> i%8) & 1))
                  selectItem(hwnd,LB_CDDBSERVERS,i);
               servers = strchr(servers,'\0')+1;
               i++;
            }
            while(*servers);
         }
      }

      datasize = getProfileSize(inifile,"CDDB","HTTP servers");
      datasize2 = getProfileSize(inifile,"CDDB","Selected HTTP servers");
      if(datasize)
      {
         char *servers = (char*) alloca(datasize);
         char *selected = NULL;

         if(datasize2)
            selected = (char*)alloca(datasize2);

         if(readProfile(inifile,"CDDB","HTTP servers",servers,datasize, FALSE))
         {
            if(selected)
               readProfile(inifile,"CDDB","Selected HTTP servers", selected, datasize2, FALSE);

            i = 0;
            do
            {
               insertItemText(hwnd,LB_HTTPCDDBSERVERS,LIT_END,servers);
               if(selected && ((*(selected+i/8) >> i%8) & 1))
                  selectItem(hwnd,LB_HTTPCDDBSERVERS,i);
               servers = strchr(servers,'\0')+1;
               i++;
            }
            while(*servers);
         }
      }

      if(readProfile(inifile,"CDDB","Format",buffer,sizeof(buffer), TRUE))
         setText(hwnd, EF_FORMAT, buffer);
      if(readProfile(inifile,"CDDB","Fallback format",buffer,sizeof(buffer), TRUE))
         setText(hwnd, EF_FALLBACKFF, buffer);
      if(readProfile(inifile,"CDDB","Proxy",buffer,sizeof(buffer), TRUE))
         setText(hwnd, EF_PROXY, buffer);
      if(readProfile(inifile,"CDDB","E-Mail",buffer,sizeof(buffer), TRUE))
         setText(hwnd, EF_EMAIL, buffer);
      if(readProfile(inifile,"CDDB","Use HTTP",buffer,1, FALSE))
         setCheck(hwnd, CB_USEHTTP, *buffer);
      if(readProfile(inifile,"CDDB","Try All",buffer,1, FALSE))
         setCheck(hwnd, CB_TRYALL, *buffer);

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
      int count, i;

      count = getItemCount(hwnd, LB_CDDBSERVERS);
      if(count)
      {
         char *firstserver, *server;
         int sizeall, sizeone;
         char *selected;

         selected = (char*) alloca(count/8+1);
         memset(selected,0,count/8+1);

         sizeall = 0;
         for(i = 0; i < count; i++)
            sizeall += getItemTextSize(hwnd,LB_CDDBSERVERS,i)+1;

         firstserver = server = (char*) alloca(++sizeall);

         for(i = 0; i < count; i++)
         {
            sizeone = getItemTextSize(hwnd,LB_CDDBSERVERS,i)+1;
            getItemText(hwnd, LB_CDDBSERVERS, i, server, sizeone);
            *(selected+i/8) |= (getSelectItem(hwnd, LB_CDDBSERVERS, i-1) == i) << i%8;
            server += sizeone;
         }
         *server = '\0';

         writeProfile(inifile,"CDDB","Servers", firstserver, sizeall);
         writeProfile(inifile,"CDDB","Selected servers", selected, count/8+1);
      }

      count = getItemCount(hwnd, LB_HTTPCDDBSERVERS);
      if(count)
      {
         char *firstserver, *server;
         int sizeall, sizeone;
         char *selected;

         selected = (char*) alloca(count/8+1);
         memset(selected,0,count/8+1);

         sizeall = 0;
         for(i = 0; i < count; i++)
            sizeall += getItemTextSize(hwnd,LB_HTTPCDDBSERVERS,i)+1;

         firstserver = server = (char*) alloca(++sizeall);

         for(i = 0; i < count; i++)
         {
            sizeone = getItemTextSize(hwnd,LB_HTTPCDDBSERVERS,i)+1;
            getItemText(hwnd, LB_HTTPCDDBSERVERS, i, server, sizeone);
            *(selected+i/8) |= (getSelectItem(hwnd, LB_HTTPCDDBSERVERS, i-1) == i) << i%8;
            server += sizeone;
         }
         *server = '\0';

         writeProfile(inifile,"CDDB","HTTP servers", firstserver, sizeall);
         writeProfile(inifile,"CDDB","Selected HTTP servers", selected, count/8+1);
      }

      getText(hwnd, EF_PROXY, buffer, sizeof(buffer));
      writeProfile(inifile,"CDDB","Proxy", buffer,0);
      getText(hwnd, EF_FORMAT, buffer, sizeof(buffer));
      writeProfile(inifile,"CDDB","Format", buffer,0);
      getText(hwnd, EF_FALLBACKFF, buffer, sizeof(buffer));
      writeProfile(inifile,"CDDB","Fallback format", buffer,0);
      getText(hwnd, EF_EMAIL, buffer, sizeof(buffer));
      writeProfile(inifile,"CDDB","E-Mail", buffer,0);
      *buffer = getCheck(hwnd, CB_USEHTTP);
      writeProfile(inifile,"CDDB","Use HTTP", buffer, 1);
      *buffer = getCheck(hwnd, CB_TRYALL);
      writeProfile(inifile,"CDDB","Try All", buffer, 1);

      closeProfile(inifile);
   }
}


static MRESULT processControl(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
/*   switch(SHORT2FROMMP(mp1))
	{
   } */

	return 0;
}


static MRESULT processCommand(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
	switch(SHORT1FROMMP(mp1))
	{
      char buffer[512];
      int  nextitem;

      case PB_ADD1:
         getText(hwnd, EF_NEWSERVER, buffer, sizeof(buffer));
         if(*buffer)
            insertItemText(hwnd, LB_CDDBSERVERS, LIT_END, buffer);
         break;

      case PB_ADD2:
         getText(hwnd, EF_NEWSERVER, buffer, sizeof(buffer));
         if(*buffer)
            insertItemText(hwnd, LB_HTTPCDDBSERVERS, LIT_END, buffer);
         break;

      case PB_DELETE1:
         nextitem = getSelectItem(hwnd, LB_CDDBSERVERS, LIT_FIRST);
         while(nextitem != LIT_NONE)
         {
            deleteItem(hwnd, LB_CDDBSERVERS, nextitem);
            nextitem = getSelectItem(hwnd, LB_CDDBSERVERS, LIT_FIRST);
         }
         break;

      case PB_DELETE2:
         nextitem = getSelectItem(hwnd, LB_HTTPCDDBSERVERS, LIT_FIRST);
         while(nextitem != LIT_NONE)
         {
            deleteItem(hwnd, LB_HTTPCDDBSERVERS, nextitem);
            nextitem = getSelectItem(hwnd, LB_HTTPCDDBSERVERS, LIT_FIRST);
         }
         break;

      case PB_UPDATE:
         WinPostMsg(workerhwnd,DLGCDDB_UPDATE,(void *)hwnd,NULL);
         break;
   }
	return 0;
}


static CDDB_socket *cddbSocket = NULL;
static char *httpservers = NULL;
static char *cddbservers = NULL;


void dlgcddb_update(void *mp1, void **mp2)
{
   HWND hwnd = (HWND) mp1;
   char server[512];
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
         updateStatus("Requesting Sites from CDDB Server %s", server);
         if(!cddbSocket->sites_req())
         {
            if(cddbSocket->getSocketError() == SOCEINTR)
               goto end;
            else
               continue;
         }

         updateStatus("Reading Sites from CDDB Server %s", server);
         int newcddbsize = 1;
         cddbservers = (char*)malloc(1);
         int newhttpsize = 1;
         httpservers = (char*)malloc(1);
         int newservertype;
         while(newservertype = cddbSocket->get_sites_req(server, sizeof(server)))
         {
            if(newservertype == CDDB)
            {
               newcddbsize += strlen(server)+1;
               cddbservers = (char*)realloc(cddbservers, newcddbsize);
               strcpy(cddbservers+newcddbsize-strlen(server)-2,server);
            }
            else if(newservertype == HTTP)
            {
               newhttpsize += strlen(server)+1;
               httpservers = (char*)realloc(httpservers, newhttpsize);
               strcpy(httpservers+newhttpsize-strlen(server)-2,server);
            }
         }
         cddbservers[newcddbsize-1] = 0;
         httpservers[newhttpsize-1] = 0;
      }
   } while(serverType != NOSERVER);

   end:

   updateStatus("");
   delete cddbSocket; cddbSocket = NULL;
}

void dlgcddb_update2(void *mp1, void *mp2)
{
   HWND hwnd = (HWND) mp1;

   char *curserver = cddbservers;
   while(curserver && *curserver)
   {
      if(searchItemText(hwnd, LB_CDDBSERVERS, LIT_FIRST, curserver) < 0)
         insertItemText(hwnd, LB_CDDBSERVERS, LIT_END, curserver);
      curserver = strchr(curserver,0)+1;
   }
   free(cddbservers); cddbservers = NULL;

   curserver = httpservers;
   while(curserver && *curserver)
   {
      if(searchItemText(hwnd, LB_HTTPCDDBSERVERS, LIT_FIRST, curserver) < 0)
         insertItemText(hwnd, LB_HTTPCDDBSERVERS, LIT_END, curserver);
      curserver = strchr(curserver,0)+1;
   }
   free(httpservers); httpservers = NULL;
}


MRESULT EXPENTRY wpCDDB(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
	switch (msg)
	{
		case WM_INITDLG:
		{
         setText(hwnd, EF_FORMAT, "%k-%t");
         setText(hwnd, EF_FALLBACKFF, "%l_%k");
         loadIni(hwnd);
         return 0;
		}
		case WM_COMMAND:
			return processCommand(hwnd,mp1,mp2);
		case WM_CONTROL:
			return processControl(hwnd,mp1,mp2);

      case WM_CLOSE:
         saveIni(hwnd);
			return 0;

		case WM_CHAR:
			if(SHORT2FROMMP(mp2) == VK_ESC)
				return 0;
			else
				break;

      case DLGCDDB_UPDATE:
         dlgcddb_update2(mp1,mp2);
         return 0;
	}

	return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

