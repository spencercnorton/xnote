/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2013-2024 Arthur Borsboom

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "../config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "help.h"
#include "xpad-app.h"

GtkWindow *help_window = NULL;

static void help_close ()
{
	help_window = NULL;
}

void show_help ()
{
	if (help_window == NULL) {

		GtkWidget *helptext, *scrolled_window, *button;
		GtkBox *vbox;
		gchar *helptextbuf = NULL;
		gboolean success;
		GError *error = NULL;

		/* Load help text from file */
		gchar *help_file = g_strdup_printf ("%s/%s/%s", DATADIR, HELPDIR, HELPFILENAME);
		success = g_file_get_contents (help_file, &helptextbuf, NULL, &error);

		if (!success) {
			/* Was: help_file freed above, then read here via %s — a use-after-free
			   read in the very error path that reports the failure. */
			xpad_app_error (NULL, _("Error showing the help"), g_strdup_printf (_("Could not find the help file %s\n%s"), help_file, error->message));
			g_free (help_file);
			g_clear_error (&error);
			return;
		}
		g_free (help_file);

		/* Set layout of help text */
		helptext = gtk_label_new ("");
		gtk_label_set_markup (GTK_LABEL (helptext), helptextbuf);
		g_free (helptextbuf);
		gtk_widget_set_margin_top (helptext, 12);
		gtk_widget_set_margin_bottom (helptext, 12);
		gtk_widget_set_margin_start (helptext, 12);
		gtk_widget_set_margin_end (helptext, 12);
		gtk_label_set_wrap (GTK_LABEL (helptext), TRUE);

		/* Create a box and stuff the text and buttons in. GTK 4 boxes have no
		   pack_start(expand,fill,padding); use append() plus expand hints. */
		vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 18));
		gtk_box_set_homogeneous (vbox, FALSE);
		gtk_widget_set_vexpand (helptext, TRUE);
		gtk_box_append (vbox, helptext);
		button = gtk_button_new_with_label (_("Close"));
		gtk_box_append (vbox, button);

		/* Initialize help window */
		help_window = GTK_WINDOW (gtk_window_new ());
		gtk_window_set_title (help_window, _("Help"));
		/* GTK 4 removed gtk_window_set_position()/gtk_window_resize(); the
		   compositor places the window and we set a default size. */
		gtk_window_set_default_size (help_window, 800, 1000);

		/* Add scrollbars */
		scrolled_window = gtk_scrolled_window_new ();
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (vbox));
		gtk_window_set_child (help_window, scrolled_window);

		g_signal_connect (help_window, "destroy", G_CALLBACK (help_close), NULL);
		g_signal_connect_swapped (GTK_BUTTON (button), "clicked", G_CALLBACK (gtk_window_destroy), help_window);

		gtk_label_set_selectable (GTK_LABEL (helptext), TRUE);
		gtk_window_present (help_window);
	}
	else
		gtk_window_present (help_window);
}
