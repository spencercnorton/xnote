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
#include "lock.xpm"
#include "main.h"
#include "pad.h"
#include "help.h"
#include "fio.h"
#include <stdlib.h>
#include <string.h>

#if defined (G_OS_UNIX)
 
 #include <signal.h>
 
 /* required by mkdir */
 #include <sys/stat.h>
 #include <sys/types.h>
 
#elif defined (G_OS_WIN32)
 
 /* required by CreateDirectory */
 #include <Winbase.h>
 
#elif defined (G_OS_BEOS)
 
 /* required by CreateDirectory */
 #include <be/storage/Directory.h>
 
#endif

gchar *working_dir;
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
		5, /* padding */
		"serif Bold 16" /* font */
	},
	NULL /* list of buttons -- default is filled in upon file load*/
};

static void g_free_helper (gpointer p1, gpointer p2)
{
	g_free (p1);
}

static gint at_gtk_exit (gpointer data)
{
	if (verbosity >= 1) printf ("xpad is shutting down.\n");
	cleanup ();
	g_free (working_dir);
	
	g_slist_foreach (current_settings.toolbar, g_free_helper, NULL);
	g_slist_free (current_settings.toolbar);
	return 0;
}

void xpad_display_dialog_with_text (GtkMessageType type, const gchar *text)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL,
        		GTK_DIALOG_MODAL,
        		type,
        		GTK_BUTTONS_CLOSE,
        		text);

	gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

static void xpad_catch_quit_signal (int signum)
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
			(const guchar *) "",
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
		/* no one else is alive.  claim the clipboard. */
		
		/* set up target list with simple string target w/ value of 1 */
		GtkTargetEntry targets[] = {{"STRING", 0, 1}};
		
		gtk_clipboard_set_with_data (clipboard, targets, 1, 
			clipboard_get, clipboard_clear, NULL);
	}
}

/**
 * Sets the value of |working_dir| and creates the directory if needed.
 */
static void xpad_make_working_dir (void)
{
#if defined (G_OS_UNIX)
	
	/* create a hidden directory under the user's home */
	working_dir = g_build_filename (g_get_home_dir (), ".xpad", NULL);
	
	/* make sure directory exists */
	mkdir (working_dir, 00770); /* give group and user all rights */
	
#elif defined (G_OS_WIN32)
	
	/* If someone has a better place to put our stuff, I'm all ears. */
	working_dir = g_build_filename (g_get_home_dir (), "xpad", NULL);
	
	/* make sure directory exists */
	CreateDirectory (working_dir, NULL); /* default security rights */
	
#elif defined (G_OS_BEOS)
	
	/* If someone has a better place to put our stuff, I'm all ears. */
	working_dir = g_build_filename (g_get_home_dir (), "xpad", NULL);
	
	/* make sure directory exists */
	CreateDirectory (working_dir, NULL);
	
#endif
}

static void xpad_register_icons (void)
{
	GtkIconSet *set;
	GtkIconFactory *factory;
	GdkPixbuf *pixbuf;
	
	pixbuf = gdk_pixbuf_new_from_xpm_data (lock_xpm);
	
	set = gtk_icon_set_new_from_pixbuf (pixbuf);
	
	factory = gtk_icon_factory_new ();
	gtk_icon_factory_add (GTK_ICON_FACTORY (factory),
		"xpad-lock", set);
	gtk_icon_factory_add_default (GTK_ICON_FACTORY (factory));
	
	g_object_unref (pixbuf);
}

static void xpad_set_default_icon (void)
{
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpad_xpm);
	
	gtk_window_set_default_icon_list (g_list_append (NULL, pixbuf));
	
	g_object_unref (pixbuf);
}

/* data is an array of void pointers, indicating the argc and argv */
static int xpad_init (gpointer data)
{
	gpointer *newdata;
	
	gtk_quit_add (0, at_gtk_exit, NULL);
	
	newdata = (gpointer *) data;
	handle_args (newdata[0], newdata[1]);
	
	xpad_check_if_others ();
	
#ifdef G_OS_UNIX
	
	/* try to intercept various quit-style signals.  don't do so if they were previously
	    ignored, however -- we should respect non-job-control shells and such */
	if (signal (SIGHUP, xpad_catch_quit_signal) == SIG_IGN)
		signal (SIGHUP, SIG_IGN);
	if (signal (SIGINT, xpad_catch_quit_signal) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGQUIT, xpad_catch_quit_signal) == SIG_IGN)
		signal (SIGQUIT, SIG_IGN);
	if (signal (SIGABRT, xpad_catch_quit_signal) == SIG_IGN)
		signal (SIGABRT, SIG_IGN);
	if (signal (SIGTERM, xpad_catch_quit_signal) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);
	
#endif
	
	xpad_make_working_dir ();
	
	if (fio_load_default_settings ())
	{
		/* this happens if there isn't a default-style (i.e. first run) */
		show_help ();
	}
	
	/* we want to make sure we save any new format changes */
	fio_save_default_settings ();

	/* save contents every "sync_time" seconds */
	reset_sync ();
	
	if (verbosity >= 2)
		printf ("Sync time is set to %i.\nVerbosity is set to %i.\nDecorations is set to %i\n",
			current_settings.sync_time, verbosity, current_settings.decorations);
	
	xpad_set_default_icon ();
	xpad_register_icons ();
	
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
