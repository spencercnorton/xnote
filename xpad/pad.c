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

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "toolbar.h"
#include "pad.h"
#include "pref.h"
#include "fio.h"
#include "help.h"

pad_node *first_pad = NULL;
pad_node *last_pad = NULL;

const toolbar_button *get_toolbar_button_by_func (GCallback func)
{
	gint i;
	
	for (i = 0; i < NUM_BUTTONS; i++)
		if (buttons[i].func == func)
			return &buttons[i];
	
	return NULL;
}

const toolbar_button *get_toolbar_button_by_name (const gchar *name)
{
	gint i;
	
	for (i = 0; i < NUM_BUTTONS; i++)
		if (!g_ascii_strcasecmp (name, buttons[i].name))
			return &buttons[i];
	
	return NULL;
}

/* helper func to get textbox from window */
GtkTextView *get_text (GtkWindow *window)
{
	return  GTK_TEXT_VIEW (
		 gtk_bin_get_child (GTK_BIN (
		  gtk_bin_get_child (GTK_BIN (
		   gtk_bin_get_child (GTK_BIN (
			gtk_container_get_children (
		     GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (window)))
		   )->data
		 )))))
		));
}

/* since reshowing all pads presents them, caller
    param will be presented afterward */
void pads_set_decorations (gboolean decor, GtkWidget *caller)
{
	pad_node *temp = first_pad;

	while (temp)
	{
		if (gtk_window_get_decorated (temp->window) != decor)
		{
			gtk_window_set_decorated (temp->window, decor);
			gtk_widget_hide (GTK_WIDGET (temp->window));
			
			/* we move it so wm's know where to place it */
			gtk_window_move (temp->window, temp->x, temp->y);
			
			gtk_widget_show (GTK_WIDGET (temp->window));
		}
		temp = temp->next;
	}
	
	gtk_window_present (GTK_WINDOW (caller));
}

static void pad_set_editable (pad_node *pad, gboolean editable)
{
	GdkCursor *cursor;
	
	gtk_text_view_set_editable (get_text (pad->window), editable);
	gtk_text_view_set_cursor_visible (get_text (pad->window), editable);
	
	cursor = gdk_cursor_new(editable ? GDK_XTERM : GDK_LEFT_PTR);
	
	gdk_window_set_cursor (gtk_text_view_get_window (get_text (
		pad->window), GTK_TEXT_WINDOW_TEXT), cursor);
	
	gdk_cursor_unref (cursor);
}

void pad_clear (pad_node *pad)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	
	gtk_text_buffer_set_text (buf, "", 0);
}

static gboolean pad_get_editable (pad_node *pad)
{
	return gtk_text_view_get_editable (get_text (pad->window));
}

static gboolean pad_is_empty (pad_node *pad)
{
	GtkTextIter s, e;
	GtkTextBuffer *buf;
	gchar *content;
	gboolean rv;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
	content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);

	rv = !*content;
	
	g_free (content);
	
	return rv;
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

static void pad_update_style (pad_node *pad)
{
	GtkRcStyle *style;
	GtkRcStyle *style1;
	pad_style *pstyle;
	
	if (pad->locked)
		pstyle = &pad->style;
	else
		pstyle = &current_settings.style;
	
	style = gtk_widget_get_modifier_style (GTK_WIDGET (get_text (pad->window)));
	style1 = gtk_widget_get_modifier_style (pad->eventbox_outer);
	
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
	gtk_widget_modify_style (pad->eventbox_outer, style1);
	
	gtk_widget_queue_draw (GTK_WIDGET (pad->window));
}

void pad_style_copy (pad_style *dest, pad_style *source)
{
	dest->back = source->back;
	dest->text = source->text;
	dest->border = source->border;
	dest->border_width = source->border_width;
	dest->padding = source->padding;
	dest->fontname = g_strdup (source->fontname);
}

void pad_style_free (pad_style *style)
{
	g_free (style->fontname);
}

