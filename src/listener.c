/* listener.c
 * awiebe, 2008
 */

#include "sockeng.h"
#include "mfd.h"
#include "wrap_ssl.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

extern void client_do_rw(SockEng *s, void *c, int rr, int rw);
extern Client *create_client_t(Listener *);

static int listener_qopts(Listener *l, int opts)
{
	l->flags = opts;
	return RET_OK;
}

static int listener_setpacketer(Listener *l, int (*func)(Client *, char *, int))
{
	if(l && func) {
		l->packeter = func;
		return RET_OK;
	}
	return RET_INVAL;
}

static int listener_setparser(Listener *l, int (*func)(Client *, char *, int))
{
	if(l && func) {
		l->parser = func;
		return RET_OK;
	}
	return RET_INVAL;
}

static int listener_setonconnect(Listener *l, int (*func)(Client *))
{
	if(l && func) {
		l->onconnect = func;
		return RET_OK;
	}
	return RET_INVAL;
}

static int listener_setonclose(Listener *l, void (*func)(Client *, int))
{
	if(l && func) {
		l->onclose = func;
		return RET_OK;
	}
	return RET_INVAL;
}

static int listener_setsockopts(myfd fdp)
{
	int opt, ret;

#ifdef SO_REUSEADDR
	opt = 1;
	ret = setsockopt(fdp.fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));
	if(ret < 0)
		return ret;
#endif /* SO_REUSEADDR */

#ifdef SO_USELOOPBACK
	opt = 1;
	ret = setsockopt(fdp.fd, SOL_SOCKET, SO_USELOOPBACK, (char *) &opt, sizeof(opt));
	if(ret < 0)
		return ret;
#endif /* SO_USELOOPBACK */

	/* FIXME:  send and recieve buffer sizes here */

	return 0;
}

static int listener_listen(myfd fdp)
{
	int res, nonb = 0;

        /* set to 24, since it seems reasonable for now */
        if(listen(fdp.fd, 24)) {
                /* FIXME:  error reporting */
                return -1;
        }

	nonb |= O_NONBLOCK;
	if((res = fcntl(fdp.fd, F_GETFL, 0)) == -1) {
		/* FIXME:  error reporting */
		;
	}
	else if (fcntl(fdp.fd, F_SETFL, res | nonb) == -1) {
		/* FIXME:  error reporting */
		;
	}
	return 0;
}

static void accept_tcp6_connect(SockEng *s, void *in, int rr, int rw)
{
	Listener *l = in;
	int i, newfd;
	struct sockaddr_in6 addr;
	unsigned int addrlen = sizeof(struct sockaddr_in6);
	Client *new;

	for(i = 0; i < 100; i++) {
		if((newfd = accept(l->fdp.fd, (struct sockaddr *) &addr, &addrlen)) < 0) {
			/* FIXME:  error reporting */
			return;
		}
		new = create_client_t(l);
		new->fdp.fd = newfd;
		new->fdp.owner = new;
		new->addr.type = TYPE_IPV6;
		memcpy(&new->addr.ip, &addr.sin6_addr, sizeof(struct in6_addr));
		new->port = ntohs(addr.sin6_port);
		mfd_add(s, &new->fdp, new, client_do_rw);
#ifdef USE_SSL
		if((l->flags & LISTEN_SSL) && sslaccept(new)) {
			new->close(new);	/* failed SSL negotiation, drop it */
			continue;
		}
#endif
		if(l->onconnect != NULL && (*l->onconnect)(new)) {
			new->close(new);
			continue;
		}
		/* FIXME: set new client socket options */
		mfd_read(s, &new->fdp);
	}
}

static void accept_tcp4_connect(SockEng *s, void *in, int rr, int rw)
{
	Listener *l = in;
	int i, newfd;
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);
	Client *new;

	for(i = 0; i < 100; i++) {
		if((newfd = accept(l->fdp.fd, (struct sockaddr *) &addr, &addrlen)) < 0) {
			/* FIXME:  error reporting */
			return;
		}
		new = create_client_t(l);
		new->fdp.fd = newfd;
		new->fdp.owner = new;
		new->addr.type = TYPE_IPV4;
		memcpy(&new->addr.ip, &addr.sin_addr, sizeof(struct in_addr));
		new->port = ntohs(addr.sin_port);
		mfd_add(s, &new->fdp, new, client_do_rw);
#ifdef USE_SSL
		if((l->flags & LISTEN_SSL) && sslaccept(new)) {
			new->close(new);	/* failed SSL negotation, drop it */
			continue;
		}
#endif
		if(l->onconnect != NULL && (*l->onconnect)(new)) {
			new->close(new);
			continue;
		}
		/* FIXME:  set new client socket options */
		mfd_read(s, &new->fdp);
	}
}

