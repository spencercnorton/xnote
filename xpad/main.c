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
	1, /* toolbar on by default */
	1, /* scrollbars on by default */
	NULL /* list of buttons -- default is filled in upon file load*/
};

static gint at_gtk_exit (gpointer data)
{
	if (verbosity >= 1) printf ("xpad is shutting down.\n");
	pref_close ();
	cleanup ();
	
	g_free (working_dir);
	g_slist_free (current_settings.toolbar_buttons);
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


static void
print_help (void)
{
	printf ("Usage: xpad [OPTIONS]\n"
	        "\n"
	        "  -V, --version         prints xpad version; exits\n"
	        "  -h, --help            prints this usage information; exits\n"
	        "  -v N, --verbosity=N   sets level of output\n"
	        "                          0=none, 1=moderate, 2=debug\n"
	        "                          default is 0\n"
	        "  -n, --new             opens a new pad\n"
	        "  -q, --quit            quits all open xpad sessions\n");
	exit (0);
}

static void
print_version (void)
{
	printf ("xpad v%s\n", VERSION);
	exit (0);
}

static void
set_verbosity (gint *v)
{
	if (*v < 0 || *v > 2)
	{
		fprintf (stderr, "Illegal verbosity value.  Must be between 0 and 2 inclusive.\n");
		exit (1);
	}
	
	verbosity = *v;
}

struct argument_def
{
	gboolean local;
	const gchar *name;
	gboolean second;
	union {
		void (*func) (void);
		void (*func_arg) (void *);
	} callbacks;
};
typedef struct argument_def argument;

static const argument arguments[] =
{
	{TRUE, "-h", FALSE, {print_help}},
	{TRUE, "--help", FALSE, {print_help}},
	{TRUE, "-V", FALSE, {print_version}},
	{TRUE, "--version", FALSE, {print_version}},
	{TRUE, "-v", TRUE, {G_CALLBACK (set_verbosity)}},
	{TRUE, "--verbosity", TRUE, {G_CALLBACK (set_verbosity)}},
	
	{FALSE, "-n", FALSE, {G_CALLBACK (pad_new)}},
	{FALSE, "--new", FALSE, {G_CALLBACK (pad_new)}},
	{FALSE, "-q", FALSE, {gtk_main_quit}},
	{FALSE, "--quit", FALSE, {gtk_main_quit}}
};

#define NUM_ARGUMENTS (sizeof (arguments) / sizeof (argument))


static void missing_companion_arg(const char argname[])
{
	fprintf(stderr, "Missing companion argument to %s\n", argname);
	exit(1);
}


static gint handle_args (int *argc, char ***argv, gboolean local)
{
	gint i, j;
	gint rv = 0;
	size_t arglen[NUM_ARGUMENTS];

	/* Set up array of argument lengths to avoid having to compute them every 
	 * time through our inner loop.  This probably ought to be global, but it
	 * won't matter all that much.
	 */
	for (j = NUM_ARGUMENTS-1; j >= 0; j--)
		arglen[j] = strlen(arguments[j].name);

	for (i = 1; i < *argc; i++)
	{
		gboolean longform;

		/* Find matching argument; there can be only one.  Loop reversed for 
		 * better performance (well, perhaps more out of sheer habit).
		 */
		for (j = NUM_ARGUMENTS-1; 
		     (j >= 0) && (strncmp((*argv)[i], arguments[j].name, arglen[j]) != 0);
		     j--)
		{
		}

		if (j < 0)
		{
		  	/* Argument not found in list.  Either its "local" setting mismatched
			 * the one passed to us, or we got an invalid argument.  Check for the
			 * latter only if we're doing local; otherwise just ignore the argument.
			 */
			if (local)
			{
				fprintf (stderr, "Didn't understand argument %s.\n", (*argv)[i]);
				exit(1);
			}
			else
			{
				continue;
			}
		}


		/* (from this point on, we know our argument was recognized) */

		longform = (strncmp(arguments[j].name, "--", 2) == 0);

		if (arguments[j].local != local)
		{
			/* We ignore this argument, but if it is in short form and expects a
			 * companion argument, make sure we skip the companion argument on our
			 * next iteration.
			 */
			if (!longform && arguments[j].second) i++;

			/* Don't accept this argument, but don't complain either. */
			continue;
		}

		if (arguments[j].second)
		{
			/* right now we only do integer arguments... */
			gint arg;
			char *companion, *endptr;
			
			if (longform)
			{
				if ((*argv)[i][arglen[j]] != '=') 
				  	missing_companion_arg(arguments[j].name);
				
				companion = (*argv)[i] + arglen[j] + 1;
			}
			else
			{
				i++;
				if (i >= *argc)
				  	missing_companion_arg(arguments[j].name);
				
				companion = (*argv)[i];
			}
			
			if (!*companion) missing_companion_arg(arguments[j].name);
			
			arg = strtol(companion, &endptr, 10);
			
			if (*endptr)
			{
				fprintf(stderr, "Invalid number: '%s'\n", companion);
				exit(1);
			}
			
			arguments[j].callbacks.func_arg(&arg);
		}
		else
		{
			arguments[j].callbacks.func();
		}
		rv++;
	}
	
	return rv;
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

/*
converts main program arguments into one long string.
puts allocated string in dest, and returns size
*/
static gint
args_to_string (int *argc, char ***argv, char **dest)
{
	gint i;
	gint size = 0;
	gint extra;
	gchar num [11];
	gchar *p;
	
	for (i = 0; i < *argc; i++)
		size += strlen ((*argv)[i]) + 1;
	
	sprintf (num, "%i", *argc);
	
	extra = strlen (num) + 1;
	
	*dest = g_malloc (size + extra);
	
	strcpy (*dest, num);
	p = *dest + extra;
	
	for (i = 0; i < *argc; i++)
	{
		strcpy (p, (*argv)[i]);
		p += strlen ((*argv)[i]) + 1;
	}
	
	if (*argc == 1)
	{
	}
	
	return size + extra;
}

/*
returns number of strings in newly allocated argv
*/
static gint
string_to_args (const char *string, char ***argv)
{
	gint num, i;
	char **list;
	
	num = atoi (string);
	string += strlen (string) + 1;
	
	list = g_malloc (sizeof (char *) * num);
	
	for (i = 0; i < num; i++)
	{
		list[i] = g_malloc (strlen (string) + 1);
		strcpy (list[i], string);
		string += strlen (string) + 1;
	}
	
	*argv = list;
	
	return num;
}

static void
clipboard_get (GtkClipboard *clipboard, GtkSelectionData 
	*selection_data, guint info, gpointer data)
{
	switch (info)
	{
	case 1:
		/* Fill the selection with nonsense data -- it is not used.  We are just using 
		   the clipboard as a message passer.  On a 1, which is a 'are you alive?' ping,
		   respond.  The other client will see this data and leave; we take
		   over his arguments. */
		gtk_selection_data_set (selection_data, 
			gdk_atom_intern ("_XPAD_EXISTS", FALSE),
			8,
			(const guchar *) "",
			0);
	default:
		break;
	}
}

static void
clipboard_clear (GtkClipboard *clipboard, gpointer data)
{
	/* no data needs to be freed, but we need to ask the other xpad that took our data
	    what the arguments were.*/
	
	GtkSelectionData *temp;
	
	/* set up target list with simple string target w/ value of 1 */
	GtkTargetEntry targets[] = {{"STRING", 0, 1}};
	
	if ((temp = gtk_clipboard_wait_for_contents (clipboard, 
		gdk_atom_intern ("STRING", FALSE))))
	{
		gint argc;
		gchar **argv;
		gint i;
		
		argc = string_to_args ((char *) temp->data, &argv);
		
		if (!handle_args (&argc, &argv, FALSE))
		{
			/* if there were no non-local arguments, insert --new as argument */
			gint c = 2;
			gchar **v = g_malloc (sizeof (gchar *) * 2);
			v[0] = "xpad";
			v[1] = "--new";
			
			handle_args (&c, &v, FALSE);
			
			g_free (v);
		}
		
		for (i = 0; i < argc; i++)
			g_free (argv[i]);
		
		g_free (argv);
		
		gtk_selection_data_free (temp);
		
		/* claim clipboard again */
		gtk_clipboard_set_with_data (clipboard, targets, 1, 
			clipboard_get, clipboard_clear, NULL);
	}
}

static void
newxpad_clipboard_clear (GtkClipboard *clipboard, gpointer data)
{
	/* other xpad got our message, let's get the hell out of here */
	gtk_main_quit ();
}

static void
newxpad_clipboard_get (GtkClipboard *clipboard, GtkSelectionData 
	*selection_data, guint info, gpointer data)
{
	gpointer *newdata;
	gint size;
	gchar *args;
	
	newdata = (gpointer *) data;
	
	switch (info)
	{
	case 1:
		size = args_to_string (newdata[0], newdata[1], &args);
		
		/* Fill the selection with our arguments. */
		gtk_selection_data_set (selection_data, 
			gdk_atom_intern ("_XPAD_EXISTS", FALSE),
			8,
			(const guchar *) args,
			size);
		g_free (args);
	default:
		break;
	}
}

static gint xpad_check_if_others (gpointer data)
{
	GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern ("_XPAD_EXISTS", FALSE));
	GtkSelectionData *temp;
	
	/* set up target list with simple string target w/ value of 1 */
	GtkTargetEntry targets[] = {{"STRING", 0, 1}};
		
	if ((temp = gtk_clipboard_wait_for_contents (clipboard, 
		gdk_atom_intern ("STRING", FALSE))))
	{
		/* If there was anything in the clipboard, that means there is another
		     xpad session going on, and so we exit after posting our arguments. */
		gtk_selection_data_free (temp);
		
		gtk_clipboard_set_with_data (clipboard, targets, 1,
			newxpad_clipboard_get, newxpad_clipboard_clear, data);
		
		return 1;
	}
	else
	{
		/* no one else is alive.  claim the clipboard. */
		
		gtk_clipboard_set_with_data (clipboard, targets, 1, 
			clipboard_get, clipboard_clear, NULL);
		
		return 0;
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

/* returns 1 if files were made, 0 else */
static gint
xpad_make_needed_files (void)
{
	gchar *defaults;
	gint rv = 0;
	
	defaults = g_build_filename (working_dir, DEFAULTS_FILENAME, NULL);
	
	if (!g_file_test (defaults, G_FILE_TEST_EXISTS))
	{
		fio_set_file (defaults, "");	/* make it start empty -- defaults will be filled in*/
		rv = 1;
	}
	
	g_free (defaults);
	
	return rv;
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


#ifdef G_OS_UNIX

/* Try to intercept a signal, but don't do so if it was previously set to be ignored -- we
 * should respect non-job-control shells and such. 
 */
static void SetSignal(int signum)
{
	if (signal(signum, xpad_catch_quit_signal) == SIG_IGN)
		signal(signum, SIG_IGN);
}


/* try to intercept various quit-style signals. */
static void SetQuitSignals(void)
{
	SetSignal(SIGHUP);
	SetSignal(SIGINT);
	SetSignal(SIGQUIT);
	SetSignal(SIGABRT);
	SetSignal(SIGTERM);
}

#else

static void SetQuitSignals(void)
{
}

#endif


static gboolean
xpad_initial_save (gpointer data)
{
	/* we want to make sure we save any new format changes */
	fio_save_default_settings ();
	
	/* save open pads */
	fio_save_pads ();
	
	return FALSE;	/* remove ourselves from idle list */
}

/* data is an array of void pointers, indicating the argc and argv */
static int xpad_init (gpointer data)
{
	gint first_time;
	gpointer *newdata;
	
	newdata = (gpointer *) data;
	
	gtk_quit_add (0, at_gtk_exit, NULL);
	
	if (xpad_check_if_others (data))
		return 0;
	
	SetQuitSignals();
	
	xpad_make_working_dir ();
	
	first_time = xpad_make_needed_files ();
	
	fio_load_default_settings ();
	
	/* save contents every "sync_time" seconds */
	reset_sync ();
	
	if (verbosity >= 2)
		printf ("Sync time is set to %i.\nVerbosity is set to %i.\nDecorations is set to %i\n",
			current_settings.sync_time, verbosity, current_settings.decorations);
	
	xpad_set_default_icon ();
	xpad_register_icons ();
	
	/* load all pads */
	fio_load_pads();
	
	handle_args (newdata[0], newdata[1], FALSE);
	
	/* when we get free time, save all the settings */
	g_idle_add (xpad_initial_save, NULL);
	
	if (first_time)
		show_help ();	/* if no defaults file, assume it is their first time and show some help */
	
	return 0;
}

int main (int argc, char *argv[])
{
	gpointer args[2];
	
	gtk_init(&argc, &argv);
	
	handle_args (&argc, &argv, TRUE);
	
	args[0] = &argc;
	args[1] = &argv;
	gtk_init_add (xpad_init, args);
	
	gtk_main ();
	
	return 0;
}
