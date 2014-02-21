/* PM stuff (C) 1998 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#include <os2.h>

HINI openProfile(char *filename)
{
   return PrfOpenProfile(WinQueryAnchorBlock(HWND_DESKTOP),filename);
}

/* with string buffer, we don't need to have an exact size match */
ULONG readProfile(HINI inifile, char *app, char *key, void *buffer,
                    ULONG buffersize, BOOL stringbuffer)
{
   ULONG datasize;

   if(stringbuffer)
      return PrfQueryProfileString(inifile, app, key, "", buffer, buffersize)-1;
   else
   {
      PrfQueryProfileSize(inifile, app, key, &datasize);
      if (datasize == buffersize)
      {
         PrfQueryProfileData(inifile, app, key, buffer, &datasize);
         return datasize;
      }
      else
         return 0;
   }
}

ULONG getProfileSize(HINI inifile, char *app, char *key)
{
   ULONG datasize = 0;
   PrfQueryProfileSize(inifile, app, key, &datasize);
   return datasize;
}

/* buffersize = 0 means a null terminated string */
BOOL writeProfile(HINI inifile, char *app, char *key, void *buffer, ULONG buffersize)
{
   if(buffersize)
      return PrfWriteProfileData(inifile, app, key, buffer, buffersize);
   else if(buffer && *(char *)buffer)
      return PrfWriteProfileString(inifile, app, key, (char*)buffer);
   else
      return PrfWriteProfileString(inifile, app, key, NULL);
}

BOOL closeProfile(HINI inifile)
{
   return PrfCloseProfile(inifile);
}
