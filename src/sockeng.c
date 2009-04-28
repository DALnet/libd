/* sockeng.c
 * awiebe, dec 2008
 */

#include <stdarg.h>

#include "sockeng.h"
#include "engine.h"

extern Group *create_supergroup(SockEng *s);
extern Listener *create_listener(SockEng *s);

static int set_errorhandler(SockEng *s, int level, void (*func)())
{
	if(!s)
		return -1;
	if(func)
		s->error = func;
	if(level)
		s->loglev = level;
	return 0;
}

void s_err(SockEng *s, int level, int err, char *pattern, ...)
{
	va_list vl;
	char buffer[BUFSIZE];

	va_start(vl, pattern);
	vsnprintf(buffer, BUFSIZE-1, pattern, vl);

	if(s->error && (level >= s->loglev))
		s->error(err, buffer);
	
	va_end(vl);
}

SockEng *init_sockeng()
{
	SockEng *new;

	new = malloc(sizeof(SockEng));

	/* data */
	new->groups = NULL;
	new->clients = NULL;
	new->listeners = NULL;
	new->error = NULL;
	new->loglev = DL_CRIT;

	/* functions */
	new->create_listener = create_listener;
	new->create_group = create_supergroup;
	new->poll = engine_read_message;
	new->set_errorhandler = set_errorhandler;

	engine_init(new);
	ebuf_init();

	return new;
}
