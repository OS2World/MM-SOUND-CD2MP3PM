typedef struct
{
   HWND hwnd;
   ENCODER *encoder;
   char *wavfile;
   char *mp3dir;
   char *custom;
   int bitrate;
   BOOL hq;
   BOOL lowprio;
} WAV2MP3_IN;

typedef struct
{
/* rename after encoder termination */
   char *renamefrom[3];
   char *renameto[3];

/* mp3file file generated if the encoder supports output */
/* for ID3 tag smacker */
   char *mp3file;
} WAV2MP3_END;

/* HAPP wav2mp3(WAV2MP3_IN *input, WAV2MP3_END *output); changed for encoder info tagging - kiwi */
HAPP wav2mp3(WAV2MP3_IN *input, WAV2MP3_END *output, CDTRACKRECORD *record);
BOOL wav2mp3_end(WAV2MP3_END *input);

