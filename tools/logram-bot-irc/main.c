#include "local.h"

struct global_t *global;

void init (void)
{
	global = malloc (sizeof (struct global_t));

	if (global == NULL)
		exit (EXIT_FAILURE);

	/* Server & Channel */
	global->config.server  = "irc.freenode.net";
	global->config.port    = 6667;
	global->config.channel = "#logram-project";
	/* Bot */
	global->config.admin = "n=linkdd@ydb.me";
	global->config.nick  = "Logram-Bot";
	global->config.host  = "Logram-Bot_hostname";
	global->config.ident = "Logram-Bot";
	global->config.name  = "Logram Bot, RSS Reader";

	/* RSS */
	global->title.news    = NULL;
	global->title.journal = NULL;
	global->title.ask     = NULL;
	global->title.msg     = NULL;
	global->title.wiki    = NULL;

	global->link.news    = NULL;
	global->link.journal = NULL;
	global->link.ask     = NULL;
	global->link.msg     = NULL;
	global->link.wiki    = NULL;
}

void release (void)
{
	message (global->sock, "QUIT :Shutdown\r\n");

	free (global);
	stack_delete ();

	exit (EXIT_SUCCESS);
}

int main (int argc, char **argv)
{
	char buf[BUF_LENGTH];
	int s, size;
	pthread_t thread;

	init ();

	if (argc >= 2)
		config_load (argv[1]);

	for (;;)
	{
		if ((s = connection (global->config.server, global->config.port)) == -1)
		{
			fprintf (stderr, "Can't connect to the server : %s\r\n", global->config.server);
			fprintf (stderr, "Retry in 5 seconds.\r\n");
			sleep (5);
		}
		else
		{
			message (s, "NICK %s\r\n", global->config.nick);
			message (s, "USER %s %s %s :%s\r\n", global->config.ident, global->config.host, global->config.nick, global->config.name);
			message (s, "JOIN %s\r\n", global->config.channel);

			global->sock = s;
			pthread_create (&thread, NULL, thread_rss, NULL);

			for (;;)
			{
				struct msg_t *msg = NULL;

				size = recv (s, buf, BUF_LENGTH - 1, 0);
				if (!size)
					exit (EXIT_FAILURE);
				buf[size] = 0;

				printf (buf);

				if (!strncmp (buf, "PING ", 5))
				{
					buf[1] = 'O';
					message (s, buf);
				}

				if ((msg = privmsg_parse (buf)) != NULL)
				{
					char **argv = parse_cmd (msg);
					int i, argc;

					for (argc = 0; argv[argc] != NULL; ++argc);
					for (i = 0; commands[i].name != NULL; ++i)
						if (!strcmp (argv[0], commands[i].name))
							commands[i].handler (s, msg, argc, argv);
				}

				stack_delete ();
			}
		}
	}

	return EXIT_SUCCESS;
}
