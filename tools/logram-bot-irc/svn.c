#include "local.h"

void parse_svn (FILE *commit, int s)
{
	char tmp0[BUF_LENGTH];
	char tmp1[BUF_LENGTH];
	char tmp2[BUF_LENGTH];
	char *p;

	if (fgets (tmp0, BUF_LENGTH, commit) == NULL
		|| fgets (tmp1, BUF_LENGTH, commit) == NULL
		|| fgets (tmp2, BUF_LENGTH, commit) == NULL)
	{
		return;
	}

	if ((p = strchr (tmp0, '\n')) != NULL)
		*p = 0;
	if ((p = strchr (tmp1, '\n')) != NULL)
		*p = 0;
	if ((p = strchr (tmp1, '\n')) != NULL)
		*p = 0;

	if (global->svn.rev == NULL || global->svn.nick == NULL || global->svn.msg == NULL
		|| strcmp (tmp0, global->svn.rev)
		|| strcmp (tmp1, global->svn.nick)
		|| strcmp (tmp2, global->svn.msg))
	{
		if (global->svn.rev != NULL) free (global->svn.rev);
		if (global->svn.nick != NULL) free (global->svn.nick);
		if (global->svn.msg != NULL) free (global->svn.msg);

		asprintf (&global->svn.rev, tmp0);
		asprintf (&global->svn.nick, tmp1);
		asprintf (&global->svn.msg, tmp2);

		message (s, "PRIVMSG %s :SVN: %s (%s): %s\r\n", global->config.channel, global->svn.nick, global->svn.rev, global->svn.msg);
	}

}

void get_svn (int s)
{
	FILE *commit = NULL;

	system ("sh ./svn.sh");

	if ((commit = fopen ("svn.txt", "r")) != NULL)
	{
		parse_svn (commit, s);
		fclose (commit);
	}
}
