/* wrap_ssl.h
 * external definitions of SSL wrapping functions
 */

#ifdef USE_SSL
#ifndef WRAP_SSL_H
#define WRAP_SSL_H

#include "sockeng.h"

extern int ssl_init(SockEng *s, char *certpath, char *keypath);
extern int sslread(SSL *id, void *buf, int sz);
extern int sslwrite(SSL *id, void *buf, int sz);
extern int sslaccept(Client *c);
extern void sslshut(SSL *id);

#endif
#endif
