/* HTTP functions (C) 1998-1999 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <sys\socket.h>
#include <netinet\in.h>
#include <netdb.h>
#include <nerrno.h>

#include "pmsam.h"
#include "nerrno.h"
#include "miscsam.h"
#include "tcpipsock.h"
#include "http.h"

HTTP_socket::HTTP_socket()
{
   memset(this, 0, sizeof(*this));
}

HTTP_socket::~HTTP_socket()
{
   free(response_response.proxy_auth);  response_response.proxy_auth = 0;
   free(response_response.www_auth);    response_response.www_auth = 0;
   free(response_response.location);    response_response.location = 0;
   free(response_response.retry_after); response_response.retry_after = 0;
   free(http_host); http_host = 0;
   close();
}

bool HTTP_socket::connect(char *http_URL, char *proxy_URL)
{
    char host[512]="", *colon;
	int port;

	/* setting the HTTP parameters in the class variables */

    sscanf(http_URL,"http://%511[^/]",host);
	if(colon = strchr(host,':'))
	{
	   *colon++ = '\0';  /* remove the : */
	   port = atoi(colon);
	}
	else
	   port = 80;

    if(!*host)
    {
       updateError("HTTP Error: Invalid URL: %s", http_URL);
       return false;
    }

    http_host = (char*)realloc(http_host,strlen(host)+1);
	strcpy(http_host,host);
	http_port = port;

    /* deciding wether to connect to a proxy or to the HTTP server itself */

    if(proxy_URL && *proxy_URL)
	{
       host[0] = 0;
	   proxy = true;
	   sscanf(proxy_URL,"http://%511[^/]",host);
	   if(colon = strchr(host,':'))
	   {
		  *colon++ = '\0';  /* remove the : */
		  port = atoi(colon);
	   }
	   else
		  port = 80;

       if(!*host)
       {
          updateError("HTTP Error: Invalid Proxy URL: %s", proxy_URL);
          return false;
       }
       else
          return tcpip_socket::connect(host, port);
	}
	else
	{
	   proxy=false;
	   return tcpip_socket::connect(http_host, http_port);
    }
}

bool HTTP_socket::new_request(char *method, char *URI,
				 GENERAL_HEADER *req_general, REQUEST_HEADER *req_req)
{
   char buffer[512], *temp;

   status_code = major = minor = 0;
   dataleft = 0;
   /* defaults */
   response_general.connection = persistent;
   response_general.transfer = none;
   free(response_response.proxy_auth);	response_response.proxy_auth = 0;
   free(response_response.www_auth);	response_response.www_auth = 0;
   free(response_response.location);	response_response.location = 0;
   free(response_response.retry_after); response_response.retry_after = 0;
   response_entity.length = 0;

   /* send request line */
   sprintf(buffer,"%s %s HTTP/1.1\n",method, URI);
   if(write(buffer,strlen(buffer)) < 1)
      return false;

   switch(req_general->connection)
   {
      case non_persistent:
		 temp = "Connection: close\n";
         if(write(temp, strlen(temp)) < 1)
            return false;
         break;

	  case persistent:
		 /* HTTP/1.1 is persistent by default, but this can
			make HTTP/1.0 server work in persistent mode */
		 temp = "Connection: Keep-Alive\n";
         if(write(temp, strlen(temp)) < 1)
            return false;
         break;
   }

   if(req_req->host)
   {
      sprintf(buffer,"Host: %s\n",req_req->host);
      if(write(buffer,strlen(buffer)) < 1)
         return false;
   }

   if(req_req->user_agent)
   {
      sprintf(buffer,"User-Agent: %s\n",req_req->user_agent);
      if(write(buffer,strlen(buffer)) < 1)
         return false;
   }

   if(write("\n",1) < 1) /* empty line indicates end of headers */
      return false;

   return true;
}

/* header from response,
   need to add support for comma seperated list over multiple lines */
