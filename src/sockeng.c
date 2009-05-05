/* sockeng.c
 * awiebe, dec 2008
 */

#include <stdarg.h>

#include "sockeng.h"
#include "engine.h"

extern int create_supergroup(SockEng *, Group **);
extern int create_listener(SockEng *, unsigned short, ipvx *, Listener **);

static int set_errorhandler(SockEng *s, void (*func)(int, char *))
{
	if(s && func) {
		s->error = func;
		return RET_OK;
	}
	return RET_INVAL;
}

static int set_loglevel(SockEng *s, int i)
{
	switch(i) {
		case DL_DEBUG:
		case DL_INFO:
		case DL_WARN:
		case DL_CRIT:
			s->loglev = i;
			return RET_OK;
		default:
			return RET_INVAL;
	}
	return RET_UNDEF;
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

int init_sockeng(SockEng **s)
{
	SockEng *new;

	new = malloc(sizeof(SockEng));
	if(!new)
		return RET_NOMEM;

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
	new->set_loglevel = set_loglevel;

	engine_init(new);
	if(ebuf_init()) {
		free(new);
		return RET_NOMEM;
	}

	*s = new;

	return RET_OK;
}
