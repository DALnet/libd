/* client.h
 * awiebe, 2008
 */

#ifndef CLIENT_H
#define CLIENT_H

#define BUFSIZE 4096

typedef struct _client Client;

struct _client {
	int		fd;
	union {
		struct in_addr *v4;
		struct in6_addr *v6;
	} ip;
	unsigned int	bufsize;
	char		buffer[BUFSIZE];
	int		sockerr;
	unsigned short	port;


	/* functions */
	int		(*send)();
	int		(*close)();
	int		(*qopts)();
};

#endif
