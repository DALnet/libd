/* group.c
 * awiebe, 2008
 */

#include "sockeng.h"

/* forward declarations */
Group *create_supergroup(SockEng *s);
static Group *create_subgroup(Group *g);

/* add a client to a group */
static int addto_group(Group *g, Client *c)
{
	cLink *link;

	link = malloc(sizeof(cLink));
	if(!link)
		return -1;

	link->cl = c;
	link->flags = 0;
	link->next = NULL;

	if(!g->clients) {
		link->head = link;
		link->prev = NULL;
		g->clients = link;
	} else {
		link->head = g->clients->head;
		link->prev = g->clients;
		g->clients->next = link;
		g->clients = link;
	}
	return 0;
}

/* remove a client from a group */
static int removefrom_group(Group *g, Client *c)
{
	cLink *tmp, *tmp2;

	tmp = g->clients;
	while(tmp) {
		if(tmp->cl == c)
			break;
		tmp = tmp->prev;
	}
	if(!tmp)
		return -1;
	if(tmp->next)
		tmp->next->prev = tmp->prev;
	if(tmp->prev)
		tmp->prev->next = tmp->next;
	if(tmp->head == tmp) {
		if(!tmp->next && !tmp->prev)
			g->clients = NULL;
		tmp2 = g->clients;
		while(tmp2) {
			tmp2->head = tmp->next;
			tmp2 = tmp2->prev;
		}
	}
	free(tmp);
	return 0;
}

/* destroy a group - only when empty */
static int destroy_group(Group *g)
{
	gLink *tmp, *tmp2;

	if(g->groups)
		return -1;
	if(g->clients)
		return -1;
	if(g->parent) {
		tmp = g->parent->groups;
		while(tmp) {
			if(tmp->gr == g)
				break;
			tmp = tmp->prev;
		}
		if(!tmp)
			return -1;
		if(tmp->next)
			tmp->next->prev = tmp->prev;
		if(tmp->prev)
			tmp->prev->next = tmp->next;
		if(tmp->head == tmp) {
			if(!tmp->next && !tmp->prev)
				g->parent->groups = NULL;
			tmp2 = g->parent->groups;
			while(tmp2) {
				tmp2->head = tmp->next;
				tmp2 = tmp2->prev;
			}
		}
		free(tmp);
	}
	free(g);
	return 0;
}

/* send a message to all clients in a group */
static int sendto_group(Group *g, char *msg, int len)
{
	cLink *tmp;

	tmp = g->clients;
	while(tmp) {
		tmp->cl->send(tmp->cl, msg, len);
		tmp = tmp->prev;
	}
	return 0;
}

/* iniitialize a group */
static Group *create_group_t()
{
	Group *new;

	new = malloc(sizeof(Group));

	if(!new)
		return NULL;
	
	/* data */
	new->clients = NULL;
	new->groups = NULL;

	/* functions */
	new->add = addto_group;
	new->remove = removefrom_group;
	new->create_subgroup = create_subgroup;
	new->destroy = destroy_group;

	new->send = sendto_group;

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

	new->parent = NULL;
	
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
		s->groups->next = link;
		s->groups = link;
	}
	return new;
}

/* associate a group with another group */
static Group *create_subgroup(Group *g)
{
	Group *new;
	gLink *link;

	if(!g)
		return NULL;

	new = create_group_t();
	if(!new)
		return NULL;
	
	new->parent = g;

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

