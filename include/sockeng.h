/* sockeng.h
 * awiebe, 2008
 */

#ifndef SOCKENG_H
#define SOCKENG_H

#include "client.h"
#include "listener.h"
#include "group.h"

typedef struct _sockeng SockEng;

struct _sockeng {

	struct _glink	*groups;
	struct _clink	*clients;
	struct _llink	*listeners;

	/* functions */
	int		(*create_listener)();
	Group		*(*create_group)();
	int		(*poll)();
};

#endif