static int create_tcp6_listener(Listener *l)
{
	struct sockaddr_in6 s;

	l->fdp.fd = socket(AF_INET6, SOCK_STREAM, 0);
	if(l->fdp.fd < 0)
		return -1;
	if(listener_setsockopts(l->fdp))
		goto out_err;

	memset(&s, '\0', sizeof(s));
	s.sin6_family = AF_INET6;
	/* 0 == unset */
	if(l->addr.type == 0)
		s.sin6_addr = in6addr_any;
	else
		memcpy(&s.sin6_addr, l->addr.ip.v6, sizeof(struct in6_addr));

	s.sin6_port = htons(l->port);

	/* FIXME:  error reporting */
	if(bind(l->fdp.fd, (struct sockaddr *) &s, sizeof(s)))
		goto out_err;

	if(listener_listen(l->fdp))
		goto out_err;

	mfd_add(l->sockeng, &l->fdp, l, accept_tcp6_connect);
	mfd_read(l->sockeng, &l->fdp);
	return 0;

out_err:
	close(l->fdp.fd);
	l->fdp.fd = -1;
	return -1;
}

static int create_tcp4_listener(Listener *l)
{
	struct sockaddr_in s;

	l->fdp.fd = socket(AF_INET, SOCK_STREAM, 0);
	if(l->fdp.fd < 0)
		return -1;

	if(listener_setsockopts(l->fdp))
		goto out_err;

	memset(&s, '\0', sizeof(s));
	s.sin_family = AF_INET;
	/* 0 == unset */
	if(l->addr.type == 0)
		s.sin_addr.s_addr = INADDR_ANY;
	else
		memcpy(&s.sin_addr.s_addr, l->addr.ip.v4, sizeof(struct in_addr));

	s.sin_port = htons(l->port);

	/* FIXME:  error reporting */
	if(bind(l->fdp.fd, (struct sockaddr *) &s, sizeof(s)))
		goto out_err;

	if(listener_listen(l->fdp))
		goto out_err;

	mfd_add(l->sockeng, &l->fdp, l, accept_tcp4_connect);
	mfd_read(l->sockeng, &l->fdp);
	return 0;

out_err:
	close(l->fdp.fd);
	l->fdp.fd = -1;
	return -1;
}

static int listener_up(Listener *l)
{
	if(!l->packeter || !l->parser)
		return RET_INVAL;

	if(l->addr.type == TYPE_IPV6) {
		if(create_tcp6_listener(l))
			return RET_INVAL;
		return RET_OK;
	} else if(create_tcp4_listener(l))
		return RET_INVAL;
	return RET_OK;
}

static int listener_down(Listener *l)
{
	mfd_del(l->sockeng, &l->fdp);
	close(l->fdp.fd);
	l->fdp.fd = -1;
	return RET_OK;
}

int create_listener(SockEng *s, unsigned short port, ipvx *address, Listener **l)
{
	Listener *new;

	if(!s)
		return RET_INVAL;

	new = malloc(sizeof(Listener));
	if(!new)
		return RET_NOMEM;

	/* data */

	new->sockeng = s;
	new->fdp.fd = -1;
	new->fdp.owner = new;
	new->port = port;
	new->count = 0;
	if(address)
		memcpy(&new->addr, address, sizeof(ipvx));
	else
		memset(&new->addr, '\0', sizeof(ipvx));
	new->last = 0;
	new->flags = 0;

	/* functions */

	new->set_options = listener_qopts;
	new->set_packeter = listener_setpacketer;
	new->set_parser = listener_setparser;
	new->set_onconnect = listener_setonconnect;
	new->set_onclose = listener_setonclose;
	new->up = listener_up;
	new->down = listener_down;

	new->packeter = NULL;
	new->parser = NULL;

	*l = new;

	return RET_OK;
}
