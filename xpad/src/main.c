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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#include <gtk/gtk.h>

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

#include "../images/sticky.xpm"
#include "main.h"
#include "pad.h"
#include "help.h"
#include "fio.h"
#include "sm.h"
#include "tray.h"
#include "settings.h"
#include "defines.h"

gchar *working_dir;
gint verbosity = 0; /* output level */

gboolean make_new_pad = TRUE;
gboolean open_old_pads = TRUE;
gint master_fd = -1;
FILE *output;
gchar *master_name = NULL;
gchar *program_name = NULL;


static void print_help (void);

static gint at_gtk_exit (gpointer data)
{
	if (verbosity >= 1) g_print ("xpad is shutting down.\n");
	
	xpad_sm_shutdown ();	
	
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
	gchar *buf;
	
	dialog = gtk_dialog_new_with_buttons (
		"",
		parent,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
		NULL);
	
	hbox = gtk_hbox_new (FALSE, 12);
	image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);
	
	if (secondary)
		buf = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s", primary, secondary);
	else
		buf = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>", primary);
	
	gtk_label_set_markup (GTK_LABEL (label), buf);
	g_free (buf);
	
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
	
	if (!xpad_sm_start_interact (TRUE))
		return;
	
	g_printerr ("%s\n", primary);
	
	dialog = xpad_alert_new (parent, GTK_STOCK_DIALOG_ERROR,
		primary,
		secondary);
	
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, 1, NULL);
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	
	gtk_widget_destroy (dialog);
	
	xpad_sm_stop_interact (FALSE);
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
		g_printerr (_("Illegal \"--verbosity\" value.  Must be either 0, 1, or 2.\n"));
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
set_session (char *str)
{
	xpad_sm_set_id (str);
}

static void
list_pads (void)
{
	const pad_node *temp;
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		gchar *title, *ltitle;
		
		title = g_strconcat (temp->title, "\n", NULL);
		ltitle = g_locale_from_utf8 (title, -1, NULL, NULL, NULL);
		
		if (ltitle)
			fprintf (output, "%s", ltitle);
		
		g_free (title);
		g_free (ltitle);
	}
}


enum arg_type {
	ARG_TYPE_NONE,
	ARG_TYPE_INT,
	ARG_TYPE_STRING
};

struct argument_def
{
	gboolean local;
	const gchar *short_name;
	const gchar *long_name;
	enum arg_type second;
	union {
		void (*func) (void);
		void (*func_arg) (void *);
	} callbacks;
	const gchar *help_comment;
};
typedef struct argument_def argument;

static const argument arguments[] =
{
	{TRUE, "-h", "--help", ARG_TYPE_NONE, {print_help}, N_("Prints usage information and exits")},
	{TRUE, "-V", "--version", ARG_TYPE_NONE, {print_version}, N_("Prints xpad version and exits")},
	{TRUE, "-v", "--verbosity", ARG_TYPE_INT, {G_CALLBACK (set_verbosity)}, N_("Sets the amount of debugging ouput (can be 0, 1, or 2; default is 0)")},
	{TRUE, NULL, "--nonew", ARG_TYPE_NONE, {set_nonew}, N_("Prevents xpad from creating a new pad on startup if no previous pads exist")},
	{TRUE, "-n", "--new", ARG_TYPE_NONE, {set_new}, NULL},
	
	{FALSE, NULL, "--nonew", ARG_TYPE_NONE, {set_nonew}, NULL},	/* registered here a second time because it has effects both on local instances and remote instances */
	{FALSE, "-n", "--new", ARG_TYPE_NONE, {G_CALLBACK (pad_new)}, N_("Causes xpad to create a new pad on startup even if pads already exist")},
	{FALSE, "-q", "--quit", ARG_TYPE_NONE, {gtk_main_quit}, N_("Causes all running xpad instances to close")},
	{FALSE, "-l", "--list", ARG_TYPE_NONE, {list_pads}, N_("Lists the titles of all open pads")},
	{FALSE, "-s", "--show", ARG_TYPE_INT, {G_CALLBACK (pad_show_p_to_i)}, N_("Causes the Nth pad (1-based) to present itself to the user; pad numbers can be seen by using \"--list\"")},
	{FALSE, NULL, "--showall", ARG_TYPE_NONE, {G_CALLBACK (pads_show_all)}, N_("Causes all pads to present themselves to the user")},
	{FALSE, NULL, "--sm-client-id", ARG_TYPE_STRING, {G_CALLBACK (set_session)}, NULL}
};

#define NUM_ARGUMENTS (sizeof (arguments) / sizeof (argument))


