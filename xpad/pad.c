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

#include "pad.h"
#include "pref.h"
#include "fio.h"
#include "help.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

pad_node *first_pad = NULL;
pad_node *last_pad = NULL;

/* helper func to get textbox from window */
GtkTextView *get_text (GtkWindow *window)
{
	
	return  GTK_TEXT_VIEW (
		 gtk_bin_get_child (GTK_BIN (
		  gtk_bin_get_child (GTK_BIN (
		   gtk_bin_get_child (GTK_BIN (window))
		  )))
		));

	/*return  GTK_TEXT_VIEW (
		 gtk_bin_get_child (GTK_BIN (
		  gtk_bin_get_child (GTK_BIN (
	      
		  gtk_container_get_children (GTK_CONTAINER (
		   gtk_bin_get_child (GTK_BIN (window))
		  ))->data)
		))));*/
}

// since reshowing all pads presents them, caller
// param will be presented afterward
void pads_set_decorations (gboolean decor, GtkWidget *caller)
{
	pad_node *temp = first_pad;

	while (temp)
	{
		if (gtk_window_get_decorated (temp->window) != decor)
		{
			gtk_window_set_decorated (temp->window, decor);
			gtk_widget_hide (GTK_WIDGET (temp->window));
			
			// we move it so wm's know where to place it
			gtk_window_move (temp->window, temp->x, temp->y);
			
			gtk_widget_show (GTK_WIDGET (temp->window));
		}
		temp = temp->next;
	}
	
	gtk_window_present (GTK_WINDOW (caller));
}

void pad_set_editable (pad_node *pad, gboolean editable)
{
	gtk_text_view_set_editable (get_text (pad->window), editable);
	gtk_text_view_set_cursor_visible (get_text (pad->window), editable);
}

gboolean pad_get_editable (pad_node *pad)
{
	return gtk_text_view_get_editable (get_text (pad->window));
}

void pads_set_editable (gboolean editable)
{
	pad_node *temp = first_pad;

	while (temp)
	{
		pad_set_editable (temp, editable);
		
		temp = temp->next;
	}
}

void pad_set_style (pad_node *pad, pad_style *pstyle)
{
	GtkRcStyle *style = gtk_widget_get_modifier_style (GTK_WIDGET (get_text (pad->window)));
	GtkRcStyle *style1 = gtk_widget_get_modifier_style (GTK_WIDGET (pad->eventbox_outer));

	style->base[GTK_STATE_NORMAL] = pstyle->back;
	style->text[GTK_STATE_NORMAL] = pstyle->text;
	style->bg[GTK_STATE_NORMAL] = pstyle->back;
	style->color_flags[GTK_STATE_NORMAL] = GTK_RC_TEXT | GTK_RC_BG | GTK_RC_BASE;
	style->font_desc = pango_font_description_from_string (pstyle->fontname);
	gtk_container_set_border_width (GTK_CONTAINER (get_text (pad->window)), pstyle->padding);

	style1->bg[GTK_STATE_NORMAL] = pstyle->border;
	style1->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
	gtk_container_set_border_width (GTK_CONTAINER (pad->eventbox), pstyle->border_width);

	gtk_widget_modify_style (GTK_WIDGET (get_text (pad->window)), style);
	gtk_widget_modify_style (GTK_WIDGET (pad->eventbox_outer), style1);

	gtk_widget_queue_draw (GTK_WIDGET (pad->eventbox_outer)); // this is necessary to show the changed border color
}

void pads_update_style ()
{
	pad_node *temp = first_pad;

	while (temp)
	{
		pad_set_style (temp, &current_settings.style);
		
		temp = temp->next;
	}
}

// returned pad_style must be g_free'd
pad_style *pad_get_style (pad_node *pad)
{
	pad_style *pstyle = (pad_style *) g_malloc (sizeof (pad_style));
	GtkStyle *style = gtk_widget_get_style (GTK_WIDGET(get_text(pad->window)));
	GtkStyle *style1 = gtk_widget_get_style (GTK_WIDGET(pad->eventbox_outer));

	pstyle->back = style->base[GTK_STATE_NORMAL];
	pstyle->text = style->text[GTK_STATE_NORMAL];
	pstyle->border = style1->bg[GTK_STATE_NORMAL];
	pstyle->border_width = gtk_container_get_border_width (GTK_CONTAINER (pad->eventbox));
	pstyle->padding = gtk_container_get_border_width (GTK_CONTAINER (get_text (pad->window)));
	strcpy (pstyle->fontname, pango_font_description_to_string (style->font_desc));

	return pstyle;
}

