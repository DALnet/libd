/************************************************************************
 *   IRC - Internet Relay Chat, src/socketengine_epoll.c
 *   Copyright (C) 2004 David Parton
 *
 * engine functions for the /dev/epoll socket engine
 *
 */
 
#include "sockeng.h"

#include <sys/epoll.h>

#ifdef NEED_EPOLL_DEFS

_syscall1(int, epoll_create, int, size)
_syscall4(int, epoll_ctl, int, epfd, int, op, int, fd, struct epoll_event*, event)
_syscall4(int, epoll_wait, int, epfd, struct epoll_event*, pevents, int, maxevents, int, timeout)

#endif

extern void mfd_set_internal(SockEng *s, int fd, void *ptr);
extern void *mfd_get_internal(SockEng *s, int fd);

static int epoll_id = -1, numfds = 0;
static struct epoll_fd
{
	int	   fd;
	unsigned int events;
} epoll_fds[MAX_FDS]; 


void engine_init(SockEng *s)
{
	epoll_id = epoll_create(MAX_FDS);
	memset(epoll_fds, 0, sizeof(epoll_fds));
}

void engine_add_fd(SockEng *s, int fd)
{
	struct epoll_event ev;
	
	if (numfds >= MAX_FDS)
		abort();
	
	ev.events = 0;
	ev.data.ptr = &epoll_fds[numfds];
	if (epoll_ctl(epoll_id, EPOLL_CTL_ADD, fd, &ev) < 0)
		abort();
	
	epoll_fds[numfds].fd = fd;
	epoll_fds[numfds].events = 0;
	mfd_set_internal(s, fd, (void*)&epoll_fds[numfds]);
	++numfds;
}

void engine_del_fd(SockEng *s, int fd)
{
	struct epoll_event ev;
	struct epoll_fd	*epfd = (struct epoll_fd*)mfd_get_internal(s, fd);
	
	if (epoll_ctl(epoll_id, EPOLL_CTL_DEL, fd, &ev) < 0)
		abort();
		
	if (epfd - epoll_fds != numfds - 1)
	{
		*epfd = epoll_fds[numfds-1];
		mfd_set_internal(s, epfd->fd, (void*)epfd);
		
		/* update the epoll internal pointer as well */
		ev.events = epfd->events;
		ev.data.ptr = epfd;
		if (epoll_ctl(epoll_id, EPOLL_CTL_MOD, epfd->fd, &ev) < 0)
			abort();
	}
	
	--numfds;
}

void engine_change_fd_state(SockEng *s, int fd, unsigned int stateplus)
{
	struct epoll_event ev;
	struct epoll_fd *epfd = (struct epoll_fd*)mfd_get_internal(s, fd);
	
	ev.events = 0;
	ev.data.ptr = epfd;
	if (stateplus & MFD_WRITE)
		ev.events |= EPOLLOUT;
	if (stateplus & MFD_READ)
		ev.events |= EPOLLIN|EPOLLHUP|EPOLLERR;
	
	if (ev.events != epfd->events)
	{
		epfd->events = ev.events;
		if (epoll_ctl(epoll_id, EPOLL_CTL_MOD, fd, &ev) < 0)
			abort();
	}
}

#define ENGINE_MAX_EVENTS 512
#define ENGINE_MAX_LOOPS (2 * (MAX_FDS / 512))

int engine_read_message(SockEng *s, time_t delay)
{
	struct epoll_event events[ENGINE_MAX_EVENTS], *pevent;
	struct epoll_fd* epfd;
	int nfds, i, numloops = 0, eventsfull;
	
	do
	{
		nfds = epoll_wait(epoll_id, events, ENGINE_MAX_EVENTS, delay * 1000);
		
		if (nfds == -1)
		{
			if (errno == EINTR || errno == EAGAIN)
				return -1;
			/* FIXME: error reporting 
			report_error("epoll_wait: %s:%s", &me);
			*/
			sleep(5);
			return -1;
		}
		eventsfull = nfds == ENGINE_MAX_EVENTS;
		/* FIXME: time sync 
		if (delay || numloops)
			NOW = timeofday = time(NULL);
		*/
		numloops++;
		
		for (i = 0, pevent = events; i < nfds; i++, pevent++)
		{
			epfd = pevent->data.ptr;
			if (epfd->fd != -1)
			{
				int rr = pevent->events & (EPOLLIN|EPOLLHUP|EPOLLERR);
				int rw = pevent->events & EPOLLOUT;
				myfd *mfd = s->local[epfd->fd];

				if(!mfd || !mfd->cb)
					abort();

				(*mfd->cb)(s, mfd->owner, rr, rw);
			}
		}
	} while (eventsfull && numloops < ENGINE_MAX_LOOPS);
	
	return 0;
}

