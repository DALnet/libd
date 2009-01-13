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

#define BUFSIZE 4096

/* typedefs make life easier */
typedef struct _ipvx		ipvx;		/* ip abstraction */
typedef struct _client 		Client;		/* a Client */
typedef struct _group 		Group;		/* a Group */
typedef struct _sockeng 	SockEng;	/* a Socket Engine */
typedef struct _listener 	Listener;	/* a Listener */
typedef struct _llink 		lLink;		/* link structure for listeners */
typedef struct _glink 		gLink;		/* link structure for groups */
typedef struct _clink		cLink;		/* link structure for clients */

/*
 * This structure allows us to handle IPV6 and IPV4 easily
 */
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

/*
 * The client structure provides all necessary details in order to communicate with a client
 */
struct _client {
	int		fd;			/* file descriptor of this client */
	ipvx		addr;			/* address of the client */
	unsigned int	bufsize;		/* current size of the buffer */
	char		buffer[BUFSIZE];	/* the buffer! */
	int		sockerr;		/* any socket error is cached here */
	unsigned short	port;			/* the remote port */
	int		type;			/* the type of this connection (tcp/udp/raw) */

	Listener	*listener;		/* listener this client came in on (optional) */

	/* functions */
	int		(*send)();
	int		(*close)();
	int		(*qopts)();

	int		(*set_packeter)();
	int		(*set_parser)();

	char		*(*packeter)();
	int		(*parser)();
};

/*
 * The listener structure provides all necessary details in order to build and maintain a listener
 */
struct _listener {
	int 		fd;		/* file descriptor of the listener */
	unsigned short 	port;		/* port of the descriptor */
	unsigned int 	count;		/* count of the clients connected */
	ipvx		addr;		/* address of the listener to bind to */
	time_t		last;		/* TS of last connect */

	SockEng		*sockeng;	/* socket engine this is running on */

	int		flags;		/* flags? */

	int		(*qopts)();		/* function to set options */
	int		(*set_packeter)();	/* function to set packeter */
	int		(*set_parser)();	/* function to set parser */


	char		*(*packeter)();		/* the packeter */
	int		(*parser)();		/* the parser */
};


/*
 * The Group structure provides the ability to group together clients into distinct sets to allow
 * for easy communication in a `multicast' way.
 */
struct _group {
	cLink	 	*clients;		/* clients in this group */
	gLink		*groups;		/* subgroups to this group */

	Group		*parent;		/* parent group (if applicable) */

	int		(*add)(Group *gr, Client *cl);		/* function for adding clients to this group */
	int		(*remove)(Group *gr, Client *cl);	/* function for removing clients from this group */
	Group		*(*create_subgroup)(Group *gr);		/* function to create a subgroup */
	int		(*destroy)(Group *gr);			/* destroy this group and its subgroups */

	int		(*send)();				/* send a message to this group */
};

/*
 * The SockEng structure provides top level interfaces and tracking for the socket engine as a whole
 */
struct _sockeng {

	gLink		*groups;
	cLink		*clients;
	lLink		*listeners;

	/* functions */
	Listener	*(*create_listener)();
	Group		*(*create_group)();
	int		(*poll)();
	int		(*set_errorhandler)();
	void		(*error)();
};

#endif