void pad_toolbar_update (pad_node *pad)
{
	GList *list, *tmp;
	
	toolbar_update (pad->toolbar);
	
	list = tmp = toolbar_get_buttons (pad->toolbar);
	
	while (tmp)
	{
		GCallback func;
		GtkWidget *widget = GTK_WIDGET (tmp->data);
		
		func = ((const toolbar_button *) g_object_get_data 
			(G_OBJECT (widget), "tb"))->func;
		
		if (func == G_CALLBACK (pad_toggle_lock))
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				pad->locked);
		}
		
		g_signal_connect_swapped (widget, "clicked", func, pad);
		
		tmp = tmp->next;
	}
	
	g_list_free (list);
}

static void quit_if_no_pads (void)
{
	if (!first_pad)
		gtk_main_quit ();
}

/* unlinks pad from linked list of all pads */
static void pad_remove (pad_node *pad)
{
	if (!pad || !pad->window)
		return;
	
	/*  first, find pad in linked list, and remove it  */
	if (first_pad == pad) /* first in list  */
		first_pad = first_pad->next;
	else
	{
		pad_node *temp;
		for (temp = first_pad; temp->next && (temp->next != pad); temp = temp->next)
		{
		}

		if (temp->next)
		{
			temp->next = pad->next;
			if (pad == last_pad)
				last_pad = temp;
		}
	}
}

static gboolean pad_window_destroyed (GtkWidget *window, pad_node *pad);

static void pad_free (pad_node *pad)
{
	g_signal_handlers_destroy (pad->window);
	pad_remove_toolbar (pad);
	gtk_widget_destroy (GTK_WIDGET (pad->window));
	g_free (pad->contentname);
	g_free (pad->infoname);
	pad_style_free (&pad->style);
	g_free (pad);
}

void pad_destroy (pad_node *pad)
{
	if (verbosity >= 1) printf ("Destroying pad [%s].\n", pad->infoname);

	fio_remove_pad_files (pad);
	pad_remove (pad);

	if (verbosity >= 2) printf ("Freeing pad's memory [%s].\n", pad->infoname);
	pad_free (pad);

	quit_if_no_pads ();
}

/* returns true if pad destroyed */
gboolean pad_confirm_destroy (pad_node *pad)
{
	gboolean do_destroy;
	if (!pad_is_empty (pad) && current_settings.confirm_destroy)
	{
		GtkWidget *dialog, *checkbox, *align;

		/* Create the widgets */
		dialog = gtk_message_dialog_new (pad->window,
        		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
        		GTK_MESSAGE_WARNING,
        		GTK_BUTTONS_OK_CANCEL,
        		"All contents are lost\nupon deletion.");

		align = gtk_alignment_new (1, 0.5, 0, 0);
		checkbox = gtk_check_button_new_with_label ("Don't show this warning again");
		gtk_container_add (GTK_CONTAINER (align), checkbox);
		gtk_container_set_border_width (GTK_CONTAINER (align), 6);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), align, FALSE, FALSE, 3);
		gtk_widget_show_all (align);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		do_destroy = gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK;

		/* If it has changed... */
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox)))
		{
			current_settings.confirm_destroy = FALSE;
			fio_save_default_settings (); /* to catch the change in confirmation */
		}
		
		gtk_widget_destroy (dialog);
	}
	else
	{
		do_destroy = TRUE;
	}

	if (do_destroy)
		pad_destroy (pad);

	return do_destroy;
}

