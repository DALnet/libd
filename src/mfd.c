/* mfd.c
 * Manage file descriptor abstraction
 */

#include "sockeng.h"
#include "engine.h"

static void fd_assert(SockEng *s, myfd *fd)
{
#ifdef DEBUG
	if(fd->fd < 0 || fd->fd > MAX_FDS)
		abort();
	if(s->local[fd->fd] != fd)
		abort();
	if(fd->owner == NULL)
		abort();
	if(fd->cb == NULL)
		abort();
#endif
}


int mfd_add(SockEng *s, myfd *fd, void *owner, void (*cb)())
{
	if(s->local[fd->fd] != NULL)
		return -1;
	fd->owner = owner;
	fd->cb = cb;
	fd->state = 0;
	s->local[fd->fd] = fd;

	engine_add_fd(s, fd->fd);

	return 0;
}

void mfd_del(SockEng *s, myfd *fd)
{
	fd_assert(s, fd);
	s->local[fd->fd] = NULL;
	engine_del_fd(s, fd->fd);
}

void mfd_read(SockEng *s, myfd *fd)
{
	fd_assert(s, fd);
	fd->state |= MFD_READ;
	engine_change_fd_state(s, fd->fd, MFD_READ);
}

void mfd_write(SockEng *s, myfd *fd)
{
	fd_assert(s, fd);
	fd->state |= MFD_WRITE;
	engine_change_fd_state(s, fd->fd, MFD_WRITE);
}

void mfd_set_internal(SockEng *s, int fd, void *ptr)
{
	s->local[fd]->internal = ptr;
}

void *mfd_get_internal(SockEng *s, int fd)
{
	return s->local[fd]->internal;
}
