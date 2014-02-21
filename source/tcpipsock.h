/* TCP/IP functions (C) 1998-1999 Samuel Audet <guardia@cam.org> */

class tcpip_socket
{
   public:
     tcpip_socket();
	  ~tcpip_socket();

      bool connect(int setsock);
      bool connect(char *address, int port);
      bool close();
      bool cancel() {return !so_cancel(sock); };

      char *gets(char *buffer, int size);
      int read(char *buffer, int size);
      int write(char *buffer, int size);

      bool isOnline()      { return online; };
      int getSocketError() { return socket_error; };
      int getTcpipError()  { return tcpip_error;  };

   protected:
      /* socket */
      int sock;
      bool online;

      int socket_error;
      int tcpip_error;
};
