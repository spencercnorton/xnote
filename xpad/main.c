/*

Copyright (c) 2001 Michael Terry

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "main.h"
#include "pad.h"
#include "fio.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/* static data */
gchar working_dir[1024];
const gchar *VERSION = "xpad v0.2.1";
gint update_time = 60000;

void sigcatch (int signum)
{
	cleanup ();

	gtk_exit (0);
}

void handle_args (int *argc, char ***argv)
{
	gint i;
	char *arg;

	for (i = 0; i < *argc; i++)
	{
		if (strcmp ((*argv)[i], "--version") == 0)
		{
			printf ("%s\n", VERSION);
			gtk_exit (0);
		}
	}
}

/* an every-minute checkup. */
int checkup (gpointer data)
{
	save_pads ();
}

void xpad_init (int *argc, char ***argv)
{
	struct sigaction sa;

	handle_args (argc, argv);

	/* save contents every minute */
	gtk_timeout_add (update_time, checkup, NULL);

	/* Initialize sa */
	sa.sa_handler = sigcatch;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction (SIGINT, &sa, NULL);
	sigaction (SIGHUP, &sa, NULL);
	sigaction (SIGKILL, &sa, NULL);
	sigaction (SIGQUIT, &sa, NULL);
	sigaction (SIGTERM, &sa, NULL);
	sigaction (SIGTRAP, &sa, NULL);
	sigaction (SIGABRT, &sa, NULL);

	strcpy (working_dir, getenv("HOME"));
	strcat (working_dir, "/.xpad/");

	/* load default info */
	current_info = DEFAULT_INFO;

	/* load all pads */
	load_pads();
}

int main (int argc, char *argv[])
{
	gtk_set_locale ();
	gtk_init(&argc, &argv);

	xpad_init (&argc, &argv);

	gtk_main ();

	cleanup ();

	gtk_exit (0);

	return 0;
}











