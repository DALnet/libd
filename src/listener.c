/* listener.c
 * awiebe, 2008
 */

#include "sockeng.h"

static int listener_qopts(Listener *l, int opts)
{
	return 0;
}

static int listener_setpacketer(Listener *l, char *(*func)())
{
	if(l) {
		l->packeter = func;
		return 0;
	}
	return -1;
}

static int listener_setparser(Listener *l, int (*func)())
{
	if(l) {
		l->parser = func;
		return 0;
	}
	return -1;
}

static int listener_setsockopts(int fd)
{
	int opt, ret;

#ifdef SO_REUSEADDR
	opt = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));
	if(ret < 0)
		return ret;
#endif /* SO_REUSEADDR */

#ifdef SO_USELOOPBACK
	opt = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_USELOOPBACK, (char *) &opt, sizeof(opt));
	if(ret < 0)
		return ret;
#endif /* SO_USELOOPBACK */

	/* FIXME:  send and recieve buffer sizes here */

	return 0;
}

static int create_tcp4_listener(Listener *l)
{
	l->fd = socket(PF_INET, SOCK_STREAM, 0);
	if(l->fd < 0)
		return -1;
	if(listener_setsockopts(l->fd))
		return -1;
}

Listener *create_listener(SockEng *s, unsigned short port, ipvx *address)
{
	Listener *new;

	new = malloc(sizeof(Listener));

	if(!new)
		return NULL;
	
	if(!s)
		return NULL;

	/* data */

	new->fd = -1;
	new->port = port;
	new->count = 0;
	if(address)
		memcpy(&new->addr, address, sizeof(ipvx));
	else
		memset(&new->addr, '\0', sizeof(ipvx));
	new->last = 0;
	new->flags = 0;

	/* functions */

	new->qopts = listener_qopts;
	new->set_packeter = listener_setpacketer;
	new->set_parser = listener_setparser;

	new->packeter = NULL;
	new->parser = NULL;

	return new;
}