pad_node *get_pad (GtkWindow *window)
{
	pad_node *current = first_pad;

	while (current != NULL)
	{
		if (current->window == window)
			return current;

		current = current->next;
	}

	return NULL;
}

void quit_if_no_pads (void)
{
	if (!first_pad)
		xpad_exit();
}

// unlinks pad from linked list of all pads
void pad_remove (pad_node *pad)
{
	if (!pad || !pad->window)
		return;

	// first, find pad in linked list, and remove it 
	if (first_pad == pad) // first in list 
		first_pad = first_pad->next;
	else
	{
		pad_node *temp;
		for (temp = first_pad; temp->next; temp = temp->next)
		{
			if (temp->next == pad)
			{
				temp->next = pad->next;

				if (pad == last_pad)
					last_pad = temp;

				break;
			}
		}
	}
}

void pad_window_destroyed (GtkWidget *window, pad_node *pad)
{	
	if (verbosity >= 1) printf ("Closing pad [%s].\n", pad->infoname);

	// widget is gone, but we can clean up pad memory
	fio_save_pad (pad);
	fio_close_pad_files (pad);
	pad_remove (pad);
	
	if (verbosity >= 2) printf ("Freeing pad's memory [%s].\n", pad->infoname);
	g_free (pad);

	quit_if_no_pads ();
}

void pad_destroy (pad_node *pad)
{
	if (verbosity >= 1) printf ("Destroying pad [%s].\n", pad->infoname);

	g_signal_handlers_block_by_func (pad->window, G_CALLBACK (pad_window_destroyed), pad);

	fio_close_pad_files (pad);
	fio_remove_pad_files (pad);
	pad_remove (pad);

	if (verbosity >= 2) printf ("Freeing pad's memory [%s].\n", pad->infoname);
	gtk_widget_destroy (GTK_WIDGET(pad->window));
	g_free (pad);

	quit_if_no_pads ();
}


void pad_close (pad_node *pad)
{
	if (verbosity >= 1) printf ("Closing pad [%s].\n", pad->infoname);

	g_signal_handlers_block_by_func (pad->window, G_CALLBACK (pad_window_destroyed), pad);

	fio_save_pad (pad);
	fio_close_pad_files (pad);
	pad_remove (pad);

	if (verbosity >= 2) printf ("Freeing pad's memory [%s].\n", pad->infoname);
	gtk_widget_destroy (GTK_WIDGET(pad->window));
	g_free (pad);

	quit_if_no_pads ();
}


void pad_close_all (void)
{
	pad_node *temp = first_pad;

	while (temp)
	{
		pad_close (temp);
		temp = first_pad;
	}
}

void cleanup (void)
{
	pad_close_all();
}

/* must be full filename */
void pad_fill_with_file (pad_node *pad, const gchar *filename)
{
	gchar *contentbuf;
	struct stat statbuf;
	GtkTextBuffer *buffer;
	GtkTextView *textbox = get_text (pad->window);
	
	stat (filename, &statbuf);
	contentbuf = (gchar *) g_malloc (statbuf.st_size + 1);
	fio_get_file (filename, contentbuf, statbuf.st_size);
	buffer = gtk_text_view_get_buffer (textbox);
	gtk_text_buffer_set_text (buffer, contentbuf, -1);
	g_free (contentbuf);
}

void pad_move (pad_node *node, GdkEvent *event)
{
	GdkEventButton *eb = (GdkEventButton *) event;

	gtk_window_begin_move_drag (node->window, eb->button, eb->x_root, eb->y_root, eb->time);
}

void pad_resize (pad_node *node, GdkEvent *event)
{
	GdkEventButton *eb = (GdkEventButton *) event;

	gtk_window_begin_resize_drag (node->window, GDK_WINDOW_EDGE_SOUTH_EAST, eb->button, eb->x_root, eb->y_root, eb->time);
}

void display_dialog_with_text (pad_node *pad, const gchar *text)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (pad->window,
        		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
        		GTK_MESSAGE_INFO,
        		GTK_BUTTONS_CLOSE,
        		text);

	gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

void about_dialog (pad_node *pad)
{
	gchar text[100];

	sprintf (text, "xpad %s\n%s\n\nUsing GTK+ %i.%i.%i",
		VERSION, "http://xpad.sourceforge.net", 
		gtk_major_version, gtk_minor_version, gtk_micro_version);

	display_dialog_with_text (pad, text);
}

