/* Geez, why are WAV RIFF headers are so secret?  I got something together,
   but wow...  anyway, I hope someone will find this useful.
   - Samuel Audet <guardia@cam.org> */

typedef struct
{
   char R1;
   char I2;
   char F3;
   char F4;
   unsigned long WAVElen;
   struct
   {
      char W1;
      char A2;
      char V3;
      char E4;
      char f5;
      char m6;
      char t7;
      char space;
      unsigned long fmtlen;
      struct
      {
         unsigned short FormatTag;
         unsigned short Channels;
         unsigned long SamplesPerSec;
         unsigned long AvgBytesPerSec;
         unsigned short BlockAlign;
         unsigned short BitsPerSample; /* format specific for PCM */
      } fmt;
      char d8;
      char a9;
      char t10;
      char a11;
      unsigned long datalen;
      /* from here you insert your PCM data */
   } WAVE;
} RIFF;

/* warning: append mode doesn't take in account unknown RIFF chunks */
typedef enum { READ, CREATE, APPEND } MODE;

/* AFAIK, all of those also have BitsPerSample as format specific */
#define WAVE_FORMAT_PCM       0x0001
#define WAVE_FORMAT_ADPCM     0x0002
#define WAVE_FORMAT_ALAW      0x0006
#define WAVE_FORMAT_MULAW     0x0007
#define WAVE_FORMAT_OKI_ADPCM 0x0010
#define WAVE_FORMAT_DIGISTD   0x0015
#define WAVE_FORMAT_DIGIFIX   0x0016
#define IBM_FORMAT_MULAW      0x0101
#define IBM_FORMAT_ALAW       0x0102
#define IBM_FORMAT_ADPCM      0x0103

#define NO_WAV_FILE 1000

int openWAV(char *filename, MODE mode, RIFF *header, int *samplerate,
         int *channels, int *bits, int *format, int *size);
int closeWAV(int hfile);
int readData(int hfile, char *buffer, int bytes);
int writeData(int hfile, RIFF *header, char *buffer, int bytes);
