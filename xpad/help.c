#include "help.h"

GtkWidget *help_window = NULL;

void help_close ()
{
	help_window = NULL;
}

GtkWidget *create_help (gint page)
{
	GtkWidget *dialog, *helptext, *helplabel, *button, *notebook, *keytext, *keylabel;
	GtkWidget *edittext, *editlabel;
	
	/* Create the widgets */
	
	dialog = gtk_dialog_new ();
	helptext = gtk_label_new ("");
	helplabel = gtk_label_new ("Introduction");
	keytext = gtk_label_new ("");
	keylabel = gtk_label_new ("Keyboard Shortcuts");
	edittext = gtk_label_new ("");
	editlabel = gtk_label_new ("Edit Lock");
	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	notebook = gtk_notebook_new ();
	
	gtk_label_set_markup (GTK_LABEL (helptext), 
"\nxpad was designed with ease of use in mind, but if you "
"have troubles, here's how to do most things you would "
"want to:\n\n"

"<b>moving</b>: To move a pad, hold down CTRL and drag "
"with the left mouse button.\n\n"

"<b>resizing</b>: To resize a pad, hold down CTRL and "
"drag with the right mouse button.\n\n"
);

	gtk_label_set_line_wrap (GTK_LABEL (helptext), TRUE);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), helptext, helplabel);
	
	
	gtk_label_set_markup (GTK_LABEL (keytext),
"<b>CTRL+n</b>: Creates a new pad.\n\n"
"<b>CTRL+s</b>: Saves the contents of a pad to a file.\n\n"
"<b>CTRL+o</b>: Copies the contents of a file into a pad.\n\n"
"<b>CTRL+p</b>: Opens the preferences window.\n\n"
"<b>CTRL+SHIFT+c</b>: Closes all open pads.\n\n"
"<b>CTRL+SHIFT+d</b>: Destroys the currently selected pad.\n");
	
	gtk_label_set_line_wrap (GTK_LABEL (keytext), TRUE);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), keytext, keylabel);
	
	
	gtk_label_set_markup (GTK_LABEL (edittext),
"If Edit Lock is enabled, a pad is always in one of two "
"modes:  Edit Mode or Move Mode.\n\n"
"<b>Edit Mode</b>: You can edit the text of the pad, select "
"text, cut and paste, etc.\n\n"
"<b>Move Mode</b>: In this mode, clicking and dragging on the "
"pad will move the pad, rather than selecting text.  You cannot "
"edit the contents of the pad.\n\n"
"Any pad without focus is in Move Mode.  To enter Edit "
"Mode for any pad, double click on it with the left mouse "
"button.  Now this pad will be editable until it loses focus.\n\n"
"If Edit Lock is disabled, all pads are always in Edit Mode.\n");

	gtk_label_set_line_wrap (GTK_LABEL (edittext), TRUE);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), edittext, editlabel);
	
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page);
	
	gtk_window_set_title (GTK_WINDOW (dialog), "xpad Help");
	
	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
	gtk_dialog_add_button (GTK_DIALOG(dialog), "gtk-close", 1);
	
	gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	
	gtk_widget_show_all (dialog);
	
	g_signal_connect (GTK_OBJECT (dialog), "destroy", 
		G_CALLBACK (help_close), NULL);
		
	return dialog;
}

void show_help (void)
{
	show_help_at_page (0);
}

void show_help_at_page (gint page)
{
	if (help_window == NULL)
		help_window = create_help (page);
	else
		gtk_window_present (GTK_WINDOW (help_window));
}