void pad_confirm_destroy (pad_node *pad)
{
	if (current_settings.confirm_destroy)
	{
		GtkWidget *dialog, *checkbox;
		gboolean said_yes;

		/* Create the widgets */
		dialog = gtk_message_dialog_new (pad->window,
        		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
        		GTK_MESSAGE_QUESTION,
        		GTK_BUTTONS_YES_NO,
        		"Are you sure you want\nto delete this pad?");

		checkbox = gtk_check_button_new_with_label ("Do not ask this again");
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), checkbox, FALSE, FALSE, 3);
		gtk_widget_show (checkbox);
		
		said_yes = gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_YES;

		current_settings.confirm_destroy = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox));
		fio_save_as_defaults (&current_settings);

		gtk_widget_destroy (dialog);

		if (said_yes)
			pad_destroy (pad);
	}
	else
		pad_destroy (pad);
}


void open_file_callback (GtkWidget *button, pad_node *pad)
{
	GtkTextIter s, e;
	GtkTextBuffer *buf;
	gchar *content;
	const gchar *filename;
	GtkFileSelection *selector;

	selector = GTK_FILE_SELECTION (gtk_widget_get_toplevel (button));

	filename = gtk_file_selection_get_filename (selector);

	/* test if we can read it. */
	if (open (filename, O_RDONLY) == -1)
	{
		display_dialog_with_text (pad, "Cannot open file.");
		return;
	}

	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
        content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);

	if (!strcmp (content, ""))
		pad_fill_with_file (pad, filename);
	else
	{
		pad_node *newpad = pad_new ();
		pad_fill_with_file (newpad, filename);
	}

	g_free (content);
}

void open_file (pad_node *pad)
{
	GtkWidget *filedialog;

	filedialog = gtk_file_selection_new ("Open which file?");

	g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filedialog)->ok_button),
			"clicked",
			G_CALLBACK (open_file_callback),
			(gpointer) pad);

	/* Ensure that the dialog box is destroyed when the user clicks a button. */

	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (filedialog)->ok_button),
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy), 
                             (gpointer) filedialog); 

	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (filedialog)->cancel_button),
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy),
                             (gpointer) filedialog);
	
   	gtk_window_set_position (GTK_WINDOW (filedialog), GTK_WIN_POS_CENTER);
	
	/* Display that dialog */
	gtk_widget_show (filedialog);
}

void save_as_file_callback (GtkWidget *button, pad_node *pad)
{
	GtkTextIter s, e;
	GtkTextBuffer *buf;
	gchar *content;
	const gchar *filename;
	GtkFileSelection *selector;

	selector = GTK_FILE_SELECTION (gtk_widget_get_toplevel (button));

	filename = gtk_file_selection_get_filename (selector);

	/* test if we can write to it. */
	if (open (filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR) == -1)
	{
		display_dialog_with_text (pad, "Cannot write to file.");
		return;
	}

	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
        content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);

	fio_set_file (filename, content);

	g_free (content);
}

void save_as_file (pad_node *pad)
{
	GtkWidget *filedialog;

	filedialog = gtk_file_selection_new ("Save as which file?");

	g_signal_connect (GTK_FILE_SELECTION (filedialog)->ok_button,
			"clicked",
			G_CALLBACK (save_as_file_callback),
			pad);

	/* Ensure that the dialog box is destroyed when the user clicks a button. */

	g_signal_connect_swapped (GTK_FILE_SELECTION (filedialog)->ok_button,
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy), 
                             filedialog); 

	g_signal_connect_swapped (GTK_FILE_SELECTION (filedialog)->cancel_button,
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy),
                             filedialog); 
	
	gtk_window_set_position (GTK_WINDOW (filedialog), GTK_WIN_POS_CENTER);
	
	/* Display that dialog */
	gtk_widget_show (filedialog);
}

