/* sockeng.h
 * awiebe, 2008
 */

#ifndef SOCKENG_H
#define SOCKENG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

#include "setup.h"
#include "ebuf.h"

#define MAX_FDS MAXCONNECTIONS
#define DEBUG
#define BUFSIZE 8192

/* threadsafe atomics for interacting with memory less than 8 bytes (long long) */
#define atomic_add(x, y) ((void) __sync_fetch_and_add(&(x), y))
#define atomic_sub(x, y) ((void) __sync_fetch_and_add(&(x), -(y)))
#define atomic_incr(x)   ((void) __sync_fetch_and_add(&(x), 1))
#define atomic_deincr(x) ((void) __sync_fetch_and_add(&(x), -1))

/* debug levels */
#define DL_DEBUG 0x01
#define DL_INFO  0x02
#define DL_WARN  0x04
#define DL_CRIT  0x08

/* return codes */
#define RET_OK		0	/* ok! */
#define RET_UNDEF	1	/* unknown? */
#define RET_INVAL	2	/* invalid argument */
#define RET_NOMEM	3	/* no memory! */
#define RET_NOSUCH	4	/* no such... something */
#define RET_EXISTS	5	/* stuff still exists */

/* typedefs make life easier */
typedef struct _ipvx		ipvx;		/* ip abstraction */
typedef struct _client 		Client;		/* a Client */
typedef struct _group 		Group;		/* a Group */
typedef struct _sockeng 	SockEng;	/* a Socket Engine */
typedef struct _listener 	Listener;	/* a Listener */
typedef struct _llink 		lLink;		/* link structure for listeners */
typedef struct _glink 		gLink;		/* link structure for groups */
typedef struct _clink		cLink;		/* link structure for clients */
typedef struct _mfd		myfd;		/* file descriptor abstraction */

/*
 * This structure allows us to handle IPV6 and IPV4 easily
 */
#define TYPE_NONE 0
#define TYPE_IPV4 1
#define TYPE_IPV6 2
struct _ipvx {
	short	type;
	union {
		struct in_addr *v4;
		struct in6_addr *v6;
	} ip;
};

/*
 * These structures allow us to link together listeners, clients, and groups in
 * rather infinite ways flexibly
 */

struct _llink {
	Listener 	*lr;			/* the current listener */
	int		flags;			/* flags of this association */
	lLink	 	*next, *prev, *head;	/* list navigation pointers */
};

struct _clink {
	Client 		*cl;			/* the client link */
	int		flags;			/* flags of this link status */
	cLink		*next, *prev, *head;	/* link list stuff */
};

struct _glink {
	Group		*gr;			/* the group link */
	int		flags;			/* flags of this group link */
	gLink		*next, *prev, *head;	/* link list stuff */
};


#define MFD_NONE 0x00
#define MFD_READ 0x01
#define MFD_WRITE 0x02

/* file descriptor abstraction for socket engine use */
struct _mfd {
	int		fd;
	int		state;
	void		*owner;
	void		(*cb)(SockEng *, void *, int, int);
	void		*internal;
};

/*
 * The client structure provides all necessary details in order to communicate with a client
 */
struct _client {
	myfd		fdp;			/* file descriptor abstraction */
	ipvx		addr;			/* address of the client */
	unsigned int	bufsize;		/* current size of the buffer */
	char		buffer[BUFSIZE];	/* the buffer! */
	eBuf		recvQ;			/* inbound queue */
	eBuf		sendQ;			/* outbound queue */
	int		sockerr;		/* any socket error is cached here */
	unsigned short	port;			/* the remote port */
	int		type;			/* the type of this connection (tcp/udp/raw) */

	Listener	*listener;		/* listener this client came in on (optional) */
	SockEng		*sockeng;		/* socket engine for this client */

	/* functions */
	int		(*send)(Client *, char *, int);
	int		(*close)(Client *);
	int		(*qopts)(Client *, int);

	int		(*set_packeter)(Client *, int (*)(Client *, char *, int));
	int		(*set_parser)(Client *, int (*)(Client *, char *, int));
	int		(*set_onclose)(Client *, void (*)(Client *, int));

	int		(*packeter)(Client *, char *, int);
	int		(*parser)(Client *, char *, int);
	void		(*onclose)(Client *, int);
};

/*
 * The listener structure provides all necessary details in order to build and maintain a listener
 */
struct _listener {
	myfd		fdp;		/* file descriptor abstraction */
	unsigned short 	port;		/* port of the descriptor */
	unsigned int 	count;		/* count of the clients connected */
	ipvx		addr;		/* address of the listener to bind to */
	time_t		last;		/* TS of last connect */

	SockEng		*sockeng;	/* socket engine this is running on */

	int		flags;		/* flags? */

	/* functions */
	int		(*qopts)(Listener *, int);
	int		(*set_packeter)(Listener *, int (*)(Client *, char *, int));
	int		(*set_parser)(Listener *, int (*)(Client *, char *, int));
	int		(*set_onconnect)(Listener *, int (*)(Client *));
	int		(*set_onclose)(Listener *, void (*)(Client *, int));

	int		(*packeter)(Client *, char *, int);
	int		(*parser)(Client *, char *, int);
	int		(*onconnect)(Client *c);
	void		(*onclose)(Client *c, int err);
};


/*
 * The Group structure provides the ability to group together clients into distinct sets to allow
 * for easy communication in a `multicast' way.
 */
struct _group {
	cLink	 	*clients;		/* clients in this group */
	gLink		*groups;		/* subgroups to this group */

	Group		*parent;		/* parent group (if applicable) */

	/* functions */
	int		(*add)(Group *gr, Client *cl);		/* function for adding clients to this group */
	int		(*remove)(Group *gr, Client *cl);	/* function for removing clients from this group */
	Group		*(*create_subgroup)(Group *gr);		/* function to create a subgroup */
	int		(*destroy)(Group *gr);			/* destroy this group and its subgroups */

	int		(*send)(Group *, char *, int);			/* send a message to this group */
	int		(*send_butone)(Group *, Client *, char *, int); /* send a message to all but one */
};

/*
 * The SockEng structure provides top level interfaces and tracking for the socket engine as a whole
 */
struct _sockeng {

	gLink		*groups;
	cLink		*clients;
	lLink		*listeners;
	int		loglev;

	myfd		*local[MAX_FDS];

	/* functions */
	int		(*create_listener)(SockEng *, unsigned short, ipvx *, Listener **);
	int		(*create_group)(SockEng *, Group **);
	int		(*poll)(SockEng *, time_t);
	int		(*set_errorhandler)(SockEng *, void (*)(int, char *));
	int		(*set_loglevel)(SockEng *, int);

	void		(*error)(int, char *);
};

/* functions */
extern int init_sockeng(SockEng **);

#endif
