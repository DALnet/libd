/* client.c
 * awiebe, 2008
 */

#include "sockeng.h"
#include "mfd.h"

static int client_send(Client *c, char *msg, int len)
{
	/* buffer it up for sending .. */
	if(ebuf_put(&c->sendQ, msg, len))
		return RET_NOMEM;
	mfd_write(c->sockeng, &c->fdp);
	return RET_OK;
}

static int client_close(Client *c)
{
	/* clean up and close the client out */
	ebuf_delete(&c->recvQ, eBufLength(&c->recvQ));
	ebuf_delete(&c->sendQ, eBufLength(&c->sendQ));
	mfd_del(c->sockeng, &c->fdp);
	close(c->fdp.fd);
	free(c);
	return RET_OK;
}

static int client_qopts(Client *c, int qopts)
{
	printf("Client qopts %d\n", qopts);
	return RET_OK;
}

static int client_setparser(Client *c, int (*func)(Client *, char *, int))
{
	if(c && func) {
		c->parser = func;
		return RET_OK;
	}
	return RET_INVAL;
}

static int client_setpacketer(Client *c, int (*func)(Client *, char *, int))
{
	if(c && func) {
		c->packeter = func;
		return RET_OK;
	}
	return RET_INVAL;
}

static int client_setonclose(Client *c, void (*func)(Client *, int))
{
	if(c && func) {
		c->onclose = func;
		return RET_OK;
	}
	return RET_INVAL;
}

/* unexpected shutdown - read or write error */
static void client_shutdown(Client *c, int err)
{
	if(c->onclose)
		c->onclose(c, err);
	client_close(c);
}

static void client_doread(Client *c)
{
	static char readbuf[BUFSIZE];
	int len, plen;

	len = recv(c->fdp.fd, readbuf, sizeof(readbuf), 0);
	if(len < 0) {
		if(errno == EWOULDBLOCK || errno == EAGAIN)
			return;
		client_shutdown(c, errno);
	}
	if(len == 0)
		return;
	if(eBufLength(&c->recvQ) > 0) {
		if(ebuf_put(&c->recvQ, readbuf, len))
			return;
		len = ebuf_get(&c->recvQ, readbuf, BUFSIZE);
		plen = c->packeter(c, readbuf, len);
		if(plen) {
			c->parser(c, readbuf, plen);
			ebuf_delete(&c->recvQ, plen);
		}
	} else {
		plen = c->packeter(c, readbuf, len);
		if(plen)
			c->parser(c, readbuf, plen);
		else if(ebuf_put(&c->recvQ, readbuf, len))
			return;
	}
	return;
}

static void client_dowrite(Client *c)
{
	static struct iovec v[WRITEV_IOV];
	int num, ret;

	if(!(eBufLength(&c->sendQ) > 0)) {
		mfd_unwrite(c->sockeng, &c->fdp);
		return;
	}
	num = ebuf_mapiov(&c->sendQ, v);
	ret = writev(c->fdp.fd, v, num);
	if(ret > 0)
		ebuf_delete(&c->sendQ, ret);
	if(!(eBufLength(&c->sendQ) > 0))
		mfd_unwrite(c->sockeng, &c->fdp);
	return;
}

void client_do_rw(SockEng *s, void *in, int rr, int rw)
{
	Client *c = in;
	if(rr)
		client_doread(c);
	if(rw)
		client_dowrite(c);
	return;
}

Client *create_client_t(Listener *l)
{
	Client *new;
	
	new = malloc(sizeof(Client));
	if(!new)
		return NULL;
	
	new->fdp.fd = -1;
	new->fdp.owner = new;
	new->addr.type = 0;
	new->bufsize = 0;
	new->sockerr = 0;
	new->port = 0;
	new->type = 0;

	/* functions */

	new->send = client_send;
	new->close = client_close;
	new->qopts = client_qopts;

	new->set_parser = client_setparser;
	new->set_packeter = client_setpacketer;
	new->set_onclose = client_setonclose;

	new->listener = l;
	if(l) {
		new->parser = l->parser;
		new->packeter = l->packeter;
		new->sockeng = l->sockeng;
	} else {
		new->parser = NULL;
		new->packeter = NULL;
	}

	return new;
}

Client *create_client(void)
{
	return create_client_t(NULL);
}
