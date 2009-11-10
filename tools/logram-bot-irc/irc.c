#include "local.h"

int connection (char *server, int port)
{
	struct hostent *h;
	struct sockaddr_in sockname;
	int s;

	if ((s = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("socket()");
		return -1;
	}

	if ((h = gethostbyname (server)) == NULL)
	{
		if ((h = gethostbyaddr (server, sizeof server, AF_INET)) == NULL)
		{
			perror ("gethost()");
			return -1;
		}
	}

	memcpy (&sockname.sin_addr.s_addr, h->h_addr, h->h_length);
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons (port);

	if (connect (s, (struct sockaddr *) &sockname, sizeof sockname) < 0)
	{
		perror ("connect()");
		return -1;
	}

	return s;
}

int message (int sock, char *fmt, ...)
{
	va_list arg;
	char *buffer;
	int ret = 0;

	va_start (arg, fmt);
	vasprintf (&buffer, fmt, arg);
	va_end (arg);

	ret = send (sock, buffer, strlen (buffer), 0);
	free (buffer);
	return ret;
}

struct msg_t *privmsg_parse (char *buffer)
{
	struct msg_t *msg = xmalloc (sizeof (struct msg_t));
	char *str = xmalloc (strlen (buffer) + 1);
	
	memset (str, 0, strlen (buffer) + 1);
	strncpy (str, buffer, strlen (buffer));

	if (msg != NULL)
	{
		if (str[0] != ':' || strstr (str, "PRIVMSG") == NULL)
			return NULL;
	
		str++;
		asprintf (&msg->nick, "%s", strtok (str, "!"));
		asprintf (&msg->user, "%s", strtok (NULL, "@"));
		asprintf (&msg->host, "%s", strtok (NULL, " "));
		asprintf (&msg->userhost, "%s@%s", msg->user, msg->host);
		strtok (NULL, " "); /* delete PRIVMSG */
		asprintf (&msg->channel, "%s", strtok (NULL, " "));

		if ((str = strchr (buffer, '\r')) != NULL) *str = 0;
		asprintf (&msg->message, "%s", strchr (buffer + 1, ':') + 1);

		if (msg->channel[0] != '#')
		{
			free (msg->channel);
			asprintf (&msg->channel, msg->nick);
		}
	}

	return msg;
}