bool HTTP_socket::process_headers()
{
   char header[512] = {0}, name[80] = {0}, value[512] = {0};
   bool content = false;

   while(gets(header, sizeof(header)) && *header != '\n' && *header != '\r')
   {
      sscanf(header,"%79[^:]: %511[^\r\n]\r\n",name,value);
	  strip(value);

	  if(!stricmp(name,"Connection"))
	  {
		 if(!stricmp(value,"close"))
		 {
            content = true; /* we must assume a content that
                               ends on connection close */
            response_general.connection = non_persistent;
		 }
	  }
	  else if(!stricmp(name,"Transfer-Encoding"))
	  {
		 if(!stricmp(value,"chunked"))
		 {
            content = true;
			response_general.transfer = chunked;
		 }
	  }
	  else if(!stricmp(name,"Proxy-Authenticate"))
	  {
         response_response.proxy_auth = (char*)realloc(response_response.proxy_auth, strlen(value)+1);
		 strcpy(response_response.proxy_auth, value);
	  }
	  else if(!stricmp(name,"WWW-Authenticate"))
	  {
         response_response.www_auth = (char*)realloc(response_response.www_auth, strlen(value)+1);
		 strcpy(response_response.www_auth, value);
	  }
	  else if(!stricmp(name,"Location"))
	  {
         response_response.location = (char*)realloc(response_response.location, strlen(value)+1);
		 strcpy(response_response.location, value);
	  }
	  else if(!stricmp(name,"Retry-After"))
	  {
         response_response.retry_after = (char*)realloc(response_response.retry_after, strlen(value)+1);
		 strcpy(response_response.retry_after, value);
	  }
	  else if(!stricmp(name,"Content-Length"))
	  {
         content = true;
		 /* assume bytes */
		 response_entity.length = atoi(value);
	  }
   }

   return content; /* we think there might or might not be a entity-body from
					  what we have seen from the headers */
}

void HTTP_socket::build_URI(char *out, char *path)
{
   if(proxy)
	  sprintf(out,"http://%s:%d%s",http_host,http_port,path);
   else
	  strcpy(out,path);
}


