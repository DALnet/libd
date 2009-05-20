#include <stdio.h>
#include "sockeng.h"

#define CERTFILE "./test.crt"
#define KEYFILE "./test.key"

int client_packet_thingy(Client *c, char *buf, int len)
{
	int i = 0;
	/* find a \n */
	while(len > i) {
		if(buf[i] == '\n') 
			return ++i;
		i++;
	}
	return 0;
}

int client_echo_parser(Client *c, char *start, int len)
{
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);
	snprintf(buf, len+1, "%s", start);
	printf("Got buffer length %d:\n%s", len, buf);
	c->send(c, buf, len);
	return 0;
}

void client_disconnecty(Client *s, int err)
{
	printf("Client disconnected:  %s\n", strerror(err));
}

int main(int argc, char *argv[])
{
	SockEng *s;
	Listener *l1, *ssll;

	if(init_sockeng(&s))
		return -1;
	if(s->init_ssl(s, KEYFILE, CERTFILE)) {
		printf("ssl initialization failed\n");
		return -1;
	}
	if(s->create_listener(s, 1111, NULL, &l1)) {
		printf("no listener create\n");
		return -1;
	} else {
		l1->set_packeter(l1, client_packet_thingy);
		l1->set_parser(l1, client_echo_parser);
		l1->set_onclose(l1, client_disconnecty);
		if(l1->up(l1)) {
			printf("no listener create(2)\n");
			return -1;
		}
	}
	if(s->create_listener(s, 1112, NULL, &ssll)) {
		printf("ssl listener create failed\n");
		return -1;
	} else {
		ssll->set_packeter(ssll, client_packet_thingy);
		ssll->set_parser(ssll, client_echo_parser);
		ssll->set_onclose(ssll, client_disconnecty);
		ssll->set_options(ssll, LISTEN_SSL);
		if(ssll->up(ssll)) {
			printf("ssl listener failed to come up\n");
			return -1;
		}
	}

	while(1) {
		if(s->poll(s, 1)) {
			printf("poll error\n");
			return -1;
		}
	}

	return 0;
}
