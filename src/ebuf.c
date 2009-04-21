/* ebuf.c
 * awiebe - April, 2009
 */

#include "sockeng.h"
#include "ebuf.h"

/* Maximum message length */
#define MAX_MSGLEN 512

/* INITIAL_EBUFS_X - how many bytes of ebufs to preallocate */
#define INITIAL_EBUFS_SMALL 2 * (1 << 20) /* 2 meg */
#define INITIAL_EBUFS_LARGE 2 * (1 << 20) /* 2 meg */
#define INITIAL_EBUFS_USERS 256		   /* number of ebuf user structs to pool */

/* Definitions */
#define EBUF_LARGE_BUFFER	MAX_MSGLEN
#define EBUF_SMALL_BUFFER	(MAX_MSGLEN / 2)

typedef struct _eBufConfig {
	eBuffer		*largeebuf_pool, *smallebuf_pool;
	eBufUser	*user_pool;
	eBufBlock	*ebuf_blocks;
	eBufUserBlock	*ebufuser_blocks;
	int		ebufuser_total, ebufuser_used;
	int		ebufsmall_total, ebufsmall_used;
	int		ebuflarge_total, ebuflarge_used;
	int		ebufblock_used, ebufuserblock_used;
} eBufConfig;

static eBufConfig sbc;

#define EBUF_BASE			sizeof(eBuffer)
#define EBUF_LARGE_TOTAL		(EBUF_BASE + EBUF_LARGE_BUFFER)
#define EBUF_SMALL_TOTAL		(EBUF_BASE + EBUF_SMALL_BUFFER)


/* block allocation routines */
static int ebuf_allocblock_general(int size, int num, eBuffer **pool)
{
	eBufBlock	*block;
	eBuffer		*bufs;
	int		i;
	
	block = malloc(sizeof(eBufBlock));
		
	block->bufs = malloc(size * num);
		
	block->num = num;
	block->next = sbc.ebuf_blocks;
	sbc.ebuf_blocks = block;
	sbc.ebufblock_used++;
	
	bufs = block->bufs;
	for(i = 0; i < block->num - 1; ++i) {
		bufs->bufsize = size - EBUF_BASE;
		bufs->next = (eBuffer*)(((char*)bufs) + size);
		bufs = bufs->next;
	}
	bufs->bufsize = size - EBUF_BASE;
	bufs->next = *pool;
	*pool = block->bufs;
	
	return 0;
}
	
static int ebuf_allocblock_small(int size)
{
	if(size % EBUF_SMALL_TOTAL != 0)
		size = (size + EBUF_SMALL_TOTAL);
		
	sbc.ebufsmall_total += size / EBUF_SMALL_TOTAL;
		
	return ebuf_allocblock_general(EBUF_SMALL_TOTAL, size / EBUF_SMALL_TOTAL, &sbc.smallebuf_pool);
}

static int ebuf_allocblock_large(int size)
{
	if(size % EBUF_LARGE_TOTAL != 0)
		size = (size + EBUF_LARGE_TOTAL);
		
	sbc.ebuflarge_total += size / EBUF_LARGE_TOTAL;
	
	return ebuf_allocblock_general(EBUF_LARGE_TOTAL, size / EBUF_LARGE_TOTAL, &sbc.largeebuf_pool);
}

static int ebuf_allocblock_users(int count)
{
	eBufUserBlock	*block;
	eBufUser	*users;
	int		i;
	
	block = malloc(sizeof(eBufUserBlock));
		
	block->users = malloc(sizeof(eBufUser) * count);
		
	block->num = count;
	block->next = sbc.ebufuser_blocks;
	sbc.ebufuser_blocks = block;

	sbc.ebufuserblock_used++;
	sbc.ebufuser_total += block->num;
	
	users = block->users;
	for(i = 0; i < block->num - 1; ++i) {
		users->next = users + 1;
		users++;
	}
	users->next = sbc.user_pool;
	sbc.user_pool = block->users;

	return 0;
}

static int ebuf_free(eBuffer *buf)
{
	switch (buf->bufsize) {
		case EBUF_LARGE_BUFFER:
			buf->next = sbc.largeebuf_pool;
			sbc.largeebuf_pool = buf;
			sbc.ebuflarge_used--;
			break;
		
		case EBUF_SMALL_BUFFER:
			buf->next = sbc.smallebuf_pool;
			sbc.smallebuf_pool = buf;
			sbc.ebufsmall_used--;
			break;
		
		default:
			return -1;
	}
		
	return 0;
}

