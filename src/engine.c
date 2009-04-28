/* engine.c
 * Tie together all of the possible engines
 */

#include "sockeng.h"

#ifdef USE_DEVPOLL
#include "socketengine_devpoll.c"
#endif
#ifdef USE_EPOLL
#include "socketengine_epoll.c"
#endif
#ifdef USE_KQUEUE
#include "socketengine_kqueue.c"
#endif
#ifdef USE_POLL
#include "socketengine_poll.c"
#endif
#ifdef USE_SELECT
#include "sockengine_select.c"
#endif
