/**
 * Copyright (c) 2004 Michael Terry
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* required by socket stuff */
/* define _GNU_SOURCE here because that makes our sockets work nice
 Unfortunately, we lose portability... */
#define _GNU_SOURCE	1
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <string.h>
#include <stdlib.h> /* for exit */

#include "../config.h"
#include <glib/gi18n.h>

#include "fio.h" /* for fio_get_info_from_file */
#include "xpad-app.h"
#include "xpad-pad-group.h"
#include "xpad-session-manager.h"
#include "xpad-tray.h"

#include "../images/sticky.xpm"

#if defined (G_OS_UNIX)
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


/* Seems that some systems (sun-sparc-solaris2.8 at least), need the following three #defines. 
   These were provided by Alan Mizrahi <alan@cesma.usb.ve>.
*/
#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

#ifndef SUN_LEN
#define SUN_LEN(sunp) ((size_t)((struct sockaddr_un *)0)->sun_path + strlen((sunp)->sun_path))
#endif


static gint xpad_argc;
static gchar **xpad_argv;
static gboolean make_new_pad_if_none;
static gchar *config_dir;
static gchar *program_path;
static gchar *server_filename;
static gint server_fd;
static FILE *output;
XpadPadGroup *pad_group;

static gint      process_args               (gint argc, gchar **argv, gboolean local_args);
static gint      xpad_app_check_if_others   (void);

static gboolean  config_dir_exists          (void);
static gchar    *make_config_dir            (void);
static void      register_stock_icons       (void);
static void      set_default_icon           (void);
static gint      xpad_app_load_pads         (void);


static void
xpad_app_init (int argc, char **argv)
{
	/* Set up i18n */
#ifdef ENABLE_NLS
	gtk_set_locale ();
	bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	gtk_init (&argc, &argv);
	xpad_argc = argc;
	xpad_argv = argv;
	output = stdout;
	
	g_set_application_name (_("Xpad"));
	gdk_set_program_class (PACKAGE);
	
	/* Set up program path. */
	if (xpad_argc > 0)
		program_path = g_find_program_in_path (xpad_argv[0]);
	else
		program_path = NULL;
	
	/* Set up config directory. */
	if (!config_dir_exists ())
	{
		show_help ();
	}
	config_dir = make_config_dir ();
	
	make_new_pad_if_none = TRUE;
	process_args (xpad_argc, xpad_argv, TRUE);
	
	if (xpad_app_check_if_others ())
		exit (0);
	
	xpad_tray_open ();
	xpad_session_manager_init ();
	
	register_stock_icons ();
	set_default_icon ();
	
	/* load all pads */
	pad_group = NULL;
	if (xpad_app_load_pads () == 0) {
		if (make_new_pad_if_none) {
			pad_node *pad = pad_new ();
			xpad_pad_group_add (pad_group, pad);
		}
		else if (!xpad_tray_is_open ())
			exit (0);
	}
	
	process_args (xpad_argc, xpad_argv, FALSE);
}


gint main (gint argc, gchar **argv)
{
	xpad_app_init (argc, argv);
	
	gtk_main ();
	
	return 0;
}




/* parent and secondary may be NULL.
 * Returns when user dismisses error.
 */
void
xpad_app_error (GtkWindow *parent, const gchar *primary, const gchar *secondary)
{
	GtkWidget *dialog;
	
	if (!xpad_session_manager_start_interact (TRUE))
		return;
	
	g_printerr ("%s\n", primary);
	
	dialog = xpad_app_alert_new (parent, GTK_STOCK_DIALOG_ERROR, primary, secondary);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, 1, NULL);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	xpad_session_manager_stop_interact (FALSE);
}



G_CONST_RETURN gchar *
xpad_app_get_config_dir (void)
{
	return config_dir;
}


/* Returns absolute path to our own executable. May be NULL. */
G_CONST_RETURN gchar *
xpad_app_get_program_path (void)
{
	return program_path;
}


XpadPadGroup *
xpad_app_get_pad_group (void)
{
	return pad_group;
}


