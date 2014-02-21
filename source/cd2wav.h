typedef struct
{
   HWND hwnd;
   GRABBER *grabber;
   char *wavname;
   char *custom;
   int track;
   BOOL lowprio;
   char cddrive;
} CD2WAV_IN;

/* rename after grabber termination */
typedef struct
{
   char *wavname;
   char *oldname;
} CD2WAV_END;

HAPP cd2wav(CD2WAV_IN *input, CD2WAV_END *output);
BOOL cd2wav_end(CD2WAV_END *input);
