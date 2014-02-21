#ifndef __cplusplus
	#define inline _Inline	// that's for VAC++, use static at worst
#endif

char *getEXEpath(void);
char *strip(char *something);
char *uncomment(char *something);
char *translateChar(char *string, char to[], char from[]);
char *LFN2SFN(char *LFN, char *SFN);

typedef int bool;
#define true 1
#define false 0

inline char *makeValidLFN(char *LFN)
{
   translateChar(LFN, "________    __________________'--;()!-!",
                      "\a\b\t\v\f\r\"*/:<>?\\|");
   return LFN;
}

