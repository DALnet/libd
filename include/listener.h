/* listener.h
 * awiebe, 2008
 */

#ifndef LISTENER_H
#define LISTENER_H


typedef struct _listener aListener;

struct _llink {
	Listener 	*c;
	struct _llink 	*next, *prev, *head;
}

struct _listener {
	int 		fd;	/* file descriptor of the listener */
	u_short 	port;	/* port of the descriptor */
	unsigned int 	count;	/* count of the clients connected */
	union {
		struct in_addr v4;
		struct in6_addr v6;
	} ip;			/* address to bind to */
	time_t		last;	/* TS of last connect */

	int		flags;	/* flags? */

	int		(*qopts)();		/* function to set options */
	int		(*set_packeter)();	/* function to set packeter */
	int		(*set_parser)();	/* function to set parser */


	char		*(*packeter)();		/* the packeter */
	int		(*parser)();		/* the parser */

};

#endif