void pad_close (pad_node *pad)
{
	if (verbosity >= 1) printf ("Closing pad [%s].\n", pad->infoname);
	
	fio_save_pad (pad);
	pad_remove (pad);
	
	if (verbosity >= 2) printf ("Freeing pad's memory [%s].\n", pad->infoname);
	pad_free (pad);
	
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

static gboolean pad_window_destroyed (GtkWidget *window, pad_node *pad)
{
	switch (current_settings.wm_close)
	{
	case 0: /* close all */
		pad_close_all ();
		return TRUE;
		break;
	case 1: /* close this pad */
		pad_close (pad);
		return TRUE;
		break;
	case 2: /* delete this pad */
		pad_destroy (pad);
		return TRUE;
		break;
	}
	return FALSE;
}

void cleanup (void)
{
	pad_close_all();
}


static gboolean pad_fill_with_file (pad_node *pad, const gchar *filename)
{
	gchar *contentbuf;
	GtkTextBuffer *buffer;
	GtkTextView *textbox = get_text (pad->window);
	
	contentbuf = fio_get_file (filename);
	if (!contentbuf)
		return FALSE;
	
	buffer = gtk_text_view_get_buffer (textbox);
	gtk_text_buffer_set_text (buffer, contentbuf, -1);
	g_free (contentbuf);
	return TRUE;
}

static void pad_move (pad_node *node, GdkEventButton *event)
{
	gtk_window_begin_move_drag (node->window, event->button,
		event->x_root, event->y_root, event->time);
}

static void pad_resize (pad_node *node, GdkEventButton *event)
{
	gtk_window_begin_resize_drag (node->window, 
		GDK_WINDOW_EDGE_SOUTH_EAST, event->button, event->x_root,
		event->y_root, event->time);
}

static void about_dialog (pad_node *pad)
{
	gchar text[100];

	sprintf (text, "xpad %s\n%s\n\nUsing GTK+ %i.%i.%i",
		VERSION, "http://xpad.sourceforge.net", 
		gtk_major_version, gtk_minor_version, gtk_micro_version);

	xpad_display_dialog_with_text (GTK_MESSAGE_INFO, text);
}

static void open_file_callback (GtkWidget *button, pad_node *pad)
{
	const gchar *filename;
	GtkFileSelection *selector;
	gboolean newPad;

	selector = GTK_FILE_SELECTION (gtk_widget_get_toplevel (button));

	filename = gtk_file_selection_get_filename (selector);

	newPad = !pad_is_empty(pad);
	if (newPad)
	{
		pad = pad_new();
		if (!pad)
		{
			fprintf(stderr, "Could not open new pad\n");
			return;
		}
	}
	
	if (!pad_fill_with_file(pad, filename) && newPad)
		pad_destroy (pad);	/* no need to open a new pad, if no content */
}

void pad_open_file (pad_node *pad)
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

static void save_as_file_callback (GtkWidget *button, pad_node *pad)
{
	GtkTextIter s, e;
	GtkTextBuffer *buf;
	gchar *content;
	const gchar *filename;
	GtkFileSelection *selector;

	selector = GTK_FILE_SELECTION (gtk_widget_get_toplevel (button));

	filename = gtk_file_selection_get_filename (selector);

	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
        content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);

	fio_set_file (filename, content);

	g_free (content);
}

void pad_save_as_file (pad_node *pad)
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

static gboolean
enter_handler (GtkWidget *widget, GdkEventCrossing *event, pad_node *pad)
{
	/**
	 * Here we add a toolbar so the user has quick access to several features.
	 */
	if (current_settings.toolbar &&
		event->detail != GDK_NOTIFY_INFERIOR)
	{
		if (pad->toolbar->timeout)
			toolbar_end_timeout (pad);
		
		toolbar_show (pad);
	}
	
	return FALSE;
}

static gboolean
leave_handler (GtkWidget *widget, GdkEventCrossing *event, pad_node *pad)
{
	/**
	 * Here we remove the toolbar.
	 */
	if (current_settings.toolbar &&
		event->detail != GDK_NOTIFY_INFERIOR &&
		event->mode == GDK_CROSSING_NORMAL &&
		!pad->toolbar->timeout)
	{
	    toolbar_start_timeout (pad);
	}
	
	return FALSE;
}

static void
block_toolbar_events (pad_node *pad)
{
	if (pad->toolbar)
	{
		if (pad->toolbar->timeout)
			toolbar_end_timeout (pad);
		
		g_signal_handlers_block_by_func (pad->window, (gpointer) G_CALLBACK (leave_handler), pad);
		g_signal_handlers_block_by_func (pad->window, (gpointer) G_CALLBACK (enter_handler), pad);
	}
}

static void
unblock_toolbar_events (pad_node *pad)
{
	if (pad->toolbar)
	{
		g_signal_handlers_unblock_by_func (pad->window, (gpointer) G_CALLBACK (leave_handler), pad);
		g_signal_handlers_unblock_by_func (pad->window, (gpointer) G_CALLBACK (enter_handler), pad);
	}
}

