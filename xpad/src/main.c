/*

Copyright (c) 2001-2003 Michael Terry

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

/* define _GNU_SOURCE here because that makes our sockets work nice
 Unfortunately, we lose portability... */
#define _GNU_SOURCE	1

#include "../images/xpad.xpm"
#include "../images/lock.xpm"
#include "../images/sticky.xpm"
#include "main.h"
#include "pad.h"
#include "help.h"
#include "fio.h"
#include "tray.h"
#include "settings.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#if defined (G_OS_UNIX)
 
 #include <signal.h>
 
 /* required by mkdir */
 #include <sys/stat.h>
 #include <sys/types.h>
 
 /* required by socket stuff */
 #include <sys/un.h>
 #include <sys/socket.h>
 #include <sys/select.h>
 
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

gboolean make_new_pad = TRUE;
gboolean open_old_pads = TRUE;
gint master_fd = -1;
FILE *output;
gchar *master_name = NULL;

static gint at_gtk_exit (gpointer data)
{
	if (verbosity >= 1) printf ("xpad is shutting down.\n");
	pref_close ();
#ifdef G_OS_UNIX
	tray_close ();
#endif
	cleanup ();
	
	if (master_name)
	{
		close (master_fd);
		master_fd = -1;
		unlink (master_name);
		g_free (master_name);
	}
	
	g_free (working_dir);
	
	xpad_settings_shutdown ();
	
	return 0;
}

/**
 * Creates an alert with 'stock' used to create an icon and parent text of 'parent',
 * secondary text of 'secondary'.  No buttons are added.
 */
GtkWidget *xpad_alert_new (GtkWindow *parent, const gchar *stock, const gchar *primary, const gchar *secondary)
{
	GtkWidget *dialog, *hbox, *image, *label;
	gchar buf [1024];
	
	dialog = gtk_dialog_new_with_buttons (
		"",
		parent,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
		NULL);
	
	hbox = gtk_hbox_new (FALSE, 12);
	image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);
	
	sprintf (buf, "<span weight=\"bold\" size=\"larger\">%s</span>", primary);
	if (secondary)
		sprintf (buf, "%s\n\n%s", buf, secondary);
	
	gtk_label_set_markup (GTK_LABEL (label), buf);
	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
	gtk_container_add (GTK_CONTAINER (hbox), image);
	gtk_container_add (GTK_CONTAINER (hbox), label);
	
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	
	gtk_widget_show_all (hbox);
	
	return dialog;
}

/* must be at least primary */
void xpad_show_error (GtkWindow *parent, const gchar *primary, const gchar *secondary)
{
	GtkWidget *dialog;
	
	fprintf (stderr, "%s\n", primary);
	
	dialog = xpad_alert_new (parent, GTK_STOCK_DIALOG_ERROR,
		primary,
		secondary);
	
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, 1, NULL);
	
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
	fprintf (output,
		_("Usage: xpad [OPTIONS]\n"
	        "\n"
	        "  -V, --version         prints xpad version and exits\n"
	        "  -h, --help            prints this usage information and exits\n"
	        "  -v N, --verbosity=N   sets level of output\n"
	        "                          0=none, 1=moderate, 2=debug\n"
	        "                          default is 0\n"
	        "  -n, --new             opens a new pad only\n"));
	fprintf (output,
		_("  --nonew               prevents xpad from making a new pad\n"
	        "  -q, --quit            quits all open xpad sessions\n"
		"  -l, --list            lists the titles of all pads\n"
		"  -s N, --show=N        brings the Nth pad (1-based) to the foreground\n"
	        "  --showall             brings all pads to the foreground\n"));
	exit (0);
}

static void
print_version (void)
{
	fprintf (output, "xpad %s\n", VERSION);
	exit (0);
}


static void
set_nonew (void)
{
	make_new_pad = FALSE;
}

static void
set_new (void)
{
	open_old_pads = FALSE;
}

static void
set_verbosity (gint *v)
{
	if (*v < 0 || *v > 2)
	{
		fprintf (stderr, _("Illegal verbosity value.  Must be between 0 and 2 inclusive.\n"));
		exit (1);
	}
	
	verbosity = *v;
}

static void
pad_show_p_to_i (gint *n)
{
	pad_show_by_num (*n);
}

