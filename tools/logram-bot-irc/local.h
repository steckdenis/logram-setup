#ifndef __LOCAL_H
#define __LOCAL_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "queue.h"
#include "structs.h"

#define BUF_LENGTH 1447

extern struct global_t *global;
extern struct cmd_t commands[];

/* main.c */
void release (void);

/* config.c */
void config_load (const char *filename);

/* garbage.c */
void *xmalloc (size_t size);
void stack_delete (void);

/* rss.c */
void *thread_rss (void *data);

/* irc.c */
int connection (char *server, int port);
int message (int sock, char *fmt, ...);
struct msg_t *privmsg_parse (char *buffer);

/* cmd.c */
char **parse_cmd (struct msg_t *msg);

#endif /* __LOCAL_H */
