/* ebuf.h
 * awiebe, April 2009
 */

#ifndef EBUF_H
#define EBUF_H

#define eBufLength(s)	   ((s)->length)
#define eBufClear(s)	    ebuf_delete((s), (s)->length)

typedef struct _eBuffer eBuffer;
typedef struct _eBufUser eBufUser;
typedef struct _eBuf eBuf;
typedef struct _eBufBlock eBufBlock;
typedef struct _eBufUserBlock eBufUserBlock;

struct _eBuf
{
	int		length;
	eBufUser	*head, *tail;
};

struct _eBuffer
{
	eBuffer		*next;
	int		shared;
	int		bufsize;
	int		refcount;
	char		*end;
};

struct _eBufBlock
{
	int		num;
	eBuffer		*bufs;
	eBufBlock	*next;
};

struct _eBufUser
{
	char		*start;
	eBuffer		*buf;
	eBufUser	*next;
};

struct _eBufUserBlock
{
	int		num;
	eBufUser	*users;
	eBufUserBlock	*next;
};

/* functions */
extern int ebuf_init();
extern eBuffer *ebuf_begin_share(const char *buffer, int len);
extern int ebuf_end_share(eBuffer *s);
extern int ebuf_put_share(eBuf *sb, eBuffer *s);
extern int ebuf_put(eBuf *sb, const char *buffer, int len);
extern int ebuf_putiov(eBuf *sb, struct iovec *v, int count);
extern int ebuf_delete(eBuf *sb, int len);
extern char *ebuf_map(eBuf *sb, int *len);
extern int ebuf_mapiov(eBuf *sb, struct iovec *iov);
extern int ebuf_get(eBuf *sb, char *buffer, int len);

#endif
