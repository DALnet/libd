/* group.h
 * awiebe, 2008
 */

#ifndef GROUP_H
#define GROUP_H

typedef struct _group Group;

struct _clink {
	Client 		*c;
	struct _clink	*next, *prev, *head;
};

struct _glink {
	Group		*c;
	struct _glink	*next, *prev, *head;
};

struct _group {
	struct _clink 	clients;	/* clients in this group */
	struct _glink	groups;		/* subgroups to this group */

	int		(*add)();
	int		(*remove)();
	Group		*(*create_subgroup)();
	int		(*destroy)();
};
#endif
