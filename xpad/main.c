/*

Copyright (c) 2001-2002 Michael Terry

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

#include "xpad.xpm"
#include "main.h"
#include "pad.h"
#include "help.h"
#include "fio.h"
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <gdk/gdkkeysyms.h>

gchar working_dir[MAX_FILENAME_SIZE];
size_t working_dir_len = 0;
gint verbosity = 0; /* output level */
guint autosave_timeout_id = -1;

/**
 * This variable holds all the changeable settings for this session.
 * It should be a copy of the defaults file.
 *
 * Here, we populate it with hardcoded defaults in case we can not
 * find values in the defaults file.
 */
struct settings current_settings =
{
	260, /* default width */
	260, /* default height */
	60, /* sync time in seconds */
	0, /* decorations are off */
	1, /* destroy confirmations on */
	0, /* edit lock off */
	1, /* close this pad */
	{ /* default style */
		{0, 0xe000, 0xe000, 0x5600}, /* yellow background */
		{0, 0, 0, 0}, /* black text */
		{0, 0, 0, 0}, /* black border */
		0, /* border width */
		0, /* padding */
		"serif Bold 16" /* font */
	}
};

gint at_gtk_exit (gpointer data)
{
	if (verbosity >= 1) printf ("xpad is shutting down.\n");
	cleanup ();
	return 0;
}

static void sigcatch (int signum)
{
	if (verbosity >= 2) printf ("xpad caught a signal.  Shutting down.\n");
	gtk_main_quit ();
}


/* 
   TODO: make args more OO, with registering of each possible param
         and systematic treatment of short/long names and callbacks
*/
static void handle_args (int *argc, char ***argv)
{
	gint i;

	for (i = 1; i < *argc; i++)
	{
		int shortform = 0;

		if (!strcmp ((*argv)[i], "--version") || (shortform = !strcmp ((*argv)[i], "-V")))
		{
			printf ("xpad v%s\n", VERSION);
			gtk_main_quit ();
		}
		else if (!strcmp ((*argv)[i], "--help") || (shortform = !strcmp ((*argv)[i], "-h")))
		{
			printf ("Usage: xpad [OPTIONS]\n");
			printf ("\n");
			printf ("  -V, --version         print xpad version; exit\n");
			printf ("  -h, --help            print this usage information; exit\n");
			printf ("  -v N, --verbosity=N   set level of output\n");
			printf ("                          0=none, 1=moderate, 2=debug\n");
			printf ("                          default is 0\n");
			gtk_main_quit ();
		}
		else if (!strncmp ((*argv)[i], "--verbosity=", 12) || (shortform = !strcmp ((*argv)[i], "-v")))
		{
			gint temp;
			gchar *value;

			if (shortform)
			{
				if (++i == *argc)
				{
					printf ("Missing companion argument to -v.\n");
					gtk_main_quit ();
				}

				value = (*argv)[i];
			}
			else
				value = (gchar *) strchr ((*argv)[i], '=') + 1;

			temp = atoi (value);

			if (temp < 0 || temp > 2)
			{
				printf ("Illegal verbosity value.  Must be between 0 and 2 inclusive.\n");
				gtk_main_quit ();
			}
			else
				verbosity = temp;
		}
		else
			printf ("Didn't understand argument %s.\n", (*argv)[i]);
	}
}

/* an occasional checkup to sync contents. */
static int sync_pads (gpointer data)
{
	if (verbosity >= 1) printf ("Auto-saving pads.\n");

	fio_save_pads ();

	return 1;
}


static void reset_sync (void)
{
	if (autosave_timeout_id > 0)
		gtk_timeout_remove (autosave_timeout_id);

	if (current_settings.sync_time)
		autosave_timeout_id = gtk_timeout_add (current_settings.sync_time * 1000, sync_pads, NULL);
	else
		autosave_timeout_id = -1;

}


static void xpad_set_default_icon (void)
{
	GdkPixmap *pixmap;
	GdkPixbuf *pixbuf;
	GdkPixbuf *pixbuf_full;

	pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gdk_colormap_get_system (), NULL, NULL, xpad_xpm);
	
	pixbuf = gdk_pixbuf_get_from_drawable (NULL, pixmap, NULL, 0, 0, 0, 0, 48, 48);
	
	if (pixbuf)
	{
		pixbuf_full = gdk_pixbuf_add_alpha (pixbuf, TRUE, 0, 0, 0);
		
		gtk_window_set_default_icon_list (g_list_append (NULL, pixbuf_full));
		
		g_object_unref (pixbuf);
		g_object_unref (pixbuf_full);
	}
	
	g_object_unref (pixmap);
}

