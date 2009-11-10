#include "local.h"

void handle_help    (int sock, struct msg_t *data, int argc, char **argv);
void handle_rss     (int sock, struct msg_t *data, int argc, char **argv);
void handle_news    (int sock, struct msg_t *data, int argc, char **argv);
void handle_journal (int sock, struct msg_t *data, int argc, char **argv);
void handle_ask     (int sock, struct msg_t *data, int argc, char **argv);
void handle_forum   (int sock, struct msg_t *data, int argc, char **argv);
void handle_wiki    (int sock, struct msg_t *data, int argc, char **argv);
void handle_quit    (int sock, struct msg_t *data, int argc, char **argv);

struct cmd_t commands[] =
{
	{ "!help",    handle_help,    "Affiche l'aide" },
	{ "!rss",     handle_rss,     "Affiche tout les flux RSS" },
	{ "!news",    handle_news,    "Affiche la dernière news" },
	{ "!journal", handle_journal, "Affiche le dernier journal" },
	{ "!ask",     handle_ask,     "Affiche la dernière demande" },
	{ "!forum",   handle_forum,   "Affiche le dernier message du forum" },
	{ "!wiki",    handle_wiki,    "Affiche la dernière page de wiki créée" },
	{ "!quit",    handle_quit,    "Ferme le bot" },
	{ NULL, NULL, NULL }
};

char **parse_cmd (struct msg_t *msg)
{
	char **argv = NULL;
	char *p = NULL;
	int i = 0;

	p = strtok (msg->message, " ");
	while (p)
	{
		argv = realloc (argv, sizeof (char *) * ++i);
		asprintf (&argv[i - 1], p);
		p = strtok (NULL, " ");
	}

	argv = realloc (argv, sizeof (char *) * ++i);
	argv[i - 1] = NULL;

	return argv;
}

void handle_quit (int sock, struct msg_t *data, int argc, char **argv)
{
	regex_t reg;
	int match;

	if (0 == regcomp (&reg, global->config.admin, REG_NOSUB | REG_EXTENDED))
	{
		match = regexec (&reg, data->userhost, 0, NULL, 0);
		regfree (&reg);

		if (match == 0)
			release ();
	}

	message (sock, "PRIVMSG %s :Vous n'avez pas les permissions pour fermer le bot\r\n",
			data->channel);
}

void handle_help (int sock, struct msg_t *data, int argc, char **argv)
{
	int i, j;

	message (sock, "PRIVMSG %s :Affichage de l'aide requit par %s :\r\n",
			data->channel, data->nick);

	if (argc > 1) /* arguments */
	{
		for (i = 1; i < argc; ++i)
		{
			for (j = 0; commands[j].name != NULL; ++j)
				if (!strcmp (argv[i], commands[j].name))
					break;
			
			if (commands[j].name != NULL)
			{
				message (sock, "PRIVMSG %s :%s : %s\r\n",
						data->channel, argv[i], commands[j].desc);
			}
			else
			{
				message (sock, "PRIVMSG %s :Commande inconnue : %s\r\n",
						data->channel, argv[i]);
			}
		}
	}
	else /* no arguments */
	{
		for (i = 0; commands[i].name != NULL; ++i)
		{
			message (sock, "PRIVMSG %s :%s : %s\r\n",
					data->channel,
					commands[i].name,
					commands[i].desc);
		}
	}
}

void handle_rss (int sock, struct msg_t *data, int argc, char **argv)
{
	message (sock, "PRIVMSG %s :Affichage des derniers flux RSS enregistrés, requit par %s :\r\n",
			data->channel, data->nick);

	handle_news (sock, data, argc, argv);
	handle_journal (sock, data, argc, argv);
	handle_ask (sock, data, argc, argv);
	handle_forum (sock, data, argc, argv);
	handle_wiki (sock, data, argc, argv);
}

void handle_news (int sock, struct msg_t *data, int argc, char **argv)
{
	message (sock, "PRIVMSG %s :News: %s < %s >\r\n",
			data->channel,
			global->title.news,
			global->link.news);
}

void handle_journal (int sock, struct msg_t *data, int argc, char **argv)
{
	message (sock, "PRIVMSG %s :Journal: %s < %s >\r\n",
			data->channel,
			global->title.journal,
			global->link.journal);
}
	
void handle_ask (int sock, struct msg_t *data, int argc, char **argv)
{
	message (sock, "PRIVMSG %s :Demande: %s < %s >\r\n",
			data->channel,
			global->title.ask,
			global->link.ask);
}
	
void handle_forum (int sock, struct msg_t *data, int argc, char **argv)
{
	message (sock, "PRIVMSG %s :Forum: %s < %s >\r\n",
			data->channel,
			global->title.msg,
			global->link.msg);
}

void handle_wiki (int sock, struct msg_t *data, int argc, char **argv)
{
	message (sock, "PRIVMSG %s :Wiki: %s < %s >\r\n",
			data->channel,
			global->title.wiki,
			global->link.wiki);

}
