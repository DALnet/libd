/* listener.c
 * awiebe, 2008
 */

#include "sockeng.h"

Listener *create_listener(SockEng *s, u_short port, ipvx *address)
{
	Listener *new;

	new = malloc(sizeof(Listener));

	if(!new)
		return NULL;

	if(!address)
		return NULL;
	
	/* data */

	new->fd = -1;
	new->port = port;
	new->count = 0;
	memcpy(&new->addr, address, sizeof(ipvx));
	new->last = 0;
	new->flags = 0;

	/* functions */

	new->qopts = NULL;
	new->set_packeter = NULL;
	new->set_parser = NULL;

	new->packeter = NULL;
	new->parser = NULL;

	return new;
}
