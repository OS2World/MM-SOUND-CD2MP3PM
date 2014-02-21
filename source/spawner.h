typedef struct
{
   char *program;
   char *cmdline;
   USHORT sessionType;
   ULONG prioClass;
   LONG prioDelta;
   HWND hwnd;
} SPAWNER_PARAM;

HAPP doWin32OS2(SPAWNER_PARAM *input);
HAPP doNormal(SPAWNER_PARAM *input);