int HTTP_socket::get_req(CONNECTION connection, char *path)
{
   char URI[2048];
   char message[512]; /* from the status line */
   char buffer[512];

   GENERAL_HEADER req_general;
   REQUEST_HEADER req_req;

   req_general.connection = connection;
   req_req.user_agent = USER_AGENT;
   req_req.host = http_host;

   build_URI(URI, path);
   if(!new_request("GET", URI, &req_general, &req_req))
      return -1;

readagain:

   /* getting status line */
   do
      if(!gets(buffer, sizeof(buffer)))
         return -1;
   while(*buffer == '\n' || *buffer == '\r');
   sscanf(buffer, "HTTP/%d.%d %d %511[^\r\n]\r\n",&major,&minor,&status_code,message);

   if(process_headers())
   {
	  if(response_general.transfer == chunked)
		 dataleft = -1; /* and the read() function will have to do the rest */
	  else if(response_entity.length)
		 dataleft = response_entity.length;
	  else if(response_general.connection != persistent)
		 dataleft = -1;
   }
   else
	  dataleft = 0;

   switch(status_code / 100)
   {
	  case 1: /* who cares */
		 goto readagain;
		 break;

	  case 2:
		 switch(status_code % 100)
		 {
			default: /* assume as 200 */
			case 0:
			   break; /* done */
			case 2:
               updateError("HTTP Error: %s", message);
			   break; /* processing delayed, find more in entity body */
			case 4:
               updateError("HTTP Error: %s", message);
               dataleft = 0; /* no content */
			   break; /* nothin in entity because unnecessary */
            case 6:
			   break; /* range thing ok, done */
		 }
         break;

	  case 3:
		 switch(status_code % 100)
		 {
			default: /* assume as 300 */
			case 0:
			case 1:
			case 2:
			case 3:
			   /* do it automagically ? new location */
			   if(response_response.location)
                  updateError("HTTP Error: Retry at %s", response_response.location);
			   break; /* check entity for more */
			case 4:
               updateError("HTTP Error: %s", message);
               dataleft = 0; /* no content */
			   break; /* nothin is returned because unnecessary */
			case 5:
			   /* do it automagically ? need to pass through proxy */
			   if(response_response.location)
                  updateError("HTTP Error: Retry with proxy at %s", response_response.location);
			   break;
		 }
         break;

	  case 4:
		 switch(status_code % 100)
		 {
			default: /* assume as 400 */
			case 0:
               updateError("HTTP Error: %s", message);
			   break; /* bad request */
			case 1:
               if(response_response.www_auth)
                  updateError("HTTP Error: Challenge is %s", response_response.www_auth);
			   break;
			case 3:
               updateError("HTTP Error: %s", message);
			   break; /* access denied, display entity */
			case 4:
               updateError("HTTP Error: %s", message);
			   break; /* not found */
			case 5:
               updateError("HTTP Error: %s", message);
			   break; /* method not allowed */
			case 6:
               updateError("HTTP Error: %s", message);
			   break; /* can't fufill request with these accept headers, see entity */
			case 7:
               if(response_response.proxy_auth)
                  updateError("HTTP Error: Proxy challenge is %s", response_response.proxy_auth);
			   break;
			case 8:
               updateError("HTTP Error: %s", message);
			   break; /* request timeout */
			case 9:
               updateError("HTTP Error: %s", message);
			   break; /* conflict? more info in entity */
			case 10:
               updateError("HTTP Error: %s", message);
			   break; /* gone */
			case 11:
               updateError("HTTP Error: %s", message);
			   break; /* missing length, shouldn't happen */
			case 12:
               updateError("HTTP Error: %s", message);
			   break; /* precondition failed */
			case 13:
               updateError("HTTP Error: %s", message);
			   break; /* entity too large, look Retry-After */
			case 14:
               updateError("HTTP Error: %s", message);
			   break; /* URI too long */
			case 15:
               updateError("HTTP Error: %s", message);
			   break; /* unsupported media type */
		 }
         break;

	  case 5:
		 switch(status_code % 100)
		 {
			default: /* assume as 500 */
			case 0:  /* shit happens */
               updateError("HTTP Error: %s", message); break;
			case 1:  /* not implemented */
               updateError("HTTP Error: %s", message); break;
			case 2:  /* bad gateway */
               updateError("HTTP Error: %s", message); break;
			case 3:  /* service unavailable, check Retry-After */
               updateError("HTTP Error: %s", message); break;
			case 4: /* gateway timeout */
               updateError("HTTP Error: %s", message); break;
			case 5: /* HTTP version not supported */
               updateError("HTTP Error: %s", message); break;
		 }
         break;

	  default:
         updateError("HTTP Error: Unknown error.");
		 break;
   }

   return status_code;
}

char *HTTP_socket::gets_content(char *buffer, int size)
{
   int pos = 0;
   int stop = 0;
   int read;

   while(pos < size-1 && !stop)
   {
	  read = read_content(buffer+pos, 1);
	  if(read == 0 || read == -1) stop = 1;
      else if(buffer[pos] == '\n') stop = 1;
	  pos += read;
   }

   buffer[pos] = '\0';

   if(read == -1)
      return NULL;
   else
	  return buffer;
}

int HTTP_socket::read_content(char *out, int size)
{
   char buffer[512];
   int read = 0;

   if(dataleft == -1 && response_general.transfer == chunked)
   {
      if(!gets(buffer, sizeof(buffer)))
         return -1;
      sscanf(buffer, "%x%*[^\r\n]\r\n", &dataleft);
   }

   if(!dataleft) return 0;
   else if(dataleft > 0 && size > dataleft) size = dataleft;

   if(dataleft != 0) /* this let's -1 pass too */
   {
	  socket_error = 0;

      read = this->read(out, size);
      if(read > 0)
		 dataleft -= read;
   }

   if(dataleft == 0 && response_general.transfer == chunked)
   {
      if( !gets(buffer, sizeof(buffer)) || /* empty line !? */
          !gets(buffer, sizeof(buffer))   )
         return -1;

	  if(*buffer == '0') /* == end of chunk */
		 process_headers();
	  dataleft = -1;
   }

   return read; /* when read returns 0, means all is read */
}
