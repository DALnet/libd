/* listener.c
 * awiebe, 2008
 */

#include "sockeng.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

static int listener_listen(int fd)
{
	int res, nonb = 0;

        /* set to 24, since it seems reasonable for now */
        if(listen(fd, 24)) {
                /* FIXME:  error reporting */
                return -1;
        }

	nonb |= O_NONBLOCK;
	if((res = fcntl(fd, F_GETFL, 0)) == -1) {
		/* FIXME:  error reporting */
		;
	}
	else if (fcntl(fd, F_SETFL, res | nonb) == -1) {
		/* FIXME:  error reporting */
		;
	}
	return 0;
}

static int create_tcp6_listener(Listener *l)
{
	struct sockaddr_in6 s;

	l->fd = socket(AF_INET6, SOCK_STREAM, 0);
	if(l->fd < 0)
		return -1;
	if(listener_setsockopts(l->fd))
		goto out_err;

	memset(&s, '\0', sizeof(s));
	s.sin6_family = AF_INET6;
	/* 0 == unset */
	if(l->addr.type == 0)
		s.sin6_addr = in6addr_any;
	else
		memcpy(&s.sin6_addr, l->addr.ip.v6, sizeof(struct in6_addr));

	s.sin6_port = l->port;

	/* FIXME:  error reporting */
	if(bind(l->fd, (struct sockaddr *) &s, sizeof(s)))
		goto out_err;

	if(listener_listen(l->fd))
		goto out_err;

	/* FIXME:  set want read */
	return 0;

out_err:
	close(l->fd);
	l->fd = -1;
	return -1;
}

static int create_tcp4_listener(Listener *l)
{
	struct sockaddr_in s;

	l->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(l->fd < 0)
		return -1;

	if(listener_setsockopts(l->fd))
		goto out_err;

	memset(&s, '\0', sizeof(s));
	s.sin_family = AF_INET;
	/* 0 == unset */
	if(l->addr.type == 0)
		s.sin_addr.s_addr = INADDR_ANY;
	else
		memcpy(&s.sin_addr.s_addr, l->addr.ip.v4, sizeof(struct in_addr));

	s.sin_port = l->port;

	/* FIXME:  error reporting */
	if(bind(l->fd, (struct sockaddr *) &s, sizeof(s)))
		goto out_err;

	if(listener_listen(l->fd))
		goto out_err;

	/* FIXME:  set want read */
	return 0;

out_err:
	close(l->fd);
	l->fd = -1;
	return -1;
}

Listener *create_listener(SockEng *s, unsigned short port, ipvx *address)
{
	Listener *new;

	if(!s)
		return NULL;

	new = malloc(sizeof(Listener));
	if(!new)
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

	/* FIXME:  egg problem possible here.. must fix */
	new->packeter = NULL;
	new->parser = NULL;

	if(address && address->type == TYPE_IPV6) {
		if(create_tcp6_listener(new)) {
			free(new);
			return NULL;
		}
	} else if(create_tcp4_listener(new)) {
		free(new);
		return NULL;
	}

	return new;
}
