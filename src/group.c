/* group.c
 * awiebe, 2008
 */

#include "sockeng.h"
#include "mfd.h"

/* forward declarations */
int create_supergroup(SockEng *s, Group **);
static Group *create_subgroup(Group *g);

/* add a client to a group */
static int addto_group(Group *g, Client *c)
{
	cLink *link;

	link = malloc(sizeof(cLink));
	if(!link)
		return RET_NOMEM;

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
	return RET_OK;
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
		return RET_NOSUCH;
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
	return RET_OK;
}

/* destroy a group - only when empty */
static int destroy_group(Group *g)
{
	gLink *tmp, *tmp2;

	if(g->groups)
		return RET_EXISTS;
	if(g->clients)
		return RET_EXISTS;
	if(g->parent) {
		tmp = g->parent->groups;
		while(tmp) {
			if(tmp->gr == g)
				break;
			tmp = tmp->prev;
		}
		if(!tmp)
			return RET_NOSUCH;
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
	return RET_OK;
}

static int sendto_group_butone(Group *g, Client *c, char *msg, int len)
{
	cLink *tmp;
	Client *this;
	eBuffer *buffer;

	tmp = g->clients;
	buffer = ebuf_begin_share(msg, len);
	if(!buffer)
		return RET_NOMEM;
	while(tmp) {
		this = tmp->cl;
		if(this == c)
			continue;
		ebuf_put_share(&this->sendQ, buffer);
		mfd_write(this->sockeng, &this->fdp);
		tmp = tmp->prev;
	}
	ebuf_end_share(buffer);
	return RET_OK;
}

/* send a message to all clients in a group */
static int sendto_group(Group *g, char *msg, int len)
{
	return sendto_group_butone(g, NULL, msg, len);
}

/* iniitialize a group */
static Group *create_group_t(void)
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
	new->send_butone = sendto_group_butone;

	return new;
}

/* associate a group with a socket engine */
int create_supergroup(SockEng *s, Group **g)
{
	Group *new;
	gLink *link;

	if(!s)
		return RET_INVAL;

	new = create_group_t();
	if(!new)
		return RET_NOMEM;

	new->parent = NULL;
	
	/* associate the new group with the socket engine top level */
	link = malloc(sizeof(gLink));
	if(!link)
		return RET_NOMEM;
	
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
	*g = new;
	return RET_OK;
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

