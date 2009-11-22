#include "local.h"

void parse_rss (FILE *feeds, char **title, char **link, char *type, int s)
{
	char tmp0[BUF_LENGTH];
	char tmp1[BUF_LENGTH];
	char *p;

	if (fgets (tmp0, BUF_LENGTH, feeds) == NULL || fgets (tmp1, BUF_LENGTH, feeds) == NULL
		|| title == NULL || link == NULL)
		return;

	if ((p = strchr (tmp0, '\n')) != NULL)
		*p = 0;
	if ((p = strchr (tmp1, '\n')) != NULL)
		*p = 0;

	if ((*title == NULL) || (*link == NULL) || strcmp (tmp0, *title) || strcmp (tmp1, *link))
	{
		if (*title != NULL) free (*title);
		if (*link != NULL)  free (*link);

		asprintf (title, tmp0);
		asprintf (link, tmp1);
		message (s, "PRIVMSG %s :%s: %s < %s >\r\n", global->config.channel, type, *title, *link);
	}
}

void get_rss (int s)
{
	FILE *feeds = NULL;
	
	system ("sh ./rss.sh");
	
	if ((feeds = fopen ("rss.txt", "r")) != NULL)
	{
		parse_rss (feeds, &global->title.news,    &global->link.news,    "News",    s);	
		parse_rss (feeds, &global->title.journal, &global->link.journal, "Journal", s);
		parse_rss (feeds, &global->title.ask,     &global->link.ask,     "Demande", s);	
		parse_rss (feeds, &global->title.msg,     &global->link.msg,     "Forum",   s);	
		parse_rss (feeds, &global->title.wiki,    &global->link.wiki,    "Wiki",    s);	
		parse_rss (feeds, &global->title.pkg,     &global->link.pkg,     "Paquet",  s);	
		fclose (feeds);
	}
}




