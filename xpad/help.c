#include "help.h"

GtkWidget *help_window = NULL;

static void help_close (void)
{
	help_window = NULL;
}


static void show_help_at_page (gint page);

static GtkWidget *create_help (gint page)
{
	GtkWidget *dialog, *helptext, *helplabel, *button, *notebook, *keytext, *keylabel;
	GtkWidget *edittext, *editlabel;
	GtkWidget *align1, *align2, *align3;
	
	/* Create the widgets */
	
	dialog = gtk_dialog_new ();
	helptext = gtk_label_new ("");
	helplabel = gtk_label_new ("Introduction");
	keytext = gtk_label_new ("");
	keylabel = gtk_label_new ("Keyboard Shortcuts");
	edittext = gtk_label_new ("");
	editlabel = gtk_label_new ("Edit Lock");
	notebook = gtk_notebook_new ();
	align1 = gtk_alignment_new (0, 0, 0, 0);
	align2 = gtk_alignment_new (0, 0, 0, 0);
	align3 = gtk_alignment_new (0, 0, 0, 0);
	
	gtk_label_set_markup (GTK_LABEL (helptext), 
"Each xpad session consists of one or more open pads.  "
"These pads are basically textboxes on your desktop in which "
"you can write memos.\n\n"

"There are two important non-obvious operations that you "
"should be aware of:\n\n"

"<b>Moving</b>: To move a pad, hold down CTRL and drag "
"with the left mouse button.\n\n"

"<b>Resizing</b>: To resize a pad, hold down CTRL and "
"drag with the right mouse button.\n\n"
);

	gtk_label_set_line_wrap (GTK_LABEL (helptext), TRUE);
	gtk_container_add (GTK_CONTAINER (align1), helptext);
	gtk_container_set_border_width (GTK_CONTAINER (align1), 6);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), align1, helplabel);
	
	
	gtk_label_set_markup (GTK_LABEL (keytext),
"<b>CTRL+n</b>: Creates a new pad.\n\n"
"<b>CTRL+s</b>: Saves the contents of a pad to a file.\n\n"
"<b>CTRL+o</b>: Copies the contents of a file into a pad.\n\n"
"<b>CTRL+p</b>: Opens the preferences window.\n\n"
"<b>CTRL+q</b>: Quits xpad.\n\n"
"<b>CTRL+SHIFT+d</b>: Deletes the currently selected pad.\n");
	
	gtk_label_set_line_wrap (GTK_LABEL (keytext), TRUE);
	gtk_container_add (GTK_CONTAINER (align2), keytext);
	gtk_container_set_border_width (GTK_CONTAINER (align2), 6);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), align2, keylabel);
	
	
	gtk_label_set_markup (GTK_LABEL (edittext),
"If Edit Lock is enabled, a pad is always in one of two "
"modes:  Edit Mode or Move Mode.\n\n"
"<b>Edit Mode</b>: You can edit the text of the pad, select "
"text, cut and paste, etc.  To move the pad, hold down CTRL "
"while left-dragging.\n\n"
"<b>Move Mode</b>: In this mode, clicking and dragging on the "
"pad will move the pad, rather than selecting text.  You cannot "
"edit the contents of the pad.\n\n"
"Any pad without focus is in Move Mode.  To enter Edit "
"Mode for any pad, double click on it with the left mouse "
"button.  Now this pad will be editable until it loses focus.\n\n"
"If Edit Lock is disabled, all pads are in Edit Mode.\n\n"
"To enable Edit Lock, go to Preferences->Options.");

	gtk_label_set_line_wrap (GTK_LABEL (edittext), TRUE);
	gtk_container_add (GTK_CONTAINER (align3), edittext);
	gtk_container_set_border_width (GTK_CONTAINER (align3), 6);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), align3, editlabel);
	
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page);
	
	gtk_window_set_title (GTK_WINDOW (dialog), "xpad Help");
	
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
