/************************************************************************
 *	IRC - Internet Relay Chat, src/socketengine_poll.c
 *	Copyright (C) 2003 Lucas Madar
 *
 * engine functions for the poll() socket engine
 *
 */

#include "sockeng.h"

#include <sys/poll.h>

extern void mfd_set_internal(SockEng *s, int fd, void *ptr);
extern void *mfd_get_internal(SockEng *s, int fd);

struct pollfd poll_fds[MAX_FDS];
int last_pfd = -1;

void engine_init(SockEng *s)
{
}

void engine_add_fd(SockEng *s, int fd)
{
	struct pollfd *pfd = &poll_fds[++last_pfd];

	/* sanity check */
	if(last_pfd >= MAX_FDS)
		abort();

	mfd_set_internal(s, fd, (void *) last_pfd);

	pfd->fd = fd;
	pfd->events = 0;
	pfd->revents = 0;
}

void engine_del_fd(SockEng *s, int fd)
{
	int arrayidx = (int) mfd_get_internal(s, fd);

	/* If it's at the end of the array, just chop it off */
	if(arrayidx == last_pfd)
	{
		/* debug
		fdfprintf(stderr, "Removing %d[%d] from end of pollfds\n", last_pfd, fd);
		*/
		last_pfd--;
		return;
	}

	/* Otherwise, move the last array member to where the old one was 
	 * debug
	fdfprintf(stderr, "Moving pfd %d[%d] to vacated spot %d[%d] -- now %d[%d]\n", 
		 last_pfd, poll_fds[last_pfd].fd, arrayidx, fd, last_pfd, fd);
	*/
	memcpy(&poll_fds[arrayidx], &poll_fds[last_pfd], sizeof(struct pollfd));
	last_pfd--;
	mfd_set_internal(s, poll_fds[arrayidx].fd, (void *) arrayidx);
}

void engine_change_fd_state(SockEng *s, int fd, unsigned int stateplus)
{
	int arrayidx = (int) mfd_get_internal(s, fd);
	struct pollfd *pfd = &poll_fds[arrayidx];

	pfd->events = 0;
	if(stateplus & MFD_READ)
		pfd->events |= POLLIN|POLLHUP|POLLERR;
	if(stateplus & MFD_WRITE)
		pfd->events |= POLLOUT;
}

static void engine_get_pollfds(struct pollfd **pfds, int *numpfds)
{
	*pfds = poll_fds;
	*numpfds = (last_pfd + 1);
}

int engine_read_message(SockEng *s, time_t delay)
{
	static struct pollfd poll_fdarray[MAX_FDS];

	struct pollfd *pfd;
	int nfds, nbr_pfds, length, i;

	engine_get_pollfds(&pfd, &nbr_pfds);
	memcpy(poll_fdarray, pfd, sizeof(struct pollfd) * nbr_pfds);

	nfds = poll(poll_fdarray, nbr_pfds, delay * 1000);
	if (nfds == -1)
	{
		if(((errno == EINTR) || (errno == EAGAIN)))
			return -1;
		/* FIXME: error reporting
		report_error("poll %s:%s", &me);
		*/
		sleep(5);
		return -1;
	}

	/* FIXME: time sync
	if(delay)
		NOW = timeofday = time(NULL);
	*/

	for (pfd = poll_fdarray, i = 0; nfds && (i < nbr_pfds); i++, pfd++) 
	{
		myfd *mfd = s->local[pfd->fd];

		length = -1;

		if (nfds && pfd->revents)
		{
			int rr = pfd->revents & (POLLIN|POLLHUP|POLLERR);
			int rw = pfd->revents & (POLLOUT);

			/* debug
			fdfprintf(stderr, "fd %d: %s%s\n", pfd->fd, rr ? "read " : "", rw ? "write" : "");
			*/

			nfds--;

			if(!mfd || !mfd->cb)
				abort();

			(*mfd->cb)(s, mfd->owner, rr, rw);
		}
	} /* end of for() loop for testing polled sockets */

	return 0;
}