void pad_popup (pad_node *pad, GdkEventButton *event)
{
	GtkWidget *menu = gtk_menu_new ();
	
	GtkWidget *menu_item_about;
	GtkWidget *menu_item_help;
	GtkWidget *menu_item_new_pad;
	GtkWidget *menu_item_destroy;
	GtkWidget *menu_item_close_all;
	GtkWidget *menu_item_save_as;
	GtkWidget *menu_item_open;
	GtkWidget *menu_item_preferences;
	GtkWidget *separator2, *separator3, *separator4;
	GtkWidget *tearoff;
	
	tearoff = gtk_tearoff_menu_item_new ();
	separator2 = gtk_separator_menu_item_new ();
	separator3 = gtk_separator_menu_item_new ();
	separator4 = gtk_separator_menu_item_new ();
	menu_item_about = gtk_image_menu_item_new_with_mnemonic ("_About...");
	menu_item_help = gtk_image_menu_item_new_with_mnemonic ("_Help...");
	menu_item_new_pad = gtk_image_menu_item_new_with_mnemonic ("_New");
	menu_item_save_as = gtk_image_menu_item_new_with_mnemonic ("_Save As...");
	menu_item_open = gtk_image_menu_item_new_with_mnemonic ("_Open Copy...");
	menu_item_destroy = gtk_image_menu_item_new_with_label ("Delete");
	menu_item_close_all = gtk_image_menu_item_new_with_label ("Quit");
	menu_item_preferences = gtk_image_menu_item_new_with_mnemonic ("_Preferences...");
	
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_close_all), gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_destroy), gtk_image_new_from_stock (GTK_STOCK_DELETE, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_help), gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_open), gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_preferences), gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_save_as), gtk_image_new_from_stock (GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_new_pad), gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_about), gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU));
	
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), tearoff);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_new_pad);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_open);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_save_as);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_destroy);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator2);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_preferences);
	//gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator4);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_close_all);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator3);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_help);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_about);
	
	g_signal_connect_swapped (menu_item_destroy, "activate", G_CALLBACK (pad_confirm_destroy), pad);
	g_signal_connect_swapped (menu_item_close_all, "activate", G_CALLBACK (pad_close_all), NULL);
	g_signal_connect (menu_item_new_pad, "activate", G_CALLBACK (pad_new), NULL);
	g_signal_connect_swapped (menu_item_about, "activate", G_CALLBACK (about_dialog), pad);	
	g_signal_connect_swapped (menu_item_help, "activate", G_CALLBACK (show_help), NULL);
	g_signal_connect_swapped (menu_item_open, "activate", G_CALLBACK (open_file), pad);
	g_signal_connect_swapped (menu_item_save_as, "activate", G_CALLBACK (save_as_file), pad);
	g_signal_connect_swapped (menu_item_preferences, "activate", G_CALLBACK (preferences_open), pad);
	
	gtk_widget_show_all (menu);
	
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
}


static gboolean textbox_event_handler (GtkWidget *widget, GdkEvent *event, pad_node *pad)
{
	if (event == NULL)
		return FALSE;
	
	switch (event->type)
	{
		case GDK_BUTTON_PRESS: 
		{
			GdkEventButton *event_button = (GdkEventButton *) event;
		
			switch (event_button->button)
			{
				case 1:
				// raise window if clicked on
				gtk_window_present (pad->window);
				
				if ((event_button->state & GDK_CONTROL_MASK) ||
						(current_settings.edit_lock && 
						gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)) == FALSE)) {
					pad_move (pad, event);
					return TRUE;
				}
				break;

		  		case 3:
				if (event_button->state & GDK_CONTROL_MASK)
					pad_resize (pad, event);
				else
					pad_popup (pad, event_button);
				return TRUE;
			}
		}
		break;

		// if they double click...
		case GDK_2BUTTON_PRESS:
		{
			GdkEventButton *event_button = (GdkEventButton *) event;
			
			switch (event_button->button)
			{
				case 1:
				if (current_settings.edit_lock && pad_get_editable (pad) == FALSE) {
					pad_set_editable (pad, TRUE);
					return TRUE;
				}
				break;
			}
		}
		break;
		
		case GDK_KEY_PRESS:
		{
			GdkEventKey *event_key = (GdkEventKey *) event;

			// Only interested if at least CTRL is pressed...
			if (!(event_key->state & GDK_CONTROL_MASK))
		  		return FALSE;
		
			switch (event_key->keyval)
			{
		  		case GDK_d: // CTRL + SHIFT + d == destroy pad
				if (event_key->state & GDK_SHIFT_MASK) {
					pad_destroy (pad);
					return TRUE;
				}
				break;
				
		  		case GDK_q: // CTRL + q == quit
				pad_close_all ();
				return TRUE;
				
		  		case GDK_n: // CTRL + n == new pad
				pad_new ();
				return TRUE;

		  		case GDK_o: // CTRL + o == open file
				open_file (pad);
				return TRUE;

		  		case GDK_p: // CTRL + p == pad preferences
				preferences_open (pad);
				return TRUE;

		  		case GDK_s: // CTRL + s == save as
				save_as_file (pad);
				return TRUE;

				default:
				break;
			}
		}
		break;

		default:
		break;
	}
	
	return FALSE;
}


static gboolean eventbox_event_handler (GtkWidget *widget, GdkEvent *event, pad_node *pad)
{
	GdkEventButton *event_button;

	if (event == NULL)
		return FALSE;

	switch (event->type)
	{
		case GDK_BUTTON_PRESS:
			event_button = (GdkEventButton *) event;
			
			switch (event_button->button)
			{
				case 1:
				// raise window if clicked on
				gtk_window_present (pad->window);
				
				pad_move (pad, event);
				return TRUE;
	
				case 3:
				if (event_button->state & GDK_CONTROL_MASK)
					pad_resize (pad, event);
				else
					pad_popup (pad, event_button);
				return TRUE;
			}
			break;
		
		default:
			break;
	}

	return FALSE;
}