static gboolean
config_dir_exists (void)
{
	gchar *dir = NULL;
	gboolean exists = FALSE;
	
#if defined (G_OS_UNIX)
	
	/* create a hidden directory under the user's home */
	dir = g_build_filename (g_get_home_dir (), ".xpad", NULL);
	
	exists = g_file_test (dir, G_FILE_TEST_EXISTS);
	
#elif defined (G_OS_WIN32)
	
	/* If someone has a better place to put our stuff, I'm all ears. */
	dir = g_build_filename (g_get_home_dir (), "xpad", NULL);
	
	exists = g_file_test (dir, G_FILE_TEST_EXISTS);
	
#elif defined (G_OS_BEOS)
	
	/* If someone has a better place to put our stuff, I'm all ears. */
	dir = g_build_filename (g_get_home_dir (), "xpad", NULL);
	
	exists = g_file_test (dir, G_FILE_TEST_EXISTS);
	
#endif
	
	g_free (dir);
	
	return exists;
}

/**
 * Creates the directory if it does not exist.
 * Returns newly allocated dir name, NULL if an error occurred.
 */
static gchar *
make_config_dir (void)
{
	gchar *dir = NULL;
	
#if defined (G_OS_UNIX)
	
	/* create a hidden directory under the user's home */
	dir = g_build_filename (g_get_home_dir (), ".xpad", NULL);
	
	/* make sure directory exists */
	mkdir (dir, 0700); /* give user all rights */
	
#elif defined (G_OS_WIN32)
	
	/* If someone has a better place to put our stuff, I'm all ears. */
	dir = g_build_filename (g_get_home_dir (), "xpad", NULL);
	
	/* make sure directory exists */
	CreateDirectory (dir, NULL); /* default security rights */
	
#elif defined (G_OS_BEOS)
	
	/* If someone has a better place to put our stuff, I'm all ears. */
	dir = g_build_filename (g_get_home_dir (), "xpad", NULL);
	
	/* make sure directory exists */
	CreateDirectory (dir, NULL);
	
#endif
	
	return dir;
}


/**
 * Creates an alert with 'stock' used to create an icon and parent text of 'parent',
 * secondary text of 'secondary'.  No buttons are added.
 */
GtkWidget *
xpad_app_alert_new (GtkWindow *parent, const gchar *stock,
                    const gchar *primary, const gchar *secondary)
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
		buf = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s\n</span>\n%s", primary, secondary);
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


static void
register_stock_icons (void)
{
	GtkIconSource *source;
	GtkIconSet *set;
	GtkIconFactory *factory;
	GdkPixbuf *pixbuf;
	
	factory = gtk_icon_factory_new ();
	
	pixbuf = gdk_pixbuf_new_from_xpm_data (sticky_xpm);
	set = gtk_icon_set_new_from_pixbuf (pixbuf);
	gtk_icon_factory_add (factory, "xpad-sticky", set);
	gtk_icon_set_unref (set);
	g_object_unref (pixbuf);
	
	source = gtk_icon_source_new ();
	gtk_icon_source_set_icon_name (source, "xpad");
	set = gtk_icon_set_new ();
	gtk_icon_set_add_source (set, source);
	gtk_icon_factory_add (factory, "xpad", set);
	gtk_icon_set_unref (set);
	gtk_icon_source_free (source);
	
	gtk_icon_factory_add_default (factory);
	g_object_unref (factory);
}


static void
set_default_icon (void)
{
	GList *icons = NULL;
	GtkIconTheme *theme = gtk_icon_theme_get_default ();
	
	icons = g_list_append (icons,
	                       gtk_icon_theme_load_icon (theme,
	                                                 PACKAGE,
	                                                 16,
	                                                 0,
	                                                 NULL));
	
	icons = g_list_append (icons,
	                       gtk_icon_theme_load_icon (theme,
	                                                 PACKAGE,
	                                                 24,
	                                                 0,
	                                                 NULL));
	
	icons = g_list_append (icons,
	                       gtk_icon_theme_load_icon (theme,
	                                                 PACKAGE,
	                                                 32,
	                                                 0,
	                                                 NULL));
	
	icons = g_list_append (icons,
	                       gtk_icon_theme_load_icon (theme,
	                                                 PACKAGE,
	                                                 48,
	                                                 0,
	                                                 NULL));
	
	if (icons) {
		gtk_window_set_default_icon_list (icons);
		
		g_list_foreach (icons, (GFunc) g_object_unref, NULL);
		g_list_free (icons);
	}
}



