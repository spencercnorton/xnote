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

#include "help.h"
#include <string.h>

GtkWidget *help_window = NULL;

static void help_close (void)
{
	help_window = NULL;
}


static void show_help_at_page (gint page);

static GtkWidget *create_help (gint page)
{
	GtkWidget *dialog, *helptext, *helplabel, *button, *notebook;
	GtkWidget *styletext, *stylelabel;
	gchar helptextbuf[700];
	
	/* Create the widgets */
	
	dialog = gtk_dialog_new ();
	helptext = gtk_label_new ("");
	helplabel = gtk_label_new (_("Introduction"));
	styletext = gtk_label_new ("");
	stylelabel = gtk_label_new (_("Style Locking"));
	notebook = gtk_notebook_new ();
	
	strcpy (helptextbuf, 
_("Each xpad session consists of one or more open pads.  "
"These pads are basically textboxes on your desktop in which "
"you can write memos.\n\n")
	strcat (helptextbuf, 
_("The contents of pads are transparently "
"saved and reloaded when xpad is next started.\n\n"));
	strcat (helptextbuf,
_("To move a pad, left drag on the toolbar, right drag "
"on the resizer in the bottom right, or hold down CTRL "
"while left dragging anywhere on the pad.\n\n"));
	strcat (helptextbuf,
_("To resize a pad, left drag on the resizer or hold down "
"CTRL while right dragging anywhere on the pad.\n\n"));
	strcat (helptextbuf, 
_("Most actions are available throught the popup menu "
"that appears when you right click on a pad.  Try it out and "
"enjoy.\n\n")
	strcat (helptextbuf,
_("Please send comments or bug reports to "
"xpad-devel@lists.sourceforge.net"));

	gtk_label_set_markup (GTK_LABEL (helptext), helptextbuf);

	gtk_misc_set_padding (GTK_MISC (helptext), 12, 12);
	gtk_misc_set_alignment (GTK_MISC (helptext), 0, 0);
	gtk_label_set_line_wrap (GTK_LABEL (helptext), TRUE);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), helptext, helplabel);
	
	
	gtk_label_set_markup (GTK_LABEL (styletext),
_("If a pad's style is locked, any changes made in the preference "
"window to the text, background, or border color or the font face "
"do not affect that pad.\n\n"
"To lock a pad's style, right click on a "
"pad to open the popup menu and select \"Lock Style\".  "
"By default, pads are not locked."));

	gtk_misc_set_padding (GTK_MISC (styletext), 12, 12);
	gtk_misc_set_alignment (GTK_MISC (styletext), 0, 0);
	gtk_label_set_line_wrap (GTK_LABEL (styletext), TRUE);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), styletext, stylelabel);
	
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page);
	
	gtk_window_set_title (GTK_WINDOW (dialog), _("Xpad Help"));
	
	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
	button = gtk_dialog_add_button (GTK_DIALOG(dialog), "gtk-close", 1);
	
	gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	
	g_signal_connect (GTK_OBJECT (dialog), "destroy", 
		G_CALLBACK (help_close), NULL);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked", 
		G_CALLBACK (gtk_widget_destroy), dialog);
	
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_widget_show_all (dialog);
	
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
		gtk_window_present (GTK_WINDOW (help_window));
}