static void
connect_toolbar_events (pad_node *pad)
{
	if (pad->toolbar)
	{
		g_signal_connect (pad->window, "leave-notify-event", G_CALLBACK (leave_handler), pad);
		g_signal_connect (pad->window, "enter-notify-event", G_CALLBACK (enter_handler), pad);
	}
}

static void
disconnect_toolbar_events (pad_node *pad)
{
	if (pad->toolbar)
	{
		if (pad->toolbar->timeout)
			toolbar_end_timeout (pad);
		
		g_signal_handlers_disconnect_by_func (pad->window, (gpointer) G_CALLBACK (leave_handler), pad);
		g_signal_handlers_disconnect_by_func (pad->window, (gpointer) G_CALLBACK (enter_handler), pad);
	}
}

static void
disable_popup_handler (pad_node *pad)
{
	GdkRectangle rect;
	
	if (pad->toolbar)
	{
		unblock_toolbar_events (pad);
		
		/**
		 * We must check if we disabled off of pad and start the timeout if so.
		 */
		gdk_window_get_pointer (GTK_WIDGET (pad->window)->window,
			&rect.x, &rect.y, NULL);
		
		rect.width = 1;
		rect.height = 1;
		
		if (!gtk_widget_intersect (GTK_WIDGET (pad->window), &rect, NULL) &&
			!pad->toolbar->timeout)
			toolbar_start_timeout (pad);
	}
}