static void
print_help (void)
{
	gchar *msg, *tmp_msg, *lmsg;
	gint  largest_size, i;

	for (largest_size = -1, i = 0; i < NUM_ARGUMENTS; i++)
	{
		if (arguments[i].help_comment)
		{
			gchar *addition = g_strconcat ("  * ", 
				arguments[i].short_name ? arguments[i].short_name : "",
				arguments[i].second == ARG_TYPE_INT ? " #" : "",
				(arguments[i].short_name && arguments[i].long_name) ? ", " : "",
				arguments[i].long_name ? arguments[i].long_name : "",
				arguments[i].second == ARG_TYPE_INT ? "=#" : "",
				NULL);
			if (largest_size == -1 || strlen (addition) > largest_size)
			{
				largest_size = strlen (addition);
			}
			g_free (addition);
		}
	}

	msg = g_strdup (_("Usage: xpad [OPTIONS]\n\n"));

	for (i = 0; i < NUM_ARGUMENTS; i++)
	{
		if (arguments[i].help_comment)
		{
			gchar *padding;
			gchar *addition = g_strconcat ("  * ", 
				arguments[i].short_name ? arguments[i].short_name : "",
				arguments[i].second == ARG_TYPE_INT ? " #" : "",
				(arguments[i].short_name && arguments[i].long_name) ? ", " : "",
				arguments[i].long_name ? arguments[i].long_name : "",
				arguments[i].second == ARG_TYPE_INT ? "=#" : "",
				NULL);

			padding = g_strnfill (largest_size - strlen (addition) + 3, ' ');

			tmp_msg = msg;
			msg = g_strconcat (msg, addition, padding, _(arguments[i].help_comment), "\n", NULL);
			g_free (tmp_msg);
		}
	}
	
	lmsg = g_locale_from_utf8 (msg, -1, NULL, NULL, NULL);
	
	if (lmsg)
		fprintf (output, lmsg);
	
	g_free (msg);
	g_free (lmsg);
	
	exit (0);
}

static void missing_companion_arg(const char argname[])
{
	g_printerr (_("Missing companion argument to %s\n"), argname);
	exit(1);
}


