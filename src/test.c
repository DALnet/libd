#include <stdio.h>
#include "sockeng.h"

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
	Listener *l1;

	if(init_sockeng(&s))
		return -1;
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
	while(1) {
		if(s->poll(s, 1)) {
			printf("poll error\n");
			return -1;
		}
	}

	return 0;
}
