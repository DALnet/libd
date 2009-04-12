/* sockeng.c
 * awiebe, dec 2008
 */

#include "sockeng.h"
#include "engine.h"

extern Group *create_supergroup(SockEng *s);
extern Listener *create_listener(SockEng *s);

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
	new->poll = engine_read_message;
	new->set_errorhandler = set_errorhandler;

	engine_init(new);

	return new;
}