static gboolean focus_out_handler (GtkWidget *widget, GdkEvent *event, pad_node *pad)
{
	if (event == NULL)
		return FALSE;
	
	if (current_settings.edit_lock)
	{
		pad_set_editable (pad, FALSE);
	}
	
	return TRUE;
}

static gboolean pad_save_location (GtkWidget *widget, GdkEventConfigure *event, pad_node *pad)
{
	pad->x = event->x;
	pad->y = event->y;
	pad->width = event->width;
	pad->height = event->height;
	
	return FALSE;
}

/*
   creates and returns a pad with an *unshown* window -- to 
   be decorated 
*/
pad_node *start_pad (void)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *textbox = gtk_text_view_new ();
	GtkWidget *eventbox = gtk_event_box_new ();
	GtkWidget *eventbox1 = gtk_event_box_new ();
	pad_node *pad = (pad_node *) g_malloc(sizeof(pad_node));
	gchar title[20];
	static gint num = 1;

	/* set textbox's properties */
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textbox), TRUE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textbox), GTK_WRAP_WORD);

	/* add textbox to window */
	gtk_container_add (GTK_CONTAINER (eventbox), textbox);
	gtk_container_add (GTK_CONTAINER (eventbox1), eventbox);
	gtk_container_add (GTK_CONTAINER (window), eventbox1);

	g_signal_connect (textbox, "event", G_CALLBACK (textbox_event_handler), pad);
	g_signal_connect (eventbox1, "event", G_CALLBACK (eventbox_event_handler), pad);
	g_signal_connect (window, "destroy", G_CALLBACK (pad_window_destroyed), pad);
	g_signal_connect (window, "configure-event", G_CALLBACK (pad_save_location), pad);
	g_signal_connect_after (window, "focus-out-event", G_CALLBACK (focus_out_handler), pad);

	pad->next = NULL;
	pad->window = GTK_WINDOW(window);
	pad->eventbox = eventbox;
	pad->eventbox_outer = eventbox1;

	/* check if this is first pad made */
	if (first_pad == NULL)
	{
		last_pad = pad;
		first_pad = pad;
	}
	else
	{
		last_pad->next = pad;
		last_pad = pad;
	}
	
	sprintf (title, "Pad %i", num++);
	gtk_window_set_title (GTK_WINDOW(window), title);
	
	/* set wm decorations */
	gtk_window_set_decorated (GTK_WINDOW(window), current_settings.decorations);
	
	/* set editable */
	pad_set_editable (pad, current_settings.edit_lock == 0 ? TRUE : FALSE);

	/* make sure that we only save after pad is realized */
	g_signal_connect_swapped (window, "realize", G_CALLBACK (fio_save_pad), pad);

	return pad;
}

pad_node *pad_new (void)
{
	pad_node *pad;

	if (verbosity >= 2) printf ("Making new pad.\n");

	pad = start_pad ();

	gtk_window_set_default_size (pad->window, 
		current_settings.style.padding + current_settings.style.border_width
			+ current_settings.width,
		current_settings.style.padding + current_settings.style.border_width
			+ current_settings.height);

	pad_set_style (pad, &current_settings.style);

	fio_open_pad_files (pad, TRUE);

	gtk_window_set_role (pad->window, pad->infoname);

	gtk_window_set_position (pad->window, GTK_WIN_POS_MOUSE);

	gtk_widget_show_all (GTK_WIDGET(pad->window));
	gtk_widget_grab_focus (GTK_WIDGET(get_text (pad->window)));

	return pad;
}

pad_node *pad_new_with_info (pad_info *info)
{
	pad_node *pad;

	if (verbosity >= 2) printf ("Making new pad with info.\n");

	pad = start_pad ();

	gtk_window_set_default_size (pad->window, info->width, info->height);
	gtk_window_move (pad->window, info->x, info->y);

	pad_fill_with_file (pad, info->contentname);

	pad_set_style (pad, &info->style);

	strcpy (pad->infoname, info->infoname);
	strcpy (pad->contentname, info->contentname);

	fio_open_pad_files (pad, FALSE);

	gtk_window_set_role (pad->window, pad->infoname);

	gtk_widget_show_all (GTK_WIDGET(pad->window));
	gtk_widget_grab_focus (GTK_WIDGET(get_text (pad->window)));

	return pad;
}
