#include "help.h"

void show_help (void)
{
	show_help_at_page (0);
}

void show_help_at_page (gint page)
{
	GtkWidget *dialog, *helptext, *helplabel, *button, *notebook, *keytext, *keylabel;
	GtkWidget *edittext, *editlabel, *scrollwin;
	GtkObject *hadjust, *vadjust;
	
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
"xpad is a GTK+ 2.0 application that opens small textboxes "
"on your desktop on which you write notes or messages.\n"
"xpad was designed with ease of use in mind, but if you "
"have troubles, here's how to do most things you would "
"want to:\n\n\n"

"<b>moving</b>: To move a pad, hold down CTRL and drag "
"with the left mouse button.\n\n"

"<b>resizing</b>: To resize a pad, hold down CTRL and "
"drag with the right mouse button.\n\n"

"<b>making new pads</b>: To open a new pad, right click on "
"an existing pad.  Select \"New Pad\" from the menu.\n\n"

"<b>colors</b>: If black on yellow isn't your thing, change "
"the default color by right clicking on a pad "
"and selecting \"Preferences\" from the menu.  "
"An options menu will now pop up and you can change the "
"text color, background color, and border color.\n\n"

"<b>closing pads</b>: To close a pad and <i>keep</i> its "
"contents, choose \"Close\" from the right-click menu.  "
"Again, \"Destroy\" is only if you are sure you don't want "
"the pad contents -- they will be erased.\n\n"

"<b>opening files</b>: xpad allows you to open an arbitrary "
"file into a pad.  Note that this pad contains only a copy "
"of the file; destroying the pad does nothing to the original "
"file.\n");

	gtk_label_set_line_wrap (GTK_LABEL (helptext), TRUE);
	hadjust = gtk_adjustment_new (0, 0, 0, 0, 0, 0);
	vadjust = gtk_adjustment_new (0, 10, 0, 1, 5, 5);
	scrollwin = gtk_scrolled_window_new (GTK_ADJUSTMENT (hadjust), GTK_ADJUSTMENT (vadjust));
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollwin), helptext);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin, helplabel);
	
	
	gtk_label_set_markup (GTK_LABEL (keytext),
"<b>CTRL+n</b>: Creates a new pad.\n\n"
"<b>CTRL+s</b>: Saves the contents of a pad to a file.\n\n"
"<b>CTRL+o</b>: Copies the contents of a file into a pad.\n\n"
"<b>CTRL+p</b>: Opens the preferences window.\n\n"
"<b>CTRL+SHIFT+c</b>: Closes the currently selected pad.\n\n"
"<b>CTRL+SHIFT+a</b>: Closes all open pads.\n\n"
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
	
	gtk_window_set_title (GTK_WINDOW (dialog), "xpad help");
	
	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
	gtk_dialog_add_button (GTK_DIALOG(dialog), "gtk-close", 1);
	
	gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal (GTK_WINDOW(dialog), TRUE);
	
	gtk_widget_show_all (dialog);
	
	gtk_dialog_run (GTK_DIALOG(dialog));
	
	gtk_widget_destroy (dialog);
}
