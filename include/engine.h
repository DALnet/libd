/* engine.h
 * abstract the socket driver from the rest of things
 */

#ifndef ENGINE_H
#define ENGINE_H

#include "sockeng.h"

extern void init_engine(SockEng *s);
extern void engine_add_fd(SockEng *s, int fd);
extern void engine_del_fd(SockEng *s, int fd);
extern void engine_change_fd_state(SockEng *s, int fd, unsigned int stateplus);
extern int engine_read_message(SockEng *s, time_t delay);

#endif /* ENGINE_H */
