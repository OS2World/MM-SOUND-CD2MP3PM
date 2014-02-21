HINI openProfile(char *filename);
ULONG readProfile(HINI inifile, char *app, char *key, void *buffer,
                    ULONG buffersize, BOOL stringbuffer);
BOOL writeProfile(HINI inifile, char *app, char *key, void *buffer, ULONG buffersize);
BOOL closeProfile(HINI inifile);
ULONG getProfileSize(HINI inifile, char *app, char *key);