static int ebuf_user_free(eBufUser *user)
{
	user->next = sbc.user_pool;
	sbc.user_pool = user;
	sbc.ebufuser_used--;
	return 0;
}
	   
static eBuffer *ebuf_alloc(int size)
{
	eBuffer *buf;
	
	if(size >= EBUF_SMALL_BUFFER) {
		buf = sbc.largeebuf_pool;
		if(!buf) {
			ebuf_allocblock_large(INITIAL_EBUFS_LARGE);
			buf = sbc.largeebuf_pool;
			if(!buf)
				return NULL;
		}
		sbc.largeebuf_pool = sbc.largeebuf_pool->next;
		
		sbc.ebuflarge_used++;
		
		buf->bufsize = EBUF_LARGE_BUFFER;
		buf->refcount = 0;
		buf->end = ((char*)buf) + EBUF_BASE;
		buf->next = NULL;
		buf->shared = 1;
		return buf;
	} else {
		buf = sbc.smallebuf_pool;
		if(!buf) {
			ebuf_allocblock_small(INITIAL_EBUFS_SMALL);
			buf = sbc.smallebuf_pool;
			if(!buf)
				return ebuf_alloc(EBUF_SMALL_BUFFER+1); /* attempt to substitute a large buffer instead */
		}
		sbc.smallebuf_pool = sbc.smallebuf_pool->next;
		
		sbc.ebufsmall_used++;
		
		buf->bufsize = EBUF_SMALL_BUFFER;
		buf->refcount = 0;
		buf->end = ((char*)buf) + EBUF_BASE;
		buf->next = NULL;
		buf->shared = 0;
		return buf;
	}
}

static eBufUser *ebuf_user_alloc()
{
	eBufUser *user;
	
	user = sbc.user_pool;
	if(!user) {
		ebuf_allocblock_users(INITIAL_EBUFS_USERS);
		user = sbc.user_pool;
		if(!user)
			return NULL;
	}
	sbc.user_pool = sbc.user_pool->next;
	
	sbc.ebufuser_used++;
	
	user->next = NULL;
	user->start = NULL;
	user->buf = NULL;
	return user;
}

static int ebuf_alloc_error()
{
	/* FIXME:  error reporting */
	return -1;
}

/* Global functions */

int ebuf_init()
{
	memset(&sbc, 0, sizeof(eBufConfig));
	ebuf_allocblock_small(INITIAL_EBUFS_SMALL);
	ebuf_allocblock_large(INITIAL_EBUFS_LARGE);
	ebuf_allocblock_users(INITIAL_EBUFS_USERS);
	return 0;
}

/* shared buffers:
 * we allocate an ebuffer with the data, and return it to the calling
 * function via ptr
 */
eBuffer *ebuf_begin_share(const char *buffer, int len)
{
	eBuffer *s;

	if(len > MAX_MSGLEN)
		len = MAX_MSGLEN;

	s = ebuf_alloc(len);
	if(!s || len > s->bufsize) {
		ebuf_alloc_error();
		return NULL;
	}

	memcpy(s->end, buffer, len);
	s->end += len;
	s->refcount = 0;
	s->shared = 1;

	return s;
}

/* identify that this buffer has been associated to all references now */
int ebuf_end_share(eBuffer *s)
{
	s->shared = 0;
	if(s->refcount == 0)
		ebuf_free(s);

	return 0;
}

/* associate a given shared buffer with a queue */
int ebuf_put_share(eBuf *sb, eBuffer *s)
{
	eBufUser *user;

	if(!s)
		return -1;

	s->refcount++;
	user = ebuf_user_alloc();
	user->buf = s;
	user->start = (char*)(user->buf) + EBUF_BASE;

	if(sb->length == 0)
		sb->head = sb->tail = user;
	else {
		sb->tail->next = user;
		sb->tail = user;
	}
	sb->length += user->buf->end - user->start;
	return 0;
}

