/* group.c
 * awiebe, 2008
 */

#include "sockeng.h"

/* iniitialize a group */
Group *create_group_t()
{
	Group *new;

	new = malloc(sizeof(Group));

	if(!new)
		return NULL;
	
	/* data */
	new->clients = NULL;
	new->groups = NULL;

	/* functions */
	new->add = NULL;
	new->remove = NULL;
	new->create_subgroup = NULL;
	new->destroy = NULL;

	new->send = NULL;

	return new;
}

/* associate a group with a socket engine */
Group *create_supergroup(SockEng *s)
{
	Group *new;
	gLink *link;

	if(!s)
		return NULL;

	new = create_group_t();
	if(!new)
		return NULL;
	
	/* associate the new group with the socket engine top level */
	link = malloc(sizeof(gLink));
	if(!link)
		return NULL;
	
	link->gr = new;
	link->flags = 0;
	link->next = NULL;

	if(!s->groups) {
		link->head = link;
		link->prev = NULL;
		s->groups = link;
	} else {
		/* top list insert */
		link->head = s->groups->head;
		link->prev = s->groups;
		s->groups = link;
	}
	return new;
}

/* associate a group with another group */
Group *create_subgroup(Group *g)
{
	Group *new;
	gLink *link;

	if(!g)
		return NULL;

	new = create_group_t();
	if(!new)
		return NULL;
	
	link = malloc(sizeof(gLink));
	if(!link)
		return NULL;
	
	link->gr = new;
	link->flags = 0;
	link->next = NULL;

	if(!g->groups) {
		link->head = link;
		link->prev = NULL;
		g->groups = link;
	} else {
		link->head = g->groups->head;
		link->prev = g->groups;
		g->groups = link;
	}
	return new;
}

