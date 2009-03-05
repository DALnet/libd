/************************************************************************
 *	IRC - Internet Relay Chat, src/socketengine_select.c
 *	Copyright (C) 2003 Lucas Madar
 *
 * engine functions for the select() socket engine
 *
 */

#include "sockeng.h"

#include <sys/select.h>

extern void mfd_set_internal(SockEng *s, int fd, void *ptr);
extern void *mfd_get_internal(SockEng *s, int fd);

static fd_set g_read_set, g_write_set;

void engine_init(SockEng *s)
{
	FD_ZERO(&g_read_set);
	FD_ZERO(&g_write_set);
}

void engine_add_fd(SockEng *s, int fd)
{
	mfd_set_internal(s, fd, (void *) 0);
}

void engine_del_fd(SockEng *s, int fd)
{
	FD_CLR(fd, &g_read_set);
	FD_CLR(fd, &g_write_set);
}

void engine_change_fd_state(SockEng *s, int fd, unsigned int stateplus)
{
	int prevstate = (int) mfd_get_internal(s, fd);

	if((stateplus & MFD_READ) && !(prevstate & MFD_READ))
	{ 
		FD_SET(fd, &g_read_set);
		prevstate |= MFD_READ;
	}
	else if(!(stateplus & MFD_READ) && (prevstate & MFD_READ))
	{
		FD_CLR(fd, &g_read_set);
		prevstate &= ~(MFD_READ);
	}

	if((stateplus & MFD_WRITE) && !(prevstate & MFD_WRITE))
	{
		FD_SET(fd, &g_write_set);
		prevstate |= MFD_WRITE;
	}
	else if(!(stateplus & MFD_WRITE) && (prevstate & MFD_WRITE))
	{
		FD_CLR(fd, &g_write_set);
		prevstate &= ~(MFD_WRITE);
	}

	mfd_set_internal(s, fd, (void *) prevstate);
}

static void engine_get_fdsets(fd_set *r, fd_set *w)
{
	memcpy(r, &g_read_set, sizeof(fd_set));
	memcpy(w, &g_write_set, sizeof(fd_set));
}

int engine_read_message(SockEng *s, time_t delay)
{
	fd_set read_set, write_set;
	struct timeval wt;	
	int nfds, length, i;

	engine_get_fdsets(&read_set, &write_set);

	wt.tv_sec = delay;
	wt.tv_usec = 0;

	nfds = select(MAX_FDS, &read_set, &write_set, NULL, &wt);
	if (nfds == -1)
	{
		if(((errno == EINTR) || (errno == EAGAIN)))
			return -1;
		/* FIXME:  error reporting */
		sleep(5);
		return -1;
	}
	else if (nfds == 0)
		return 0;

/* FIXME:  time sync
	if(delay)
		NOW = timeofday = time(NULL);
*/
	for (i = 0; i < MAX_FDS; i++) 
	{
		myfd *mfd = s->local[i];

		length = -1;

		if (nfds)
		{
			int rr = FD_ISSET(i, &read_set);
			int rw = FD_ISSET(i, &write_set);

			if(rr || rw)
				nfds--;
			else
				continue;

			/* debug */
			/* fdfprintf(stderr, "fd %d: %s%s\n", i, rr ? "read " : "", rw ? "write" : ""); */

			if(!mfd || !mfd->cb)
				abort();

			(*mfd->cb)(s, mfd->owner, rr, rw);
		}
		else
			break; /* no more fds? break out of the loop */
	} /* end of for() loop for testing selected sockets */

	return 0;
}