static gint handle_args (int *argc, char ***argv, gboolean local)
{
	gint i, j, recognized_at;
	gint rv = 0;
	size_t short_arglen[NUM_ARGUMENTS];
	size_t long_arglen[NUM_ARGUMENTS];
	
	/* Set up array of argument lengths to avoid having to compute them every 
	 * time through our inner loop.  This probably ought to be global, but it
	 * won't matter all that much.
	 */
	for (j = NUM_ARGUMENTS-1; j >= 0; j--)
	{
		short_arglen[j] = arguments[j].short_name ? strlen (arguments[j].short_name) : 0;
		long_arglen[j] = arguments[j].long_name ? strlen (arguments[j].long_name) : 0;
	}
	
	for (i = 1; i < *argc; i++)
	{
		gboolean longform;
		
		/* Find matching argument; there can be only one.
		 */
		recognized_at = -1;
		for (j = NUM_ARGUMENTS-1; j >= 0; j--)
		{
			if ((arguments[j].short_name && strncmp ((*argv)[i], arguments[j].short_name, short_arglen[j]) == 0) ||
			    (arguments[j].long_name && strncmp ((*argv)[i], arguments[j].long_name, long_arglen[j]) == 0))
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
				g_printerr (_("Didn't understand argument '%s'\n"), (*argv)[i]);
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
		
		longform = (strncmp ((*argv)[i], "--", 2) == 0);
		
		if (arguments[j].local != local)
		{
			/* We ignore this argument, but if it is in short form and expects a
			 * companion argument, make sure we skip the companion argument on our
			 * next iteration.
			 */
			if (arguments[j].second != ARG_TYPE_NONE)
				i++;
			
			/* Don't accept this argument, but don't complain either. */
			continue;
		}

		if (arguments[j].second != ARG_TYPE_NONE)
		{
			long int int_arg;
			char *companion, *endptr;
			
			if ((*argv)[i][longform ? long_arglen[j] : short_arglen[j]] == '=')
			{
				companion = &(*argv)[i][longform ? long_arglen[j] : short_arglen[j] + 1];
			}
			else
			{
				i++;
				if (i >= *argc)
				  	missing_companion_arg(longform ? arguments[j].long_name : arguments[j].short_name);
				
				companion = (*argv)[i];
			}
			
			if (!*companion) missing_companion_arg (longform ? arguments[j].long_name : arguments[j].short_name);
			
			if (arguments[j].second == ARG_TYPE_INT)
			{
				int_arg = strtol (companion, &endptr, 10);
				
				if (*endptr)
				{
					g_printerr (_("Invalid number: '%s'\n"), companion);
					
					if (local)
						exit(1);
				}
				else
				{
					arguments[j].callbacks.func_arg (&int_arg);
				}
			}
			else /* string */
			{
				arguments[j].callbacks.func_arg (companion);
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
	
	if (verbosity >= 1) g_print ("Accepting client connection.\n");
	
	/* accept waiting connection */
	client_fd = accept (master_fd, (struct sockaddr *) &client, &client_len);
	if (client_fd == -1) return ;
	
	/* get size of args */
	bytes = read (client_fd, &size, sizeof (size));
	if (bytes != sizeof(size))
	{
		if (verbosity >= 1)
		{
			if (bytes < 0) 
				g_printerr ("Error on client connection\n");
			else if (bytes == 0)
				g_printerr ("No data on client connection\n");
			else
				g_printerr ("Expected %d bytes, got %d!\n", sizeof(size),bytes);
		}
		
		goto close_client_fd;
	}
	
	/* alloc memory */
	args = (gchar *) g_malloc (size);
	if (!args)
	{
		if (verbosity >= 2) g_printerr ("Out of memory\n");
		goto close_client_fd;
	}
	
	/* read args */
	bytes = read (client_fd, args, size);
	if (bytes < size)
	{
		if (verbosity >= 1)
		{
			if (bytes < 0) g_printerr ("Error on client connection\n");
			else g_printerr ("Broken client connection\n");
		}
		goto close_client_fd;
	}
	
	argc = string_to_args (args, &argv);
	
	if (verbosity >= 2) 
		g_printerr ("Handling %i foreign args '%s'.\n", argc, args);
	
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
can_read_from_master_fd (GIOChannel *source, GIOCondition condition, gpointer data)
{
	read_from_proc_file ();
	
	return TRUE;
}

static gint
open_proc_file (void)
{
	GIOChannel *channel;
	struct sockaddr_un master;
	
	if (verbosity >= 2) g_print ("Creating master socket '%s'.\n", master_name);
	
	unlink (master_name);
	
	/* create the socket */
	master_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
	master.sun_family = AF_LOCAL;
	strcpy (master.sun_path, master_name);
	if (bind (master_fd, (struct sockaddr *) &master, SUN_LEN (&master)))
	{
		if (verbosity >= 2) g_print ("Failed to bind master socket.\n");
		return 1;
	}
	
	/* listen for connections */
	listen (master_fd, 5);
	
	/* set up input loop, waiting for read */
	channel = g_io_channel_unix_new (master_fd);
	g_io_add_watch (channel, G_IO_IN, can_read_from_master_fd, NULL);
	g_io_channel_unref (channel);
	
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
	gchar *args = NULL;
	gint size;
	gint bytesRead;
	
	/* create master socket */
	client_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
	master.sun_family = AF_LOCAL;
	strcpy (master.sun_path, master_name);
	
	if (verbosity >= 2) g_printerr ("Connecting and sending to master socket '%s'.\n", master_name);
	
	/* connect to master socket */
	if (connect (client_fd, (struct sockaddr *) &master, SUN_LEN (&master)))
	{
		if (verbosity >= 2) g_printerr ("Error on connect.\n");
		goto done;
	}
	
	size = args_to_string (argc, argv, &args) + 1;
	
	/* first, write length of string */
	write (client_fd, &size, sizeof (size));
	
	/* now, write string */
	write (client_fd, args, size);
	
	if (verbosity >= 2) g_print ("Blocking on master socket.\n");
	
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
					g_printerr ("Error reading from master socket.\n");
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
	
	pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
	                                   PACKAGE,
	                                   48,
	                                   0,
	                                   NULL);
	
	if (pixbuf)
	{
		GList *icons = g_list_append (NULL, pixbuf);
		
		gtk_window_set_default_icon_list (icons);
		
		g_object_unref (pixbuf);
		g_list_free (icons);
	}
}


#ifdef G_OS_UNIX

static RETSIGTYPE xpad_catch_quit_signal (int signum)
{
	if (verbosity >= 2) g_print ("xpad caught a signal.  Shutting down.\n");
	gtk_main_quit ();
}

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
	return FALSE;	/* remove ourselves from idle list */
}

/* data is an array of void pointers, indicating the argc and argv */
static gboolean xpad_init (gpointer data)
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
	
#ifdef G_OS_UNIX
	tray_open ();
#endif
	
	xpad_sm_init ();
	
	gdk_set_program_class (PACKAGE);
	
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
	
	return FALSE;
}

int main (int argc, char *argv[])
{
	gpointer args[2];
	
	program_name = argv[0];
	
#if HAVE_SETLOCALE
	setlocale (LC_ALL, "");
#endif
	
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	
	output = stdout;
	
	gtk_init (&argc, &argv);
	
	handle_args (&argc, &argv, TRUE);
	
	args[0] = &argc;
	args[1] = &argv;
	gtk_init_add (xpad_init, args);
	
	gtk_main ();
	
	return 0;
}
