/************************************************************************
 *   IRC - Internet Relay Chat, src/socketengine_kqueue.c
 *   Copyright (C) 2003 Lucas Madar
 *
 * engine functions for the kqueue() socket engine
 *
 */

#include "sockeng.h"

#include <sys/event.h>
#include <sys/time.h>

extern void mfd_set_internal(SockEng *s, int fd, void *ptr);
extern void *mfd_get_internal(SockEng *s, int fd);

#define MAX_EVENT_QUEUE 64

static int kqueue_id = -1;
static struct kevent eventQs[2][MAX_EVENT_QUEUE+1];
static struct kevent *eventQ = eventQs[0];
static int eventQi = 0;
static int numEvents = 0;

static void kevent_add(struct kevent *e)
{
	if(kqueue_id == -1)
		abort();

	if(numEvents >= MAX_EVENT_QUEUE)
	{
		if(kevent(kqueue_id, eventQ, numEvents, NULL, 0, NULL) < 0) {
			/* FIXME: error handling */
			;
		}
		numEvents = 0;
	}
	memcpy(&eventQ[numEvents++], e, sizeof(struct kevent));
}

void engine_init(SockEng *s)
{
	kqueue_id = kqueue();
	numEvents = 0;
}

void engine_add_fd(SockEng *s, int fd)
{
	struct kevent e;

	e.ident = fd;
	e.filter = EVFILT_READ;
	e.flags = EV_ADD|EV_DISABLE;
	e.fflags = 0;
	e.data = 0;
	e.udata = NULL;
	kevent_add(&e);

	e.ident = fd;
	e.filter = EVFILT_WRITE;
	e.flags = EV_ADD|EV_DISABLE;
	e.fflags = 0;
	e.data = 0;
	e.udata = NULL;
	kevent_add(&e);

	mfd_set_internal(s, fd, 0);
}

void engine_del_fd(SockEng *s, int fd)
{
/* we dont accually need to do this, as a close() clears the kevent
 * filters and automagically removes itself from the queue.
 * With the way we handle kevent() calls in kevent_add(), accually
 * running these EV_DELETE routines causes bad file descriptor returns
 * due to the fact that they could be close()'d before the kevent() is
 * run.  --epi
 */

/********

   struct kevent e;

   e.ident = fd;
   e.filter = EVFILT_READ;
   e.flags = EV_DELETE;
   e.fflags = 0;
   e.data = 0;
   e.udata = NULL;
   kevent_add(&e);

   e.ident = fd;
   e.filter = EVFILT_WRITE;
   e.flags = EV_DELETE;
   e.fflags = 0;
   e.data = 0;
   e.udata = NULL;
   kevent_add(&e);

********/

	/* But we should remove this fd from the change queue -- if it was closed
	 * and we have a change pending, kevent() will fail later.  What's worse
	 * is that when the queue is flushed due to being full, a kevent() failure
	 * may leave some changes unprocessed.  Reordering the change queue is not
	 * safe, hence the gymnastics below.	-Quension
	 */
	int i, j;

	if (!numEvents)
		return;

	/* optimal case: fd isn't in the change queue */
	for (i = 0; i < numEvents; i++)
		if (eventQ[i].ident == fd)
			break;

	/* second optimal case: fd is last, truncate the queue */
	if (i == numEvents - 1)
		numEvents--;

	if (i == numEvents)
		return;

	/* swap array index, copy all fds before this one */
	eventQi ^= 1;
	memcpy(eventQs[eventQi], eventQ, sizeof(struct kevent) * i);

	/* selectively copy remaining fds, skip bad one */
	for (j = i++; i < numEvents; i++)
		if (eventQ[i].ident != fd)
			memcpy(&eventQs[eventQi][j++], &eventQ[i], sizeof(struct kevent));

	/* swap active array */
	numEvents = j;
	eventQ = eventQs[eventQi];
}

void engine_change_fd_state(SockEng *s, int fd, unsigned int stateplus)
{
	unsigned int oldflags = (unsigned int) mfd_get_internal(s, fd);
	struct kevent e;

	/* Something changed with our read state? */
	if((oldflags ^ stateplus) & MFD_READ)
	{
		e.ident = fd;
		e.filter = EVFILT_READ;
		e.flags = EV_ADD|((stateplus & FDF_WANTREAD) ? EV_ENABLE : EV_DISABLE);
		e.fflags = 0;
		e.data = 0;
		e.udata = 0;
		kevent_add(&e);
	}

	/* Something changed with our write state? */
	if((oldflags ^ stateplus) & MFD_WRITE)
	{
		e.ident = fd;
		e.filter = EVFILT_WRITE;
		e.flags = EV_ADD|((stateplus & FDF_WANTWRITE) ? EV_ENABLE : EV_DISABLE);
		e.fflags = 0;
		e.data = 0;
		e.udata = 0;
		kevent_add(&e);
	}

	mfd_set_internal(s, fd, (void *) stateplus);
}

#define ENGINE_MAX_EVENTS 512
#define ENGINE_MAX_LOOPS (2 * (MAX_FDS / 512))

int engine_read_message(SockEng *s, time_t delay)
{
	static struct kevent events[ENGINE_MAX_EVENTS];

	int nevs, length, i, numloops, eventsfull;
	unsigned int fdflags;
	int fdtype;
	void *fdvalue;
	struct timespec wait;

	numloops = 0;
	wait.tv_sec = delay;
	wait.tv_nsec = 0;

	do
	{
		nevs = kevent(kqueue_id, eventQ, numEvents, events, ENGINE_MAX_EVENTS, &wait);
		numEvents = 0;

		if(nevs == 0)
			return 0;

		if (nevs < 0)
		{
			if((errno == EINTR) || (errno == EAGAIN))
				return -1;

			report_error("kevent %s:%s", &me);
			sleep(5);
			return -1;
		}

		eventsfull = (nevs == ENGINE_MAX_EVENTS) ? 1 : 0;
		if(delay || numloops)
			NOW = timeofday = time(NULL);
		numloops++;
	  
		for(i = 0; i < nevs; i++)
		{
			int rr = 0, rw = 0;
			myfd *mfd = s->local[events[i].ident];

			if(events[i].flags & EV_ERROR)
			{
				errno = events[i].data;
				/* this should be handled later i suppose */
				continue;
			}

			if(events[i].filter == EVFILT_READ)
				rr = 1;
			else if(events[i].filter == EVFILT_WRITE)
				rw = 1;

			length = -1;

			(*mfd->cb)(s, mfd->owner, rr, rw);
		}
	} while(eventsfull && (numloops < ENGINE_MAX_LOOPS));

	return 0;
}
