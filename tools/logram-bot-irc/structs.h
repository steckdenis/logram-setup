#ifndef __STRUCTS_H
#define __STRUCTS_H

struct global_t
{
	struct
	{
		char *server;
		int port;
		char *channel;

		char *admin;
		char *nick;
		char *host;
		char *ident;
		char *name;
	} config;

	struct
	{
		char *news;
		char *journal;
		char *ask;
		char *msg;
		char *wiki;
	} title, link;

	int sock;
};

struct msg_t
{
	char *nick;
	char *user;
	char *host;
	char *userhost;

	char *channel;
	char *message;
};

struct stack_t
{
	void *adr;
	SLIST_ENTRY(stack_t) next;
};

struct cmd_t
{
	char *name;
	void (*handler) (int sock, struct msg_t *data, int argc, char **argv);
	char *desc;
};

#endif /* __STRUCTS_H */