static void
list_pads (void)
{
	const pad_node *temp;
	for (temp = first_pad; temp; temp = temp->next)
		fprintf (output, "%s\n", temp->title);
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
	{TRUE, "--nonew", FALSE, {set_nonew}},
	{TRUE, "-n", FALSE, {set_new}},
	{TRUE, "--new", FALSE, {set_new}},
	
	{FALSE, "--nonew", FALSE, {set_nonew}},	/* registered here a second time because it has effects both on local instances and remote instances */
	{FALSE, "-n", FALSE, {G_CALLBACK (pad_new)}},
	{FALSE, "--new", FALSE, {G_CALLBACK (pad_new)}},
	{FALSE, "-q", FALSE, {gtk_main_quit}},
	{FALSE, "--quit", FALSE, {gtk_main_quit}},
	{FALSE, "-l", FALSE, {list_pads}},
	{FALSE, "--list", FALSE, {list_pads}},
	{FALSE, "-s", TRUE, {G_CALLBACK (pad_show_p_to_i)}},
	{FALSE, "--show", TRUE, {G_CALLBACK (pad_show_p_to_i)}},
	{FALSE, "--showall", FALSE, {G_CALLBACK (pads_show_all)}}
};

#define NUM_ARGUMENTS (sizeof (arguments) / sizeof (argument))


static void missing_companion_arg(const char argname[])
{
	fprintf(stderr, _("Missing companion argument to %s\n"), argname);
	exit(1);
}


static gint handle_args (int *argc, char ***argv, gboolean local)
{
	gint i, j, recognized_at;
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
		
		/* Find matching argument; there can be only one.
		 */
		recognized_at = -1;
		for (j = NUM_ARGUMENTS-1; j >= 0; j--)
		{
			if (strncmp ((*argv)[i], arguments[j].name, arglen[j]) == 0)
			{
				recognized_at = j;
				
				if (arguments[j].local == local)
					break;
			}
		}
		
		if (recognized_at < 0)
		{
		  	/* Argument not found in list.  Either its "local" setting mismatched
			 * the one passed to us, or we got an invalid argument.  Check for the
			 * latter only if we're doing local; otherwise just ignore the argument.
			 */
			if (local)
			{
				fprintf (stderr, _("Didn't understand argument %s.\n"), (*argv)[i]);
				exit (1);
			}
			else
			{
				continue;
			}
		}
		else
		{
			j = recognized_at;
		}
		
		/* (from this point on, we know our argument was recognized) */
		
		longform = (strncmp (arguments[j].name, "--", 2) == 0);
		
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
			long int arg;
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
				fprintf(stderr, _("Invalid number: '%s'\n"), companion);
				
				if (local)
					exit(1);
			}
			else
			{
				arguments[j].callbacks.func_arg (&arg);
			}
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
/*	if (autosave_timeout_id > 0)
		gtk_timeout_remove (autosave_timeout_id);

	if (current_settings.sync_time)
		autosave_timeout_id = gtk_timeout_add (current_settings.sync_time * 1000, sync_pads, NULL);
	else
		autosave_timeout_id = -1;
*/
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
	gchar *p;
	
	for (i = 0; i < *argc; i++)
		size += strlen ((*argv)[i]) + 1;
	
	*dest = g_malloc (size);
	
	p = *dest;
	
	for (i = 0; i < *argc; i++)
	{
		strcpy (p, (*argv)[i]);
		p += strlen ((*argv)[i]);
		p[0] = ' ';
		p += 1;
	}
	
	p --;
	p[0] = '\0';
	
	return size;
}

/*
returns number of strings in newly allocated argv
*/
static gint
string_to_args (const char *string, char ***argv)
{
	gint num, i;
	const gchar *tmp;
	char **list;
	
	/* first, find out how many arguments we have */
	num = 1;
	for (tmp = strchr (string, ' '); tmp; tmp = strchr (tmp+1, ' '))
	  num++;
	
	list = (char **) g_malloc (sizeof (char *) * (num + 1));
	
	for (i = 0; i < num; i++)
	{
		size_t len;

		/* string points to beginning of current arg */
		tmp = strchr (string, ' '); /* NULL or end of this arg */

		if (tmp) len = tmp - string;
		else   len = strlen (string);
		
		list[i] = g_malloc (len + 1);
		strncpy (list[i], string, len);
		list[i][len] = '\0';
		
		/* make string point to beginning of next arg */
		string = tmp + 1;
	}
	
	list[i] = NULL;	/* null terminate list */
	
	*argv = list;
	
	return num;
}

