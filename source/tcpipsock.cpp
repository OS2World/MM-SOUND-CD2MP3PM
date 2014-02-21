/* TCP/IP functions (C) 1998-1999 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#include <os2.h>
#include <stdio.h>
#include <io.h>
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

tcpip_socket::~tcpip_socket()
{
   close();
}

tcpip_socket::tcpip_socket()
{
   memset(this, 0, sizeof(*this));
}

bool tcpip_socket::connect(int setsock)
{
   sock = setsock;
   online = true; /* let's say we are */
   return online;
}

bool tcpip_socket::connect(char *address, int port)
{
   struct sockaddr_in server;

   socket_error = 0;
   tcpip_error = 0;

   if(online) this->close();

   sock = socket(PF_INET, SOCK_STREAM, 0);

   memset(&server,0,sizeof(server));
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = inet_addr(address);
   if(server.sin_addr.s_addr == -1)
   {
	  struct hostent *hp = gethostbyname(address);
	  if(!hp)
	  {
         tcpip_error = tcp_h_errno();
		 socket_error = sock_errno();

		 if(tcpip_error > 0)
            updateError("TCP/IP Error: %s", h_strerror(tcpip_error));
		 else if(socket_error > 0)
		 {
            if(socket_error != SOCEINTR)
               updateError("Socket Error: %s", sock_strerror(socket_error));
		 }
		 else
            updateError("Unknown error");
		 return false;
	  }
	  else
         server.sin_addr.s_addr = *((unsigned long *) hp->h_addr);
   }
   server.sin_port = htons(port);

/* bind doesn't work? kinda weird when you think it's should be called */
//   if( !bind(sock, (struct sockaddr*) &server, sizeof(server)) &&
//       !::connect(sock, (struct sockaddr*) &server, sizeof(server)))
   if(!::connect(sock, (struct sockaddr*) &server, sizeof(server)))
	  online = true;
   else
   {
	  online = false;
	  socket_error = sock_errno();
	  if(socket_error != SOCEINTR)
         updateError("Socket Error: %s", sock_strerror(socket_error));
   }

   return online;
}

bool tcpip_socket::close()
{
   online = false;
   return !soclose(sock);
}

char *tcpip_socket::gets(char *buffer, int size)
{
   int pos = 0;
   int stop = 0;
   int read;

   while(pos < size-1 && !stop)
   {
      read = this->read(buffer+pos, 1);
      if(read == 0 || read == -1) stop = 1;
      else
      {
         if(buffer[pos] == '\n') stop = 1;
         pos += read;
      }
   }

   buffer[pos] = '\0';

   if(read == -1)
      return NULL;
   else
	  return buffer;
}

int tcpip_socket::write(char *buffer, int size)
{
   int pos = 0;
   int stop = 0;
   int sent;

   socket_error = 0;

   while(pos < size && !stop)
   {
	  sent = send(sock, buffer+pos, size-pos, 0);
      if(sent == 0)
      {
         online = false;
         stop = 1;
      }
	  else if(sent == -1)
	  {
         online = false;
         stop = 1;
         socket_error = sock_errno();
         if(socket_error != SOCEINTR)
            updateError("Socket Error: %s", sock_strerror(socket_error));
      }
      else
         pos += sent;
   }

   ::write(1, buffer, size);

   if(sent == -1)
	  return -1;
   else
	  return pos;
}

int tcpip_socket::read(char *buffer, int size)
{
   int read;

   socket_error = 0;

   read = recv(sock, buffer, size, 0);
   if(read == 0) online = false;
   else if(read == -1)
   {
      online = false;
      socket_error = sock_errno();
      if(socket_error != SOCEINTR)
         updateError("Socket Error: %s", sock_strerror(socket_error));
   }

   ::write(1, buffer, read);

   return read;
}
