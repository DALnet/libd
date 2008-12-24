/* sockeng.c
 * awiebe, dec 2008
 */

#include "sockeng.h"

/* explanation:
 *   I hate sharing scope when I don't have to.
 *   This will be a library, therefor we're going to
 *   make one nice pretty object at the end of it.
 */

#include "listener.c"
#include "group.c"
#include "client.c"

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
	new->poll = NULL;

	return new;
}
