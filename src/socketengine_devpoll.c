/************************************************************************
*   IRC - Internet Relay Chat, src/socketengine_devpoll.c
*   Copyright (C) 2004 David Parton
*
* engine functions for the /dev/poll socket engine
*
*/

#include "sockeng.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/devpoll.h>
#include <sys/poll.h>

static int devpoll_id = -1, numfds = 0;

void engine_init(SockEng *s)
{
	devpoll_id = open("/dev/poll", O_RDWR);
}

void engine_add_fd(SockEng *s, int fd)
{
	struct pollfd dev_fd;

	if(numfds >= MAX_FDS)
		abort();

	dev_fd.events = 0;
	dev_fd.revents = 0;
	dev_fd.fd = fd;
	if(write(devpoll_id, &dev_fd, sizeof(struct pollfd)) != sizeof(struct pollfd))
		abort();

	mfd_set_internal(s, fd, 0);
	++numfds;
}

void engine_del_fd(SockEng *s, int fd)
{
	struct pollfd dev_fd;

	dev_fd.events = POLLREMOVE;
	dev_fd.revents = 0;
	dev_fd.fd = fd;
	if(write(devpoll_id, &dev_fd, sizeof(struct pollfd)) != sizeof(struct pollfd))
		abort();

	--numfds;
}

void engine_change_fd_state(SockEng *s, int fd, unsigned int stateplus)
{
	unsigned int events = 0;
	struct pollfd dev_fd;

	if(stateplus & MFD_WRITE)
		events |= POLLOUT;
	if(stateplus & MFD_READ)
		events |= POLLIN|POLLHUP|POLLERR;

	dev_fd.events = events;
	dev_fd.revents = 0;
	dev_fd.fd = fd;

	if(write(devpoll_id, &dev_fd, sizeof(struct pollfd)) != sizeof(struct pollfd))
		abort();

	mfd_set_internal(s, fd, (void*)events);
}

#define ENGINE_MAX_EVENTS 512
#define ENGINE_MAX_LOOPS (2 * (MAX_FDS / 512))

int engine_read_message(SockEng *s, time_t delay)
{
	struct pollfd events[ENGINE_MAX_EVENTS], *pevent;
	struct dvpoll dopoll;
	int nfds, i, numloops = 0, eventsfull;
	unsigned int fdevents;

	dopoll.dp_fds = events;
	dopoll.dp_nfds = ENGINE_MAX_EVENTS;
	dopoll.dp_timeout = delay;
	do
	{
		nfds = ioctl(devpoll_id, DP_POLL, &dopoll);

		if (nfds < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
				return -1; 
			/* FIXME:  error reporting 
			report_error("ioctl(devpoll): %s:%s", &me);
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
			fdevents = (unsigned int)mfd_get_internal(pevent->fd);
			if (pevent->fd != -1)
			{
				int rr = (pevent->revents & (POLLIN|POLLHUP|POLLERR)) && (fdevents & (POLLIN|POLLHUP|POLLERR));
				int rw = (pevent->revents & POLLOUT) && (fdevents & POLLOUT);
				myfd *mfd = s->local[pevent->fd];

				if(!mfd || !mfd->cb)
					abort();

				(*mfd->cb)(s, mfd->owner, rr, rw);
			}
		}
	} while (eventsfull && numloops < ENGINE_MAX_LOOPS);

	return 0;
}
