// OS2DEF.H defines it with (SHORT)
#undef FIELDOFFSET
#define FIELDOFFSET(type,field) ((ULONG)&(((type *)0)->field))

#define INIFILE "CD2MP3PM.INI"
#define CFGFILE "cd2mp3pm.cfg"

/* main window buttons */

#define CD2MP3_STOP		 100
#define CD2MP3_STOPPED	 400

#define PB_CONVERT 50
#define PB_STOP	 51
#define PB_CANCEL  52

/* for grabbers & encoders */

typedef enum {VIO, VDM, WIN32OS2} APPTYPE;

typedef struct
{
	char id[64];
	char exe[128];
	APPTYPE type;
	char input[32];
	char output[32];
	char mangling[32];
	char before[64];
	char after[64];
	BOOL acceptsOutput;
	BOOL noDrive;
} GRABBER;

typedef struct
{
	char id[64];
	char exe[128];
	APPTYPE type;
	char input[32];
	char output[32];
	char lq[64];
	char hq[64];
	char before[64];
	char after[64];
	char bitrate[32];
	int  bitrateMath;
	BOOL bitratePerChannel;
	BOOL acceptsOutput;
	/* extension for Ogg Vorbis support - ch */
	char extension[32]; /* should be enough for a file extension like "ogg" */
   /* extension for info-tagging by encoder - kiwi */
	BOOL DoesTagging;
	char artist[16];
	char genre[16];
	char year[16];
	char album[16];
	char title[16];
	char comment[16];

} ENCODER;


/* for inter-dialog communication */

extern HAB	mainhab;

USHORT getBitRate(void);
extern int bitRateChanged;
ULONG getFilenameFormat(char *buffer, LONG size);
ULONG getFallbackFF(char *buffer, LONG size);
ULONG getUserHost(char *user, int sizeuser, char *host, int sizehost);
ULONG getProxy(char *proxy, int size);

#define NOSERVER 0
#define CDDB	  1
#define HTTP	  2
ULONG getNextCDDBServer(char *server, ULONG size, SHORT *index);

/* worker thread */

extern HWND workerhwnd;

#define DLGCD_REFRESH WM_USER
#define CDDB_FUZZYMATCH WM_USER+2
void dlgcd_refresh(void *mp1, void **mp2);
void dlgcd_refresh2(void *mp1, void *mp2);

#define DLGCDDB_UPDATE WM_USER+1
void dlgcddb_update(void *mp1, void **mp2);
void dlgcddb_update2(void *mp1, void *mp2);

/* container records .. I must have had a reason to put that here */

typedef struct
{
   RECORDCORE record;
   ULONG track;    /* number */
   char type[8]; /* data or audio */
   char time[8]; /* minutes:seconds */
	char *timepointer; /* kludge */
	char *typepointer; /* kludge */
   ULONG size;     /* bytes */
   ULONG mp3size;  /* bytes */

   char  title[80];
   char  artist[80];
   char  album[80];
   char  year[8];
   char  genre[80];
   char  comment[80];
   /* kludges */
   char  *titleptr;
   char  *artistptr;
   char  *albumptr;
   char  *yearptr;
   char  *genreptr;
   char  *commentptr;

   /* does not show in container */
   BOOL data;
   USHORT minutes;
   USHORT seconds;
   ULONG channels; /* 2 or 4 */

} CDTRACKRECORD;

typedef struct
{
	RECORDCORE record;
	char type[8]; /* format in string */
	char time[8];
	char *timepointer; /* kludge */
	char *typepointer; /* kludge */
	ULONG format;
	ULONG bits;
	ULONG samplerate;
	ULONG channels;
	ULONG size;
	ULONG mp3size;
} WAVRECORD;
