/* wrap_ssl.c
 * Aaron Wiebe, May 2009
 */

#include "sockeng.h"

#ifdef USE_SSL

#define CALL_SSLREAD	1
#define CALL_SSLWRITE	2
#define CALL_SSLACCEPT	3
#define CALL_SSLSHUT	4

static int call_ssl(int type, SSL *id, void *buf, int len)
{
	int ret = 0, i = 0;

	switch(type) {
		case CALL_SSLREAD:
			ret = SSL_read(id, buf, len);
			break;
		case CALL_SSLWRITE:
			ret = SSL_write(id, buf, len);
			break;
		case CALL_SSLACCEPT:
			ret = SSL_accept(id);
			break;
		case CALL_SSLSHUT:
			while(ret || i < 4) {
				ret = SSL_shutdown(id);
				i++;
			}
			break;
		default:
			return -1;
	}
	if(ret <= 0) {
		int err;
		err = SSL_get_error(id, ret);
		switch(err) {
			case SSL_ERROR_SYSCALL:
				if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
					errno = EWOULDBLOCK;
					return ret;
				}
			case SSL_ERROR_WANT_WRITE:
			case SSL_ERROR_WANT_READ:
				errno = EWOULDBLOCK;
				return ret;
			default:
				/* FIXME:  error reporting */
				return ret;
		}
	}
	return ret;
}

int sslread(SSL *id, void *buf, int sz)
{
	return call_ssl(CALL_SSLREAD, id, buf, sz);
}

int sslwrite(SSL *id, void *buf, int sz)
{
	return call_ssl(CALL_SSLWRITE, id,  buf, sz);
}

int sslaccept(SSL *id)
{
	return call_ssl(CALL_SSLACCEPT, id, NULL, 0);
}

int sslshut(SSL *id) 
{
	return call_ssl(CALL_SSLSHUT, id, NULL, 0);
}


int ssl_init(SockEng *s, char *certpath, char *keypath)
{
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	s->sslctx = SSL_CTX_new(SSLv23_server_method());

	if(!s->sslctx) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	if(SSL_CTX_use_certificate_file(s->sslctx, certpath, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		goto out_err;
	}

	if(SSL_CTX_use_PrivateKey_file(s->sslctx, keypath, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		goto out_err;
	}

	if(!SSL_CTX_check_private_key(s->sslctx)) {
		fprintf(stderr, "SSL Certificate does not match SSL key\n");
		goto out_err;
	}
	return 0;

out_err:
	SSL_CTX_free(s->sslctx);
	s->sslctx = NULL;
	return -1;
}

#endif