/* Scans config directory for pad files and loads them. */
static gint
xpad_app_load_pads (void)
{
	gint opened = 0;
	pad_info info;
	GDir *dir;
	G_CONST_RETURN gchar *name;
	
	/* First, empty our current pad list. */
	if (pad_group)
		g_object_unref (pad_group);
	pad_group = xpad_pad_group_new ();
	
	dir = g_dir_open (xpad_app_get_config_dir (), 0, NULL);
	
	if (!dir)
	{
		gchar *errtext;
		
		errtext = g_strdup_printf (_("Could not open directory %s."), xpad_app_get_config_dir ());
		
		xpad_app_error (NULL, errtext,
			_("This directory is needed to store preference and pad information.  Xpad will close now."));
		g_free (errtext);
		
		exit (1);
	}
	
	while ((name = g_dir_read_name (dir)))
	{
		/* if it's an info file, but not a backup info file... */
		if (!strncmp (name, "info-", 5) &&
		    name[strlen (name) - 1] != '~' &&
		    !fio_get_info_from_file (name, &info))
		{
			/**
			 * Fill pad from the info struct.  We don't need to free strings
			 * because the new pad takes over their care.
			 */
			pad_node *pad = pad_new_with_info (&info);
			
			xpad_pad_group_add (pad_group, pad);
			
			opened ++;
		}
	}
	
	g_dir_close (dir);
	
	return opened;
}












