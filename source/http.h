/* #define USER_AGENT for your program */

#define USER_AGENT "CD2MP3PM/1.14"

/* headers used internally to mimic the HTTP structure */

typedef enum { persistent, non_persistent } CONNECTION;
typedef enum { none, chunked } TRANSFER;

typedef struct
{
   CONNECTION connection;
   TRANSFER transfer;
} GENERAL_HEADER;

typedef struct
{
   char *host;	/* needed if URI doesn't contain host and
				   that happens when not sent through a proxy */
   char *user_agent;

} REQUEST_HEADER;

typedef struct
{
   char *proxy_auth;
   char *www_auth;
   char *location;
   char *retry_after;

} RESPONSE_HEADER;

typedef struct
{
   int length;
} ENTITY_HEADER;


class HTTP_socket : public tcpip_socket
{
   public:
     HTTP_socket();
	  ~HTTP_socket();

      bool connect(char *http_URL, char *proxy_URL);

      /* automated GET request */
      int get_req(CONNECTION connection, char *path);

      char *gets_content(char *buffer, int size); /* read line of text from content */
      int read_content(char *buffer, int size); /* read binary from content */
//    int write_content(char *buffer, int size); /* write anything to content */

      bool isContent() { return (dataleft != 0); };

   protected:
      /* connect() keeps info on the http host and port, and if we are on a
         proxy connection */
      char *http_host;
      int http_port;
      bool proxy;

      /* keeps count of what should be left to read in the content.
         if -1, read as chunked or till disconnected if no chunks */
      int dataleft;

      unsigned int major, minor; /* server http version */
      unsigned int status_code;  /* status code xxx */

      /* keeps information about response for reading entity-body */
      GENERAL_HEADER  response_general;
      RESPONSE_HEADER response_response;
      ENTITY_HEADER   response_entity;

      /* fills up previous structures */
      int process_headers();

      /* resets the structures to default and does a new request */
      int new_request(char *method, char *URI,
                      GENERAL_HEADER *req_general, REQUEST_HEADER *req_req);

      /* builds URI for GET request */
      void build_URI(char *out, char *path);
};