/* This reads a line from the proc file.  This line will contain a filename to get further data from. */
static void
read_from_proc_file (void)
{
	gint argc;
	gchar **argv;
	gint client_fd, size;
	gchar *args;
	struct sockaddr_un client;
	socklen_t client_len;
	size_t bytes;
	
	if (verbosity >= 1) printf ("Accepting client connection.\n");
	
	/* accept waiting connection */
	client_fd = accept (master_fd, (struct sockaddr *) &client, &client_len);
	if (client_fd == -1) return;
	
	/* get size of args */
	bytes = read (client_fd, &size, sizeof (size));
	if (bytes != sizeof(size))
	{
		if (verbosity >= 1)
		{
			if (bytes < 0) 
				fprintf(stderr, "Error on client connection\n");
			else if (bytes == 0)
				fprintf(stderr, "No data on client connection\n");
			else
				fprintf(stderr, "Expected %d bytes, got %d!\n",sizeof(size),bytes);
		}
		
		goto close_client_fd;
	}
	
	/* alloc memory */
	args = (gchar *) g_malloc (size);
	if (!args)
	{
		if (verbosity >= 2) fprintf(stderr, "Out of memory\n");
		goto close_client_fd;
	}
	
	/* read args */
	bytes = read (client_fd, args, size);
	if (bytes < size)
	{
		if (verbosity >= 1)
		{
			if (bytes < 0) fprintf(stderr, "Error on client connection\n");
			else fprintf(stderr, "Broken client connection\n");
		}
		goto close_client_fd;
	}
	
	argc = string_to_args (args, &argv);
	
	if (verbosity >= 2) 
	  fprintf (stderr, "Handling %i foreign args '%s'.\n", argc, args);
	
	g_free (args);
	
	/* here we redirect output to the socket */
	output = fdopen (client_fd, "a");
	
	if (!handle_args (&argc, &argv, FALSE))
	{
		/* if there were no non-local arguments, insert --new as argument */
		gint c = 2;
		gchar **v = g_malloc (sizeof (gchar *) * c);
		v[0] = "xpad";
		v[1] = "--new";
		
		handle_args (&c, &v, FALSE);
		
		g_free (v);
	}
	
	/* restore standard output */
	fclose (output);
	output = stdout;
	
	g_strfreev (argv);

close_client_fd:
	close (client_fd);
}

static gboolean
poll_master_fd (gpointer data)
{
	fd_set fdset;
	struct timeval tv = {0, 1000};	/* (almost) non-blocking mode */
	gint num;
	
	FD_ZERO (&fdset);
	FD_SET (master_fd, &fdset);
	num = select (master_fd + 1, &fdset, NULL, NULL, &tv);
	
	if ((num > 0) && FD_ISSET (master_fd, &fdset))
		read_from_proc_file ();
	
	return TRUE;
}

static gint
open_proc_file (void)
{
	struct sockaddr_un master;
	
	if (verbosity >= 2) printf ("Creating master socket '%s'.\n", master_name);
	
	unlink (master_name);
	
	/* create the socket */
	master_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
	master.sun_family = AF_LOCAL;
	strcpy (master.sun_path, master_name);
	if (bind (master_fd, (struct sockaddr *) &master, SUN_LEN (&master)))
	{
		if (verbosity >= 2) printf ("Failed to bind master socket.\n");
		return 1;
	}
	
	/* listen for connections */
	listen (master_fd, 5);
	
	gtk_idle_add (poll_master_fd, NULL);
	
	return 0;
}

