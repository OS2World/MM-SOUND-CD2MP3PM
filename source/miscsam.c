/* Misc stuff (C) 1998-1999 Samuel Audet <guardia@cam.org> */

#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *getEXEpath()
{
   ULONG PathLength;
   char *Path = NULL;

   /* query max path length */

   if (!DosQuerySysInfo(QSV_MAX_PATH_LENGTH, QSV_MAX_PATH_LENGTH, &PathLength, sizeof(ULONG)))
      Path = (char *) malloc(2*PathLength+1);

   /* multiplied by 2 to include the filename length too */

   if(Path)
   {
      PPIB ppib;
      PTIB ptib;

      DosGetInfoBlocks(&ptib, &ppib);
      DosQueryModuleName(ppib->pib_hmte, 2*PathLength+1, Path);

      *(strrchr(Path,'\\')) = 0;

      Path = (char *)realloc(Path, strlen(Path)+1);
   }

   return Path;
}


/* removes comments starting with # */
char *uncomment(char *something)
{
	int source = 0;
	BOOL inquotes = FALSE;

	while(something[source])
	{
		if(something[source] == '\"')
			inquotes = !inquotes;
      else if(something[source] == '#' && !inquotes)
		{
			something[source] = 0;
			break;
		}
		source++;
	}

	return something;
}

/* remove leading and trailing spaces and quotes */
char *strip(char *something)
{
   int i,e;
   char *pos = something;

   while (*pos == ' ' || *pos == '\t' || *pos == '\n') pos++;
   i = strlen(pos) - 1;
   for (;i>0;i--) if (pos[i] != ' ' && pos[i] != '\t' && pos[i] != '\n') break;
   if(*pos == '\"' && pos[i] == '\"' )
   {
      pos++;
      i -= 2;
   }
   for(e = 0; e < i+1; e++)
      something[e] = pos[e];
   something[e] = 0;
   return(something);
}

char *translateChar(char *string, char to[], char from[])
{
   int i;
   for(i = 0; from[i]; i++)
   {
      char *pos = string;
      while(*pos)
      {
         if(*pos == from[i])
            *pos = to[i];
         pos++;
      }
   }
   return string;
}

char *LFN2SFN(char *LFN, char *SFN)
{
   char leftpart[12];
   char rightpart[4];
   char *period;

   period = strrchr(LFN,'.');
   if(!period)
   {
      strncpy(leftpart,LFN,8);
      leftpart[8] = 0;
      rightpart[0] = 0;
   }
   else if(period-LFN > 8)
   {
      strncpy(leftpart,LFN,8);
      leftpart[8] = 0;
      strncpy(rightpart,period+1,3);
      rightpart[3] = 0;
   }
   else
   {
      strncpy(leftpart,LFN,period-LFN);
      leftpart[period-LFN] = 0;
      strncpy(rightpart,period+1,3);
      rightpart[3] = 0;
   }

   /* let's remove chars that are illegal in 8.3 filenames */
   translateChar(rightpart, "__-()!-__" , ". +[];=,~");
   translateChar(leftpart, "__-()!-__" , ". +[];=,~");

   strcpy(SFN,leftpart);
   if(rightpart[0])
   {
      strcat(SFN,".");
      strcat(SFN,rightpart);
   }

   return SFN;
}