/*
converts main program arguments into one long string.
puts allocated string in dest, and returns size
*/
static gint
args_to_string (int argc, char **argv, char **dest)
{
	gint i;
	gint size = 0;
	gchar *p;
	
	for (i = 0; i < argc; i++)
		size += strlen (argv[i]) + 1;
	
	*dest = g_malloc (size);
	
	p = *dest;
	
	for (i = 0; i < argc; i++)
	{
		strcpy (p, argv[i]);
		p += strlen (argv[i]);
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

#include <errno.h>
static gint xpad_app_open_proc_file (void);
/* This reads a line from the proc file.  This line will contain arguments to process. */
static void
xpad_app_read_from_proc_file (void)
{
	gint client_fd, size;
	gint argc;
	gchar **argv;
	gchar *args;
	struct sockaddr_un client;
	socklen_t client_len;
	size_t bytes;
	
	/* accept waiting connection */
	client_len = sizeof (client);
	client_fd = accept (server_fd, (struct sockaddr *) &client, &client_len);
	if (client_fd == -1)
		return;
	
	/* get size of args */
	bytes = read (client_fd, &size, sizeof (size));
	if (bytes != sizeof(size))
		goto close_client_fd;
	
	/* alloc memory */
	args = (gchar *) g_malloc (size);
	if (!args)
		goto close_client_fd;
	
	/* read args */
	bytes = read (client_fd, args, size);
	if (bytes < size)
		goto close_client_fd;
	
	argc = string_to_args (args, &argv);
	
	g_free (args);
	
	/* here we redirect singleton->priv->output to the socket */
	output = fdopen (client_fd, "w");
	
	if (!process_args (argc, argv, FALSE))
	{
		/* if there were no non-local arguments, insert --new as argument */
		gint c = 2;
		gchar **v = g_malloc (sizeof (gchar *) * c);
		v[0] = PACKAGE;
		v[1] = "--new";
		
		process_args (c, v, FALSE);
		
		g_free (v);
	}
	
	/* restore standard singleton->priv->output */
	fclose (output);
	output = stdout;
	
	g_strfreev (argv);
	return;
	
close_client_fd:
	close (client_fd);
}


static gboolean
can_read_from_server_fd (GIOChannel *source, GIOCondition condition, gpointer data)
{
	xpad_app_read_from_proc_file ();
	
	return TRUE;
}

static gint
xpad_app_open_proc_file (void)
{
	GIOChannel *channel;
	struct sockaddr_un master;
	
	unlink (server_filename);
	
	/* create the socket */
	server_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
	bzero (&master, sizeof (master)); 
	master.sun_family = AF_LOCAL;
	strcpy (master.sun_path, server_filename);
	if (bind (server_fd, (struct sockaddr *) &master, SUN_LEN (&master)))
	{
		return 1;
	}
	
	/* listen for connections */
	listen (server_fd, 5);
	
	/* set up input loop, waiting for read */
	channel = g_io_channel_unix_new (server_fd);
	g_io_add_watch (channel, G_IO_IN, can_read_from_server_fd, NULL);
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
		
		xpad_app_open_proc_file ();
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
	/* No data needs to be freed.  This shouldn't be called anyway -- we retain
	  control over clipboard throughout our life. */
}


static void
xpad_app_pass_args (void)
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
	strcpy (master.sun_path, server_filename);
	
	/* connect to master socket */
	if (connect (client_fd, (struct sockaddr *) &master, SUN_LEN (&master)))
		goto done;
	
	size = args_to_string (xpad_argc, xpad_argv, &args) + 1;
	
	/* first, write length of string */
	write (client_fd, &size, sizeof (size));
	
	/* now, write string */
	write (client_fd, args, size);
	
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
	g_free (server_filename);
}


static gint
xpad_app_check_if_others (void)
{
	GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern ("_XPAD_EXISTS", FALSE));
	GtkSelectionData *temp;
	
	/* create master socket name */
	server_filename = g_build_filename (xpad_app_get_config_dir (), "server", NULL);
	
	if ((temp = gtk_clipboard_wait_for_contents (clipboard, 
		gdk_atom_intern ("STRING", FALSE))))
	{
		xpad_app_pass_args ();
		
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
 * Here are the functions called when arguments are passed to us.
 */


enum {
	XPAD_ARG_TYPE_NONE,
	XPAD_ARG_TYPE_INT,
	XPAD_ARG_TYPE_STRING
};

struct argument
{
	gboolean local;
	const gchar *short_name;
	const gchar *long_name;
	guint type;
	union {
		void (*func) (void);
		void (*func_arg) (void *);
	} callbacks;
	const gchar *help_comment;
};

static void missing_companion_arg (const char argname[]);
static void xpad_app_args_print_help (void);
static void xpad_app_args_print_version (void);
static void xpad_app_args_set_nonew (void);
static void xpad_app_args_set_session (const gchar *str);
static void xpad_app_args_spawn_pad (void);

static const struct argument arguments[] =
{
	{TRUE, "-h", "--help", XPAD_ARG_TYPE_NONE, {G_CALLBACK (xpad_app_args_print_help)}, N_("Prints usage information and exits")},
	{TRUE, "-v", "--version", XPAD_ARG_TYPE_NONE, {G_CALLBACK (xpad_app_args_print_version)}, N_("Prints xpad version and exits")},
	{TRUE, NULL, "--nonew", XPAD_ARG_TYPE_NONE, {G_CALLBACK (xpad_app_args_set_nonew)}, N_("Prevents xpad from creating a new pad on startup if no previous pads exist")},
	
	/* above are local arguments, the ones below are run by the remote instance */
	
	/* --nonew is here twice because it does things for both local and remote instances.  in remote, it stops the remote instance from thinking there were no args and spawning a new pad */
	{FALSE, NULL, "--nonew", XPAD_ARG_TYPE_NONE, {G_CALLBACK (xpad_app_args_set_nonew)}, NULL},
	{FALSE, "-n", "--new", XPAD_ARG_TYPE_NONE, {G_CALLBACK (xpad_app_args_spawn_pad)}, N_("Causes xpad to create a new pad on startup even if pads already exist")},
	{FALSE, "-q", "--quit", XPAD_ARG_TYPE_NONE, {G_CALLBACK (gtk_main_quit)}, N_("Causes all running xpad instances to close")},
	{FALSE, NULL, "--sm-client-id", XPAD_ARG_TYPE_STRING, {G_CALLBACK (xpad_app_args_set_session)}, NULL}
};


static void
xpad_app_args_spawn_pad (void)
{
	pad_node *pad = pad_new ();
	xpad_pad_group_add (pad_group, pad);
}


static void
xpad_app_args_print_version (void)
{
	fprintf (output, _("Xpad %s\n"), PACKAGE_VERSION);
	exit (0);
}


static void
xpad_app_args_set_nonew (void)
{
	make_new_pad_if_none = FALSE;
}


static void
xpad_app_args_set_session (const gchar *str)
{
	xpad_session_manager_set_id (str);
}


static void
xpad_app_args_print_help (void)
{
	gchar *msg, *tmp_msg, *lmsg;
	gint  largest_size, i;

	for (largest_size = -1, i = 0; i < G_N_ELEMENTS (arguments); i++)
	{
		if (arguments[i].help_comment)
		{
			gchar *addition = g_strconcat ("  * ", 
				arguments[i].short_name ? arguments[i].short_name : "",
				arguments[i].type == XPAD_ARG_TYPE_INT ? " #" : "",
				(arguments[i].short_name && arguments[i].long_name) ? ", " : "",
				arguments[i].long_name ? arguments[i].long_name : "",
				arguments[i].type == XPAD_ARG_TYPE_INT ? "=#" : "",
				NULL);
			if (largest_size == -1 || strlen (addition) > largest_size)
			{
				largest_size = strlen (addition);
			}
			g_free (addition);
		}
	}

	msg = g_strdup_printf (_("Usage: %s [OPTIONS]\n\n"), g_get_prgname ());

	for (i = 0; i < G_N_ELEMENTS (arguments); i++)
	{
		if (arguments[i].help_comment)
		{
			gchar *padding;
			gchar *addition = g_strconcat ("  * ", 
				arguments[i].short_name ? arguments[i].short_name : "",
				arguments[i].type == XPAD_ARG_TYPE_INT ? " #" : "",
				(arguments[i].short_name && arguments[i].long_name) ? ", " : "",
				arguments[i].long_name ? arguments[i].long_name : "",
				arguments[i].type == XPAD_ARG_TYPE_INT ? "=#" : "",
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


static void
missing_companion_arg (const char argname[])
{
	g_printerr (_("Missing companion argument to %s\n"), argname);
	exit(1);
}


static gint
process_args (gint argc, gchar **argv, gboolean local)
{
	gint i, j, recognized_at;
	gint rv = 0;
	size_t short_arglen[G_N_ELEMENTS (arguments)];
	size_t long_arglen[G_N_ELEMENTS (arguments)];
	
	/* Set up array of argument lengths to avoid having to compute them every 
	 * time through our inner loop.  This probably ought to be global, but it
	 * won't matter all that much.
	 */
	for (j = G_N_ELEMENTS (arguments) - 1; j >= 0; j--) {
		short_arglen[j] = arguments[j].short_name ? strlen (arguments[j].short_name) : 0;
		long_arglen[j] = arguments[j].long_name ? strlen (arguments[j].long_name) : 0;
	}
	
	for (i = 1; i < argc; i++) {
		gboolean longform;
		
		/* Find matching argument; there can be only one.
		 */
		recognized_at = -1;
		for (j = G_N_ELEMENTS (arguments) - 1; j >= 0; j--) {
			if ((arguments[j].short_name && strncmp (argv[i], arguments[j].short_name, short_arglen[j]) == 0) ||
			    (arguments[j].long_name && strncmp (argv[i], arguments[j].long_name, long_arglen[j]) == 0)) {
				recognized_at = j;
				
				if (arguments[j].local == local)
					break;
			}
		}
		
		if (recognized_at < 0) {
		  	/* Argument not found in list.  Either its "local" setting mismatched
			 * the one passed to us, or we got an invalid argument.  Check for the
			 * latter only if we're doing local; otherwise just ignore the argument.
			 */
			if (local) {
				g_printerr (_("Didn't understand argument '%s'\n"), argv[i]);
				exit (1);
			}
			else {
				continue;
			}
		}
		else {
			j = recognized_at;
		}
		
		/* (from this point on, we know our argument was recognized) */
		
		longform = (strncmp (argv[i], "--", 2) == 0);
		
		if (arguments[j].local != local) {
			/* We ignore this argument, but if it is in short form and expects a
			 * companion argument, make sure we skip the companion argument on our
			 * next iteration.
			 */
			if (arguments[j].type != XPAD_ARG_TYPE_NONE)
				i++;
			
			/* Don't accept this argument, but don't complain either. */
			continue;
		}

		if (arguments[j].type != XPAD_ARG_TYPE_NONE) {
			long int int_arg;
			char *companion, *endptr;
			
			if (argv[i][longform ? long_arglen[j] : short_arglen[j]] == '=') {
				companion = &argv[i][longform ? long_arglen[j] : short_arglen[j] + 1];
			}
			else {
				i++;
				if (i >= argc)
				  	missing_companion_arg (longform ? arguments[j].long_name : arguments[j].short_name);
				
				companion = argv[i];
			}
			
			if (!*companion)
				missing_companion_arg (longform ? arguments[j].long_name : arguments[j].short_name);
			
			if (arguments[j].type == XPAD_ARG_TYPE_INT) {
				int_arg = strtol (companion, &endptr, 10);
				
				if (*endptr) {
					g_printerr (_("Invalid number: '%s'\n"), companion);
					
					if (local)
						exit(1);
				}
				else {
					arguments[j].callbacks.func_arg (&int_arg);
				}
			}
			else { /* string */
				arguments[j].callbacks.func_arg (companion);
			}
		}
		else {
			arguments[j].callbacks.func ();
		}
		rv++;
	}
	
	return rv;
}