/* create and place a buffer in the correct user location */
int ebuf_put(eBuf *sb, char *buffer, int len)
{
	eBufUser	**user, *u;
	int		chunk;

	if(sb->length == 0)
		user = &sb->head;
	else
		user = &sb->tail;

	if((u = *user) != NULL && u->buf->refcount > 1) {
		u->next = ebuf_user_alloc();
		u = u->next;
		if(!u)
			return ebuf_alloc_error();
		*user = u; /* tail = u */

		u->buf = ebuf_alloc(len);
		u->buf->refcount = 1;
		u->start = u->buf->end;
	}

	sb->length += len;

	for(; len > 0; user = &(u->next)) {
		if((u = *user) == NULL) {
			u = ebuf_user_alloc();
			if(!u)
				return ebuf_alloc_error();
			*user = u;
			sb->tail = u;

			u->buf = ebuf_alloc(len);
			u->buf->refcount = 1;
			u->start = u->buf->end;
		}
		chunk = (((char*)u->buf) + EBUF_BASE + u->buf->bufsize) - u->buf->end;
		if(chunk) {
			if(chunk > len)
				chunk = len;
			memcpy(u->buf->end, buffer, chunk);

			u->buf->end += chunk; 
			buffer	 += chunk;
			len   -= chunk;
		}
	}
	return 0;
}

/*
int ebuf_putiov(eBuf *sb, struct iovec *v, int count)
{
	int i = 0, ret;

	while(i < count) {
		ret = ebuf_put(sb, v[i]->iov_base, v[i]->iov_len);
		if(ret)
			return ret;
		i++;
	}
	return 0;
}
*/

int ebuf_delete(eBuf *sb, int len)
{
	if(len > sb->length)
		len = sb->length;

	sb->length -= len;

	while(len) {
		int chunk = sb->head->buf->end - sb->head->start;
		if(chunk > len)
			chunk = len;

		sb->head->start += chunk;
		len		   -= chunk;

		if(sb->head->start == sb->head->buf->end) {
			eBufUser *tmp = sb->head;
			sb->head = sb->head->next;

			tmp->buf->refcount--;
			if(tmp->buf->refcount == 0 && tmp->buf->shared == 0)
				ebuf_free(tmp->buf);
			ebuf_user_free(tmp);
		}
	}
	if(sb->head == NULL)
		sb->tail = NULL;

	return 0;
}

char *ebuf_map(eBuf *sb, int *len)
{
	if(sb->length != 0) {
		*len = sb->head->buf->end - sb->head->start;
		return sb->head->start;
	}
	*len = 0;
	return NULL;
}

int ebuf_mapiov(eBuf *sb, struct iovec *iov)
{
	int i = 0;
	eBufUser *sbu;

	if(sb->length == 0)
		return 0;

	for(sbu = sb->head; sbu; sbu = sbu->next) {
		iov[i].iov_base = sbu->start;
		iov[i].iov_len = sbu->buf->end - sbu->start;
		if(++i == WRITEV_IOV)
			break;
	}

	return i;
}

/*
int ebuf_flush(eBuf *sb)
{
	eBufUser *tmp;
	char *ptr;
	
	if(sb->length == 0)
		return 0;
	
	while(sb->head) {
		ptr = sb->head->start;
		while(ptr < sb->head->buf->end && IsEol(*ptr))
			ptr++;
		
		sb->length -= ptr - sb->head->start;
		sb->head->start = ptr;
		if(ptr < sb->head->buf->end)
			break;
		
		tmp = sb->head;
		sb->head = tmp->next;
		
		tmp->buf->refcount--;
		if(tmp->buf->refcount == 0 && tmp->buf->shared == 0)
			ebuf_free(tmp->buf);
		ebuf_user_free(tmp);
	}
	if(sb->head == NULL)
		sb->tail = NULL;
	return sb->length;   
}
*/

/* get an arbitrary length from the queue and put it in buffer */
int ebuf_get(eBuf *sb, char *buffer, int len)
{
	eBufUser	*user;
	int		copied = 0;
	char		*ptr, *max;
	
	for(user = sb->head; user && len; user = user->next) {
		max = user->start + len; 
		if(max > user->buf->end)
			max = user->buf->end;
		
		ptr = user->start;
		while(ptr < max)
			*buffer++ = *ptr++;
			
		copied += ptr - user->start;
		len -= ptr - user->start;
		
		if(!len) {
			*buffer = 0;	/* null terminate.. */
			/*
			ebuf_delete(sb, copied);
			ebuf_flush(sb);
			*/
			return copied;
		}
	}
	return 0;
}

/* 
int ebuf_get(eBuf *sb, char *buffer, int len)
{
	char	*buf;
	int	chunk, copied;

	if(sb->length == 0)
		return 0;

	copied = 0;
	while(len && (buf = ebuf_map(sb, &chunk)) != NULL) {
		if(chunk > len)
			chunk = len;

		memcpy(buffer, buf, chunk);
		copied += chunk;
		buffer += chunk;
		len -= chunk;
		ebuf_delete(sb, chunk);
	}
	return copied;
}
*/
