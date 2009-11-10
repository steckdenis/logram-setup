#include "local.h"

void config_load (const char *filename)
{
	FILE *cfg = fopen (filename, "r");
	char buf[512], *p;
	int i = 0;

	if (cfg != NULL)
	{
		while (fgets (buf, 512, cfg))
		{
			if ((p = strchr (buf, '\n')) != NULL) *p = 0;
			p = strchr (buf, '=');

			/* server & channel */
			if (!strncmp (buf, "server=", 7))
				asprintf (&global->config.server, "%s", p + 1);
			else if (!strncmp (buf, "port=", 5))
				global->config.port = strtol (p + 1, NULL, 10);
			else if (!strncmp (buf, "chan=", 5))
				asprintf (&global->config.channel, "%s", p + 1);
			/* bot */
			else if (!strncmp (buf, "admin=", 6))
				asprintf (&global->config.admin, "%s", p + 1);
			else if (!strncmp (buf, "nick=", 5))
				asprintf (&global->config.nick, "%s", p + 1);
			else if (!strncmp (buf, "host=", 5))
				asprintf (&global->config.host, "%s", p + 1);
			else if (!strncmp (buf, "ident=", 6))
				asprintf (&global->config.ident, "%s", p + 1);
			else if (!strncmp (buf, "name=", 5))
				asprintf (&global->config.name, "%s", p + 1);
			else
				fprintf (stderr, "Error in %s:%d.\n", filename, +i);
		}

		fclose (cfg);
	}
	else
		fprintf (stderr, "Couldn't find %s.\n", filename);
}
