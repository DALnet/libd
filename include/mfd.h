/* mfd.h
 * shared functions from the mfd handler
 */

#ifndef MDF_H
#define MDF_H

extern int mfd_add(SockEng *s, myfd *fd, void *owner, void (*cb)());
extern void mfd_del(SockEng *s, myfd *fd);
extern void mfd_read(SockEng *s, myfd *fd);
extern void mfd_unread(SockEng *s, myfd *fd);
extern void mfd_write(SockEng *s, myfd *fd);
extern void mfd_unwrite(SockEng *s, myfd *fd);
extern void mfd_set_internal(SockEng *s, int fd, void *ptr);
extern void *mfd_get_internal(SockEng *s, int fd);

#endif
