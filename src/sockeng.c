/* sockeng.c
 * awiebe, dec 2008
 */

#include "sockeng.h"

extern Group *create_supergroup(SockEng *s);
extern Listener *create_listener(SockEng *s);

static int fake_poll(SockEng *s)
{
	return 0;
}

static int set_errorhandler(SockEng *s, void (*func)())
{
	if(s) {
		s->error = func;
		return 0;
	}
	return -1;
}

SockEng *init_sockeng()
{
	SockEng *new;

	new = malloc(sizeof(SockEng));

	/* data */
	new->groups = NULL;
	new->clients = NULL;
	new->listeners = NULL;

	/* functions */
	new->create_listener = create_listener;
	new->create_group = create_supergroup;
	new->poll = fake_poll;
	new->set_errorhandler = set_errorhandler;

	return new;
}
