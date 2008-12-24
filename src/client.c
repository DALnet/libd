/* client.c
 * awiebe, 2008
 */

#include "sockeng.h"

Client *create_client_t()
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

	new->send = NULL;
	new->close = NULL;
	new->qopts = NULL;

	return new;
}
