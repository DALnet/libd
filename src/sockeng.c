/* sockeng.c
 * awiebe, dec 2008
 */

#include "sockeng.h"

/* sockeng_add_listener */
void sockeng_add_listener(SockEng *s, Listener *l)
{
	struct _llink *link, *tmp;

	link = malloc(sizeof(struct _llink));

	if(s->listeners == NULL) {
		link->head = link;
		link->next = NULL;
		link->prev = NULL;
		link->c = l;
		s->listeners = link;
		return;
	}

	tmp = s->listeners;
	while(tmp->next)
		tmp = tmp->next;

	link->head = tmp->head;
	link->prev = tmp;
	link->next = NULL;
	link->c = l;
	tmp->next = link;
	return;
}

/* _create_listener
 * Create a listener on a given port
 * Bind to the IP address provided
 * type = IPV4 or IPV6
 */
static int _create_listener(SockEng *s, unsigned short port, char *ip, short type)
{
	Listener *new;

	new = malloc(sizeof(Listener));

	if(type == TYPE_IPV4 && (inet_pton(AF_INET, ip, &new->ip.v4) != 1))
		goto out_fail;
	else if(type == TYPE_IPV6 && (inet_pton(AF_INET6, ip, &new->ip.v6) != 1))
		goto out_fail;
	else
		goto out_fail;

	new->port = port;

	return new;

out_fail:
	free(new);
	return NULL;
}

SockEng *init_sockeng()
{
	SockEng *new;

	new = malloc(sizeof(SockEng));

	return new;
}