static void pad_popup (pad_node *pad, GdkEventButton *event)
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
	GtkWidget *menu_item_lock;
	GtkWidget *menu_item_clear;
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
	menu_item_clear = gtk_image_menu_item_new_with_mnemonic ("_Clear");
	menu_item_lock = gtk_check_menu_item_new_with_mnemonic ("_Lock Style");
	
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_close_all), gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_destroy), gtk_image_new_from_stock (GTK_STOCK_DELETE, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_help), gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_open), gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_preferences), gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_save_as), gtk_image_new_from_stock (GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_new_pad), gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_about), gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item_clear), gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item_lock), pad->locked);
	
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), tearoff);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_new_pad);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_open);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_save_as);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_destroy);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator4);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_clear);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_lock);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator2);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_preferences);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_close_all);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator3);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_help);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item_about);
	
	g_signal_connect_swapped (menu_item_destroy, "activate", G_CALLBACK (pad_confirm_destroy), pad);
	g_signal_connect_swapped (menu_item_close_all, "activate", G_CALLBACK (pad_close_all), NULL);
	g_signal_connect (menu_item_new_pad, "activate", G_CALLBACK (pad_new), NULL);
	g_signal_connect_swapped (menu_item_about, "activate", G_CALLBACK (about_dialog), pad);	
	g_signal_connect_swapped (menu_item_help, "activate", G_CALLBACK (show_help), NULL);
	g_signal_connect_swapped (menu_item_open, "activate", G_CALLBACK (pad_open_file), pad);
	g_signal_connect_swapped (menu_item_save_as, "activate", G_CALLBACK (pad_save_as_file), pad);
	g_signal_connect_swapped (menu_item_preferences, "activate", G_CALLBACK (preferences_open), pad);
	g_signal_connect_swapped (menu_item_clear, "activate", G_CALLBACK (pad_clear), pad);
	g_signal_connect_swapped (menu_item_lock, "toggled", G_CALLBACK (pad_toggle_lock), pad);

	block_toolbar_events (pad);
	g_signal_connect_swapped (menu, "deactivate", G_CALLBACK (disable_popup_handler), pad);
	
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
				/* raise window if clicked on */
				gtk_window_present (pad->window);
				
				if ((event_button->state & GDK_CONTROL_MASK) ||
						(current_settings.edit_lock && 
						gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)) == FALSE)) {
					pad_move (pad, event_button);
					return TRUE;
				}
				break;

		  		case 3:
				if (event_button->state & GDK_CONTROL_MASK)
					pad_resize (pad, event_button);
				else
					pad_popup (pad, event_button);
				return TRUE;
			}
		}
		break;

		/* if they double click... */
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

			/* Only interested if at least CTRL is pressed... */
			if (!(event_key->state & GDK_CONTROL_MASK))
		  		return FALSE;
		
			switch (event_key->keyval)
			{
		  		case GDK_d: /* CTRL + SHIFT + d == destroy pad */
				if (event_key->state & GDK_SHIFT_MASK) {
					pad_destroy (pad);
					return TRUE;
				}
				break;
				
		  		case GDK_q: /* CTRL + q == quit */
				pad_close_all ();
				return TRUE;
				
		  		case GDK_n: /* CTRL + n == new pad */
				pad_new ();
				return TRUE;

		  		case GDK_o: /* CTRL + o == open file */
				pad_open_file (pad);
				return TRUE;

		  		case GDK_p: /* CTRL + p == pad preferences */
				preferences_open ();
				return TRUE;

		  		case GDK_s: /* CTRL + s == save as */
				pad_save_as_file (pad);
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


static gboolean window_button_handler (GtkWidget *widget, GdkEventButton *event, pad_node *pad)
{
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;
	
	switch (event->button)
	{
	case 1:
		/* raise window if clicked on */
		gtk_window_present (pad->window);
		
		pad_move (pad, event);
		return TRUE;
	
	case 3:
		if (event->state & GDK_CONTROL_MASK)
			pad_resize (pad, event);
		else
			pad_popup (pad, event);
		return TRUE;
	}
	
	return FALSE;
}

void
pad_lock_style (pad_node *pad)
{
	pad_style_free (&pad->style);
	pad_style_copy (&pad->style, &current_settings.style);
	pad->locked = 1;
}

void
pad_unlock_style (pad_node *pad)
{
	pad->locked = 0;
	pad_update_style (pad);
}

void
pad_toggle_lock (pad_node *pad)
{
	if (pad->locked)
		pad_unlock_style (pad);
	else
		pad_lock_style (pad);
}

static gboolean
focus_in_handler (GtkWidget *widget, GdkEventFocus *event, pad_node *pad)
{
	return FALSE;
}

static gboolean
focus_out_handler (GtkWidget *widget, GdkEventFocus *event, pad_node *pad)
{
	if (current_settings.edit_lock && pad_get_editable (pad))
		pad_set_editable (pad, FALSE);
	
	return FALSE;
}

static gboolean pad_save_location (GtkWidget *widget, GdkEventConfigure *event, pad_node *pad)
{
	pad->x = event->x;
	pad->y = event->y;
	pad->width = event->width;
	pad->height = event->height;
	
	return FALSE;
}

static void
pad_when_textbox_realized (GtkWidget *widget, pad_node *pad)
{
	/* set editable */
	pad_set_editable (pad, current_settings.edit_lock == 0);
	
	/* we want to start off with valid values for position/size */
	gtk_window_get_size (pad->window, &pad->width, &pad->height);
	gtk_window_get_position (pad->window, &pad->x, &pad->y);
}

static gboolean
grip_press_handler (GtkWidget *widget, GdkEventButton *event, pad_node *pad)
{
	if (event->button == 1)
		gtk_window_begin_resize_drag (pad->window,
			GDK_WINDOW_EDGE_SOUTH_EAST,
			event->button,
			event->x_root, event->y_root,
			event->time);
	else
		gtk_window_begin_move_drag (pad->window,
			event->button,
			event->x_root, event->y_root,
			event->time);
	
	return TRUE;
}

void
pad_remove_toolbar (pad_node *pad)
{
	if (verbosity >= 2) printf ("Removing toolbar from pad.\n");

	if (pad->toolbar)
	{
		disconnect_toolbar_events (pad);
		
		toolbar_hide (pad);
		gtk_container_remove (GTK_CONTAINER (pad->box), pad->toolbar->bar);
		/*g_free (pad->toolbar->bar);*/
		g_free (pad->toolbar);
		pad->toolbar = NULL;
	}
}

void
pad_add_toolbar (pad_node *pad)
{
	if (verbosity >= 2) printf ("Adding toolbar to pad.\n");
	
	if (!pad->toolbar)
	{
		pad->toolbar = toolbar_new ();
		
		gtk_box_pack_start (GTK_BOX (pad->box), pad->toolbar->bar, FALSE, FALSE, 0);
		
		g_signal_connect (G_OBJECT (pad->toolbar->grip), "button-press-event", 
			G_CALLBACK (grip_press_handler), pad);
		
		pad_toolbar_update (pad);
		
		connect_toolbar_events (pad);
	}
}

/*
   creates and returns a pad with an *unshown* window -- to 
   be decorated 
*/
static pad_node *start_pad (void)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *textbox = gtk_text_view_new ();
	GtkWidget *eventbox = gtk_event_box_new ();
	GtkWidget *eventbox1 = gtk_event_box_new ();
	GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
	GtkWidget *box = gtk_vbox_new (FALSE, 0);
	pad_node *pad = (pad_node *) g_malloc(sizeof(pad_node));

	gchar title[20];
	static gint num = 1;

	/* set textbox's properties */
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textbox), TRUE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textbox), GTK_WRAP_WORD);

	/* set up scrollbar */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
		GTK_SHADOW_NONE);
	
	/* add textbox to window */
	gtk_container_add (GTK_CONTAINER (scroll), textbox);
	gtk_container_add (GTK_CONTAINER (eventbox), scroll);
	gtk_container_add (GTK_CONTAINER (eventbox1), eventbox);
	gtk_box_pack_start (GTK_BOX (box), eventbox1, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (window), box);
	
	/* We want to make xpad moveable anywhere a lower widget doesn't have priority */
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);
	g_signal_connect (window, "button-press-event", G_CALLBACK (window_button_handler), pad);

	g_signal_connect (textbox, "event", G_CALLBACK (textbox_event_handler), pad);
	/*//g_signal_connect (eventbox1, "event", G_CALLBACK (eventbox_event_handler), pad);*/
	g_signal_connect (window, "destroy", G_CALLBACK (pad_window_destroyed), pad);
	g_signal_connect (window, "configure-event", G_CALLBACK (pad_save_location), pad);
	g_signal_connect_after (window, "focus-out-event", G_CALLBACK (focus_out_handler), pad);
	g_signal_connect_after (window, "focus-in-event", G_CALLBACK (focus_in_handler), pad);
	
	g_object_set_data (G_OBJECT (window), "pad", pad);
	
	pad->next = NULL;
	pad->window = GTK_WINDOW(window);
	pad->eventbox = eventbox;
	pad->eventbox_outer = eventbox1;
	pad->scrollbar = scroll;
	pad->box = box;
	pad->toolbar = NULL;
	
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
	
	gtk_window_set_gravity (GTK_WINDOW (window), GDK_GRAVITY_STATIC);
	
	if (current_settings.toolbar)
		pad_add_toolbar (pad);
	
	/* set wm decorations */
	gtk_window_set_decorated (GTK_WINDOW(window), current_settings.decorations);
	
	/* make sure that we save after pad is realized */
	g_signal_connect_after (textbox, "realize", G_CALLBACK 
		(pad_when_textbox_realized), pad);
	
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
	
	fio_open_pad_files (pad, TRUE);
	
	pad_style_copy (&pad->style, &current_settings.style);
	pad_update_style (pad);
	
	gtk_window_set_position (pad->window, GTK_WIN_POS_MOUSE);
	pad->locked = 0;
	
	gtk_widget_show_all (pad->eventbox_outer);
	gtk_widget_show (pad->box);
	gtk_widget_show (GTK_WIDGET(pad->window));
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
	
	pad->locked = info->locked;
	pad->infoname = info->infoname;
	pad->contentname = info->contentname;
	
	pad_style_copy (&pad->style, &info->style);
	pad_update_style (pad);
	
	fio_open_pad_files (pad, FALSE);
	
	gtk_widget_show_all (pad->eventbox_outer);
	gtk_widget_show (pad->box);
	gtk_widget_show (GTK_WIDGET(pad->window));
	gtk_widget_grab_focus (GTK_WIDGET(get_text (pad->window)));
	
	return pad;
}
