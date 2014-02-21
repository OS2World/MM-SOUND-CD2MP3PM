/* CDDB functions (C) 1998-2001 Samuel Audet <guardia@cam.org> */

#define COMMAND_ERROR 0
#define COMMAND_OK	 1
#define COMMAND_MORE  2 /* more to read or write with appropriate member functions */

#define PROGRAM	 "CD2MP3PM"
#define VERSION    "1.14"
#define PROTOLEVEL 4

/* for get_sites_req() */
#define NOSERVER 0
#define CDDB	  1
#define HTTP	  2


typedef struct
{
	char category[24];
	char title[80];
	unsigned long discid;
} CDDBQUERY_DATA;


class CDDB_socket: public tcpip_socket
{
	public:
	  CDDB_socket();
	  ~CDDB_socket();

	  unsigned long discid(CD_drive *cdDrive);
	  int banner_req();
	  int handshake_req(char *username, char *hostname);

	  int query_req(CD_drive *cdDrive, CDDBQUERY_DATA *output);
	  bool get_query_req(CDDBQUERY_DATA *output); /* returns one match after the other */

	  int read_req(char *category, unsigned long discid);
	  char *get_disc_title(int which) { return disctitle[which]; };
	  char *get_track_title(int track, int which) { return titles[track][which]; };

	  bool sites_req();
	  int get_sites_req(char *server, int size);

	  char *gets_content(char *buffer, int size); /* read line of text from content */

	  bool isContent() { return content; };

	  /* used for HTTP kludge */
	  bool connect(char *http_URL, char *proxy_URL, char *path);
	  char *gets(char *buffer, int size);
	  int write(char *buffer, int size);
	  bool close();
	  bool cancel();

	protected:

	  /* this is inefficient for read memory access, but the database imposes
		 no rule on the order of the tracks, so it would be getting ugly to
		 write.	filled after a read_req() */
	  char *disctitle[3];	/* artist, disc title and extended stuff about disc */
	  char *titles[256][2]; /* [0][0] = title of first track
								[0][1] = extended stuff of first track */
	  /* frees above */
	  void free_junk();

	  /* tells if a read should succeed */
	  bool content;

	  /* fallback server response processor */
	  int process_default(char *response);

	  /* CDDB server protocol level */
	  bool change_protolevel();
	  int protolevel;

	  /* used for HTTP kludge */
	  HTTP_socket *httpsocket;
	  char *CGI; /* path to CGI script */
	  char *username, *hostname; /* for hello */
};