static void clipboard_clear (GtkClipboard *clipboard, gpointer data)
{
	/* no data needs to be freed */
}

static void clipboard_get (GtkClipboard *clipboard, GtkSelectionData 
	*selection_data, guint info, gpointer data)
{
	switch (info)
	{
	case 1:
		/* Fill the selection with nonsense data -- it is not used.  We are just using 
		   the clipboard as a message passer.  On a 1, which is a 'are you alive?' ping,
		   create a new pad.  The other client will see this data and leave; we take
		   over his pad. */
		pad_new ();
		gtk_selection_data_set (selection_data, 
			gdk_atom_intern ("_XPAD_EXISTS", FALSE),
			8,
			"",
			0);
	default:
		break;
	}
}

static void xpad_check_if_others (void)
{
	GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern ("_XPAD_EXISTS", FALSE));
	GtkSelectionData *temp;
	
	if ((temp = gtk_clipboard_wait_for_contents (clipboard, 
		gdk_atom_intern ("STRING", FALSE))))
	{
		/* If there was anything in the clipboard, that means there is another
		     xpad session going on, and so we exit (the other session knows we
		     tried to start, and will make a new pad. */
		gtk_selection_data_free (temp);
		gtk_main_quit ();
	}
	else
	{
		/* set up target list with simple string target w/ value of 1 */
		GtkTargetEntry targets[] = {{"STRING", 0, 1}};
		
		gtk_clipboard_set_with_data (clipboard, targets, 1, 
			clipboard_get, clipboard_clear, NULL);
	}
}

/* data is an array of void pointers, indicating the argc and argv */
static int xpad_init (gpointer data)
{
	gpointer *newdata;
	struct sigaction sa;
	
	gtk_quit_add (0, at_gtk_exit, NULL);
	
	newdata = (gpointer *) data;
	handle_args (newdata[0], newdata[1]);
	
	xpad_check_if_others ();
	
	/* Initialize sa */
	sa.sa_handler = sigcatch;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction (SIGHUP, &sa, NULL);  /* 1 hangup */
	sigaction (SIGINT, &sa, NULL);  /* 2 interrupt */
	sigaction (SIGQUIT, &sa, NULL); /* 3 quit */
	sigaction (SIGABRT, &sa, NULL); /* 6 abort */
	sigaction (SIGKILL, &sa, NULL); /* 9 kill */
	sigaction (SIGTERM, &sa, NULL); /*15 terminate */
	
	working_dir[sizeof(working_dir)-1] = '\0';
	strncpy (working_dir, g_get_home_dir (), sizeof(working_dir));

	/* Oops--working dir is too long!  Use . instead (which sucks...) */
	if (working_dir[sizeof(working_dir)-1])
	{
		fprintf(stderr, "Working directory too long, using '.'\n");
		strcpy(working_dir, ".");
	}

	strcat (working_dir, "/.xpad/");
	working_dir_len = strlen(working_dir);
	
	/* try to make sure directory exists */
	mkdir (working_dir, 00777);

	fio_get_values_from_file (DEFAULTS_FILENAME, 
						  "decorations", &current_settings.decorations,
						  "sync_time", &current_settings.sync_time,
						  "height", &current_settings.height,
						  "width", &current_settings.width,
						  "confirm_destroy", &current_settings.confirm_destroy,
						  "edit_lock", &current_settings.edit_lock,
						  "wm_close", &current_settings.wm_close,
						  NULL);

	if (fio_get_style_from_file (DEFAULTS_FILENAME, &current_settings.style))
	{
		/* this happens if there isn't a ~/.xpad directory (i.e. first run) */
		fio_save_as_defaults (&current_settings);
		show_help ();
	}
	
	/* save contents every "sync_time" seconds */
	reset_sync ();
	
	if (verbosity >= 2)
		printf ("Sync time is set to %i.\nVerbosity is set to %i.\nDecorations is set to %i\n",
			current_settings.sync_time, verbosity, current_settings.decorations);
	
	xpad_set_default_icon ();
	
	/* load all pads */
	fio_load_pads();
	
	return 0;
}

int main (int argc, char *argv[])
{
	gpointer args[2];
	
	gtk_set_locale ();
	gtk_init(&argc, &argv);
	
	args[0] = &argc;
	args[1] = &argv;
	gtk_init_add (xpad_init, args);
	
	gtk_main ();
	
	return 0;
}
