/* client.c
 * awiebe, 2008
 */

#include "sockeng.h"

static int client_send(Client *c, char *msg, int len)
{
	char buffer[512];
	snprintf(buffer, len > 512 ? len : 512, "%s", msg);
	printf("Send to client %#x:  %s", c, buffer);
	return 0;
}

static int client_close(Client *c)
{
	printf("Closed client\n");
	return 0;
}

static int client_qopts(Client *c, int qopts)
{
	printf("Client qopts %d\n", qopts);
	return 0;
}

static int client_setparser(Client *c, int (*func)())
{
	if(c) {
		c->parser = func;
		return 0;
	}
	return -1;
}

static int client_setpacketer(Client *c, char *(*func)())
{
	if(c) {
		c->packeter = func;
		return 0;
	}
	return -1;
}

Client *create_client_t(Listener *l)
{
	Client *new;
	
	new = malloc(sizeof(Client));
	if(!new)
		return NULL;
	
	new->fd = -1;
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

	if(l) {
		new->parser = l->parser;
		new->packeter = l->packeter;
	} else {
		new->parser = NULL;
		new->packeter = NULL;
	}

	return new;
}

Client *create_client()
{
	return create_client_t(NULL);
}
