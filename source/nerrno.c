/* Uses error constants in nerror.h (16bit stack, the 32 bit stack
   already has a sock_strerror() ) for TCP/IP in OS/2 and returns
   the string in the comments */

#if defined(__IBMC__) || defined(__IBMCPP__)
  #include <nerrno.h>
#else
  #include <sys/errno.h>
#endif

#include <string.h>
#include <netdb.h>


char *sock_strerror(int socket_errno)
{
   switch(socket_errno)
   {
#if defined(__IBMC__) || defined(__IBMCPP__)
      case SOCEPERM       : return "Not owner";
      case SOCESRCH       : return "No such process";
      case SOCEINTR       : return "Interrupted system call";
      case SOCENXIO       : return "No such device or address";
      case SOCEBADF       : return "Bad file number";
      case SOCEACCES      : return "Permission denied";
      case SOCEFAULT      : return "Bad address";
      case SOCEINVAL      : return "Invalid argument";
      case SOCEMFILE      : return "Too many open files";
      case SOCEPIPE       : return "Broken pipe";

      case SOCEOS2ERR     : return "OS/2 Error";

      case SOCEWOULDBLOCK : return "Operation would block";
      case SOCEINPROGRESS : return "Operation now in progress";
      case SOCEALREADY    : return "Operation already in progress";
      case SOCENOTSOCK    : return "Socket operation on non-socket";
      case SOCEDESTADDRREQ: return "Destination address required";
      case SOCEMSGSIZE    : return "Message too long";
      case SOCEPROTOTYPE  : return "Protocol wrong type for socket";
      case SOCENOPROTOOPT : return "Protocol not available";
      case SOCEPROTONOSUPPORT: return "Protocol not supported";
      case SOCESOCKTNOSUPPORT: return "Socket type not supported";
      case SOCEOPNOTSUPP     : return "Operation not supported on socket";
      case SOCEPFNOSUPPORT   : return "Protocol family not supported";
      case SOCEAFNOSUPPORT   : return "Address family not supported by protocol family";
      case SOCEADDRINUSE     : return "Address already in use";
      case SOCEADDRNOTAVAIL  : return "Can't assign requested address";
      case SOCENETDOWN       : return "Network is down";
      case SOCENETUNREACH    : return "Network is unreachable";
      case SOCENETRESET      : return "Network dropped connection on reset";
      case SOCECONNABORTED   : return "Software caused connection abort";
      case SOCECONNRESET     : return "Connection reset by peer";
      case SOCENOBUFS        : return "No buffer space available";
      case SOCEISCONN        : return "Socket is already connected";
      case SOCENOTCONN       : return "Socket is not connected";
      case SOCESHUTDOWN      : return "Can't send after socket shutdown";
      case SOCETOOMANYREFS   : return "Too many references: can't splice";
      case SOCETIMEDOUT      : return "Connection timed out";
      case SOCECONNREFUSED   : return "Connection refused";
      case SOCELOOP          : return "Too many levels of symbolic links";
      case SOCENAMETOOLONG   : return "File name too long";
      case SOCEHOSTDOWN      : return "Host is down";
      case SOCEHOSTUNREACH   : return "No route to host";
      case SOCENOTEMPTY      : return "Directory not empty";
#else
      case EPERM       : return "Not owner";
      case ESRCH       : return "No such process";
      case EINTR       : return "Interrupted system call";
      case ENXIO       : return "No such device or address";
      case EBADF       : return "Bad file number";
      case EACCES      : return "Permission denied";
      case EFAULT      : return "Bad address";
      case EINVAL      : return "Invalid argument";
      case EMFILE      : return "Too many open files";
      case EPIPE       : return "Broken pipe";

//      case EOS2ERR     : return "OS/2 Error";

      case EWOULDBLOCK : return "Operation would block";
      case EINPROGRESS : return "Operation now in progress";
      case EALREADY    : return "Operation already in progress";
      case ENOTSOCK    : return "Socket operation on non-socket";
      case EDESTADDRREQ: return "Destination address required";
      case EMSGSIZE    : return "Message too long";
      case EPROTOTYPE  : return "Protocol wrong type for socket";
      case ENOPROTOOPT : return "Protocol not available";
      case EPROTONOSUPPORT: return "Protocol not supported";
      case ESOCKTNOSUPPORT: return "Socket type not supported";
      case EOPNOTSUPP     : return "Operation not supported on socket";
      case EPFNOSUPPORT   : return "Protocol family not supported";
      case EAFNOSUPPORT   : return "Address family not supported by protocol family";
      case EADDRINUSE     : return "Address already in use";
      case EADDRNOTAVAIL  : return "Can't assign requested address";
      case ENETDOWN       : return "Network is down";
      case ENETUNREACH    : return "Network is unreachable";
      case ENETRESET      : return "Network dropped connection on reset";
      case ECONNABORTED   : return "Software caused connection abort";
      case ECONNRESET     : return "Connection reset by peer";
      case ENOBUFS        : return "No buffer space available";
      case EISCONN        : return "Socket is already connected";
      case ENOTCONN       : return "Socket is not connected";
      case ESHUTDOWN      : return "Can't send after socket shutdown";
      case ETOOMANYREFS   : return "Too many references: can't splice";
      case ETIMEDOUT      : return "Connection timed out";
      case ECONNREFUSED   : return "Connection refused";
      case ELOOP          : return "Too many levels of symbolic links";
      case ENAMETOOLONG   : return "File name too long";
      case EHOSTDOWN      : return "Host is down";
      case EHOSTUNREACH   : return "No route to host";
      case ENOTEMPTY      : return "Directory not empty";
#endif
      default: return (char *) 0;
   }
}

char *h_strerror(int tcpip_errno)
{
   switch(tcpip_errno)
   {
      case HOST_NOT_FOUND  : return "Host not found";
      case TRY_AGAIN       : return "Server failure";
      case NO_RECOVERY     : return "Non-recoverable error";
      case NO_DATA         : return "Valid name, no data record of requested type";
      default: return NULL;
   }
}