static void
clipboard_get (GtkClipboard *clipboard, GtkSelectionData 
	*selection_data, guint info, gpointer data)
{
	static gboolean first_time = TRUE;
	
	if (first_time)
	{
		first_time = FALSE;
		
		open_proc_file ();
	}
	
	switch (info)
	{
	case 1:
		/* Fill the selection with the filename of our proc_file.  On a 1, which is a 
		   'are you alive?' ping, respond.  The other client will see this data and leave; we take
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
	/* No data needs to be freed, This shouldn't be called anyway -- we retain
	  control over clipboard throughout our life. */
}

static void
xpad_pass_args (int *argc, char ***argv)
{
	int client_fd;
	struct sockaddr_un master;
	fd_set fdset;
	gchar buf [129];
	gchar *args;
	gint size;
	gint bytesRead;
	
	/* create master socket */
	client_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
	master.sun_family = AF_LOCAL;
	strcpy (master.sun_path, master_name);
	
	if (verbosity >= 2) fprintf (stderr, "Connecting and sending to master socket '%s'.\n", master_name);
	
	/* connect to master socket */
	if (connect (client_fd, (struct sockaddr *) &master, SUN_LEN (&master)))
	{
		if (verbosity >= 2) fprintf (stderr, "Error on connect.\n");
		goto done;
	}
	
	size = args_to_string (argc, argv, &args) + 1;
	
	/* first, write length of string */
	write (client_fd, &size, sizeof (size));
	
	/* now, write string */
	write (client_fd, args, size);
	
	if (verbosity >= 2) fprintf (stderr, "Blocking on master socket.\n");
	
	do
	{
		/* wait for response */
		FD_ZERO (&fdset);
		FD_SET (client_fd, &fdset);	
		/* block until we are answered, or an error occurs */
		select (client_fd + 1, &fdset, NULL, &fdset, NULL);
		
		do
		{
			bytesRead = read (client_fd, buf, 128);
			
			if (bytesRead < 0)
			{
				if (verbosity >= 2)
					fprintf (stderr, "Error reading from master socket.\n");
			  goto done;
			}

			buf[bytesRead] = '\0';
			printf ("%s", buf);
		}
		while (bytesRead > 0);
	}
	while (bytesRead > 0);
	
done:
	close (client_fd);
	
	g_free (args);
	g_free (master_name);
	
	gtk_main_quit ();
}

static gint xpad_check_if_others (gpointer data)
{
	GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern ("_XPAD_EXISTS", FALSE));
	GtkSelectionData *temp;
	
	/* create master socket name */
	master_name = g_build_filename (working_dir, "server", NULL);
	
	if ((temp = gtk_clipboard_wait_for_contents (clipboard, 
		gdk_atom_intern ("STRING", FALSE))))
	{
		gpointer *newdata;
		
		newdata = (gpointer *) data;
		
		xpad_pass_args (newdata[0], newdata[1]);
		
		gtk_selection_data_free (temp);
		
		return 1;
	}
	else
	{
		/* set up target list with simple string target w/ value of 1 */
		GtkTargetEntry targets[] = {{"STRING", 0, 1}};
		
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
	
	factory = gtk_icon_factory_new ();
	
	pixbuf = gdk_pixbuf_new_from_xpm_data (lock_xpm);
	set = gtk_icon_set_new_from_pixbuf (pixbuf);
	gtk_icon_factory_add (GTK_ICON_FACTORY (factory),
		"xpad-lock", set);
	g_object_unref (pixbuf);
	
	pixbuf = gdk_pixbuf_new_from_xpm_data (sticky_xpm);
	set = gtk_icon_set_new_from_pixbuf (pixbuf);
	gtk_icon_factory_add (GTK_ICON_FACTORY (factory),
		"xpad-sticky", set);
	g_object_unref (pixbuf);
	
	gtk_icon_factory_add_default (GTK_ICON_FACTORY (factory));
	g_object_unref (factory);
}

static void xpad_set_default_icon (void)
{
	GdkPixbuf *pixbuf;
	GList *icons;
	
	pixbuf = gdk_pixbuf_new_from_xpm_data (xpad_xpm);
	icons = g_list_append (NULL, pixbuf);
	
	gtk_window_set_default_icon_list (icons);
	
	g_object_unref (pixbuf);
	g_list_free (icons);
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
	
	xpad_make_working_dir ();
	
	first_time = xpad_make_needed_files ();
	
	SetQuitSignals();
	
	if (xpad_check_if_others (data))
		return 0;
	
	xpad_settings_init ();
	
	/* save contents every "sync_time" seconds */
	reset_sync ();
	
#ifdef G_OS_UNIX
	tray_open ();
#endif
	
	xpad_set_default_icon ();
	xpad_register_icons ();
	
	/* load all pads */
	if (open_old_pads && fio_load_pads () == 0)
	{
		if (make_new_pad)
		{
			pad_new ();
		}
		else
		{
			gtk_main_quit ();
		}
	}
	
	handle_args (newdata[0], newdata[1], FALSE);
	
	/* when we get free time, save all the settings */
	g_idle_add (xpad_initial_save, NULL);
	
	if (first_time)
		show_help ();
	
	return 0;
}

int main (int argc, char *argv[])
{
	gpointer args[2];
	
#if HAVE_SETLOCALE
	setlocale (LC_ALL, "");
#endif
	
#if ENABLE_NLS
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif
	
	output = stdout;
	
	gtk_init(&argc, &argv);
	
	handle_args (&argc, &argv, TRUE);
	
	args[0] = &argc;
	args[1] = &argv;
	gtk_init_add (xpad_init, args);
	
	gtk_main ();
	
	return 0;
}
