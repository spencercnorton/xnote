/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2013-2014 Arthur Borsboom

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
#include "help.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

GtkWindow *help_window = NULL;

static void help_close (void)
{
	help_window = NULL;
}

static void show_help_at_page (gint page);

static GtkWindow *create_help (gint page)
{
	GtkWindow *dialog;
	GtkWidget *helptext, *button;
	gchar *helptextbuf;
	
	/* Create the widgets */
	dialog = GTK_WINDOW (gtk_dialog_new ());
	helptext = gtk_label_new ("");
	
	if (page == 0) {
		/* we use g_strdup_printf because C89 has size limits on static strings */
		helptextbuf = g_strdup_printf ("%s\n\n%s\n\n%s\n\n%s\n\n%s\n\n%s",
		_("Each xpad session consists of one or more open pads.  "
		"These pads are basically sticky notes on your desktop in which "
		"you can write memos."),
		_("<b>To move a pad</b>, left drag on the toolbar, right drag "
		"on the resizer in the bottom right, or hold down CTRL "
		"while left dragging anywhere on the pad."),
		_("<b>To resize a pad</b>, left drag on the resizer or hold down "
		"CTRL while right dragging anywhere on the pad."),
		_("<b>To change color settings</b>, right click on a pad "
		"and choose Edit->Preferences."),
		_("Most actions are available throught the popup menu "
		"that appears when you right click on a pad.  Try it out and "
		"enjoy."),
		_("Please send ideas or bug reports to\n"
		"https://bugs.launchpad.net/xpad/+filebug"));
	}
	else
		helptextbuf = g_strdup_printf("Unknown help page requested");
	
	gtk_label_set_markup (GTK_LABEL (helptext), helptextbuf);
	
	g_free (helptextbuf);
	
	gtk_misc_set_padding (GTK_MISC (helptext), 12, 12);
	gtk_misc_set_alignment (GTK_MISC (helptext), 0, 0);
	gtk_label_set_line_wrap (GTK_LABEL (helptext), TRUE);
	
	gtk_window_set_title (dialog, _("Help"));
	
	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), helptext);
	button = gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-close", 1);
	
	gtk_window_set_position (dialog, GTK_WIN_POS_CENTER);
	
	g_signal_connect (dialog, "destroy", G_CALLBACK (help_close), NULL);
	g_signal_connect_swapped (GTK_BUTTON (button), "clicked", G_CALLBACK (gtk_widget_destroy), dialog);
	
	gtk_window_set_resizable (dialog, TRUE);
	gtk_widget_show_all (GTK_WIDGET (dialog));
	
	return dialog;
}

void show_help (void)
{
	show_help_at_page (0);
}

static void show_help_at_page (gint page)
{
	if (help_window == NULL)
		help_window = create_help (page);
	else
		gtk_window_present (help_window);
}
