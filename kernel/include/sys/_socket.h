#pragma once

#include <lib/types.h>
#include <lib/stddef.h>
#include <sys/_uio.h>

typedef  int            socklen_t;
typedef  unsigned int   sa_family_t;

struct   sockaddr {
   sa_family_t  sa_family;  // Address family. 
   char         sa_data[];  // Socket address (variable-length data).
};

struct sockaddr_storage {
   sa_family_t   ss_family;
};

struct msghdr {
   void          *msg_name;        // Optional address. 
   socklen_t      msg_namelen;     // Size of address.
   struct iovec  *msg_iov;         // Scatter/gather array.
   int            msg_iovlen;      // Members in msg_iov.
   void          *msg_control;     // Ancillary data; see below.
   socklen_t      msg_controllen;  // Ancillary data buffer len.
   int            msg_flags;       // Flags on received message
};

struct cmsghdr {
   socklen_t  cmsg_len;    // Data byte count, including the cmsghdr. 
   int        cmsg_level;  // Originating protocol. 
   int        cmsg_type;   // Protocol-specific type. 
};

struct linger {
   int  l_onoff;   // Indicates whether linger option is enabled. 
   int  l_linger;  // Linger time, in seconds.
};

#define SOCK_DGRAM      // Datagram socket.
#define SOCK_RAW        // [RS] [Option Start] Raw Protocol Interface. [Option End]
#define SOCK_SEQPACKET  // Sequenced-packet socket.
#define SOCK_STREAM     // Byte-stream socket.

#define SOL_SOCKET      // Options to be accessed at socket level, not protocol level.

#define SO_ACCEPTCONN   // Socket is accepting connections.
#define SO_BROADCAST    // Transmission of broadcast messages is supported.
#define SO_DEBUG        // Debugging information is being recorded.
#define SO_DONTROUTE    // Bypass normal routing.
#define SO_ERROR        // Socket error status.
#define SO_KEEPALIVE    // Connections are kept alive with periodic messages.
#define SO_LINGER       // Socket lingers on close.
#define SO_OOBINLINE    // Out-of-band data is transmitted in line.
#define SO_RCVBUF       // Receive buffer size.
#define SO_RCVLOWAT     // Receive ``low water mark''.
#define SO_RCVTIMEO     // Receive timeout.
#define SO_REUSEADDR    // Reuse of local addresses is supported.
#define SO_SNDBUF       // Send buffer size.
#define SO_SNDLOWAT     // Send ``low water mark''.
#define SO_SNDTIMEO     // Send timeout.
#define SO_TYPE         // Socket type.

#define SOMAXCONN       // The maximum backlog queue length.

#define MSG_CTRUNC      //Control data truncated.
#define MSG_DONTROUTE   //Send without using routing tables.
#define MSG_EOR         //Terminates a record (if supported by the protocol).
#define MSG_OOB         //Out-of-band data.
#define MSG_NOSIGNAL    //No SIGPIPE generated when an attempt to send is made on a stream-oriented socket that is no longer connected.
#define MSG_PEEK        //Leave received data in queue.
#define MSG_TRUNC       //Normal data truncated.
#define MSG_WAITALL     //Attempt to fill the read buffer.

#define AF_UNSPEC       //Unspecified.
#define AF_INET         //Internet domain sockets for use with IPv4 addresses.
#define AF_INET6        //[IP6] [Option Start] Internet domain sockets for use with IPv6 addresses. [Option End]
#define AF_UNIX         //UNIX domain sockets.

#define __CONST_SOCKADDR_ARG const struct sockaddr *

/* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  */
extern int socket(int __domain, int __type, int __protocol);

/* Create two new sockets, of type TYPE in domain DOMAIN and using
   protocol PROTOCOL, which are connected to each other, and put file
   descriptors for them in FDS[0] and FDS[1].  If PROTOCOL is zero,
   one will be chosen automatically.  Returns 0 on success, -1 for errors.  */
extern int socketpair (int __domain, int __type, int __protocol,
		      int __fds[2]);

/* Give the socket FD the local address ADDR (which is LEN bytes long).  */
extern int bind (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);

/* Put the local address of FD into *ADDR and its length in *LEN.  */
extern int getsockname (int __fd, struct sockaddr *__addr,
			socklen_t *__restrict __len);

/* Open a connection on socket FD to peer at ADDR (which LEN bytes long).
   For connectionless socket types, just set the default address to send to
   and the only address from which to accept transmissions.
   Return 0 on success, -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int connect (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);

/* Put the address of the peer connected to socket FD into *ADDR
   (which is *LEN bytes long), and its actual length into *LEN.  */
extern int getpeername (int __fd, struct sockaddr *__addr,
			socklen_t *__restrict __len);


/* Send N bytes of BUF to socket FD.  Returns the number sent or -1.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t send (int __fd, const void *__buf, size_t __n, int __flags);

/* Read N bytes into BUF from socket FD.
   Returns the number read or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t recv (int __fd, void *__buf, size_t __n, int __flags);