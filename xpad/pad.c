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

static void
menuitem_cb (gpointer callback_data, guint callback_action, GtkWidget *widget);

static GtkItemFactoryEntry menu_items[] = 
{
	{"/_File", 					NULL,					0,				0, 	"<Branch>"},
	{"/File/_New",				"<control>N",			menuitem_cb, 	1,	"<StockItem>",	GTK_STOCK_NEW},
	{"/File/_Open",				"<control>O",			menuitem_cb,	2,	"<StockItem>",	GTK_STOCK_OPEN},
	{"/File/sep", 				NULL,					0,				0,	"<Separator>"},
	{"/File/Save _As",			"<shift><control>S",	menuitem_cb,	3,	"<StockItem>",	GTK_STOCK_SAVE_AS},
	{"/File/sep2", 				NULL,					0,				0,	"<Separator>"},
	{"/File/_Close",			"<control>W",			menuitem_cb,	4,	"<StockItem>",	GTK_STOCK_CLOSE},
	{"/File/_Delete",			NULL,					menuitem_cb,	5,	"<StockItem>",	GTK_STOCK_DELETE},
	{"/File/_Quit",				"<control>Q",			menuitem_cb,	6,	"<StockItem>",	GTK_STOCK_QUIT},
	{"/_Edit",					NULL,					0,				0,	"<Branch>"},
	{"/Edit/C_ut",				"<control>X",			menuitem_cb,	11,	"<StockItem>",	GTK_STOCK_CUT},
	{"/Edit/_Copy",				"<control>C",			menuitem_cb,	12,	"<StockItem>",	GTK_STOCK_COPY},
	{"/Edit/_Paste",			"<control>V",			menuitem_cb,	13,	"<StockItem>",	GTK_STOCK_PASTE},
	{"/Edit/Clear Pad",			NULL,					menuitem_cb,	14,	"<StockItem>",	GTK_STOCK_CLEAR},
	{"/Edit/sep",				NULL,					0,				0,	"<Separator>"},
	{"/Edit/_Preferences",		NULL,					menuitem_cb,	7,	"<StockItem>",	GTK_STOCK_PREFERENCES},
	{"/_Windows",				NULL,					0,				0,	"<Branch>"},
	{"/_Help",					NULL,					0,				0,	"<Branch>"},
	{"/Help/_Contents",			"F1",					menuitem_cb,	8,	"<StockItem>",	GTK_STOCK_HELP},
	{"/Help/_About",			NULL,					menuitem_cb,	9,	"<StockItem>",	GTK_STOCK_DIALOG_INFO}
};

#define SHOW_ACTION_OFFSET		10000

static GtkAccelGroup *accel_group = NULL;

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
			
			if (!temp->hidden)
			{
				gtk_widget_hide (GTK_WIDGET (temp->window));
				
				/* we move it so wm's know where to place it */
				gtk_window_move (temp->window, temp->x, temp->y);
				
				gtk_widget_show (GTK_WIDGET (temp->window));
			}
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
	
	pad_background_clear (pad);
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

	rv = strcmp (content, "") == 0;
	
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

void pad_set_scrollbars (pad_node *pad, gboolean on)
{
	if (on)
	{
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pad->scrollbar), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	}
	else
	{
		GtkAdjustment *v, *h;
		
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pad->scrollbar), 
			GTK_POLICY_NEVER, GTK_POLICY_NEVER);
		
		/* now we need to adjust view so that user can see whole pad */
		h = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
		v = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
		
		gtk_adjustment_set_value (h, 0);
		gtk_adjustment_set_value (v, 0);
	}
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
		else if (func == G_CALLBACK (pad_toggle_sticky))
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				pad->sticky);
		}
		
		g_signal_connect_swapped (widget, "clicked", func, pad);
		
		tmp = tmp->next;
	}
	
	g_list_free (list);
}

/* sets a toggleable widget to a particular value */
static void
pad_toolbar_set_widget (pad_node *pad, GCallback target_func, gboolean value)
{
	GList *list, *tmp;
	
	list = tmp = toolbar_get_buttons (pad->toolbar);
	
	while (tmp)
	{
		GCallback func;
		GtkWidget *widget = GTK_WIDGET (tmp->data);
		
		func = ((const toolbar_button *) g_object_get_data 
			(G_OBJECT (widget), "tb"))->func;
		
		if (func == target_func)
		{
			g_signal_handlers_block_by_func (widget, (gpointer) func, pad);
			
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				value);
			
			g_signal_handlers_unblock_by_func (widget, (gpointer) func, pad);
			
			break;
		}
		
		tmp = tmp->next;
	}
	
	g_list_free (list);
}

static void
pad_set_sticky (pad_node *pad, gboolean on)
{
	if (on)
	{
		gtk_window_stick (pad->window);
		pad->sticky = 1;
		
		/* make sure the toolbar widget is up to date */
		pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_sticky), TRUE);
	}
	else
	{
		gtk_window_unstick (pad->window);
		pad->sticky = 0;
		
		/* make sure the toolbar widget is up to date */
		pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_sticky), FALSE);
	}

}

void pad_toggle_sticky (pad_node *pad)
{
	pad_set_sticky (pad, !pad->sticky);
}

void pad_edit_cut (pad_node *pad)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	
	gtk_text_buffer_cut_clipboard (buf, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), TRUE);
}

void pad_edit_copy (pad_node *pad)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	
	gtk_text_buffer_copy_clipboard (buf, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

void pad_edit_paste (pad_node *pad)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));

	gtk_text_buffer_paste_clipboard (buf, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), NULL, TRUE);
}

static void quit_if_no_pads (void)
{
	gboolean alive = FALSE;
	pad_node *p = first_pad;
	
	while (p)
	{
		if (!p->hidden)
			alive = TRUE;
		
		p = p->next;
	}
	
	if (!alive)
		pad_close_all ();
}

/* unlinks pad from linked list of all pads */
static void pad_remove (pad_node *pad)
{
	if (!pad)
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
static void pad_renew (pad_node *pad);

static void
pad_free_gtk (pad_node *pad)
{
	g_signal_handlers_destroy (pad->window);
	pad_remove_toolbar (pad);
	gtk_widget_destroy (GTK_WIDGET (pad->window));
	g_free (pad->menu);
	
	pad->window = NULL;
}

static void pad_free (pad_node *pad)
{
	/* have we already freed gtk stuff? */
	if (pad->window)
		pad_free_gtk (pad);
	
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
	
	toolbar_hide (pad);
	
	fio_save_pad (pad);
	
	pad_free_gtk (pad);
	
	pad->hidden = TRUE;
	
	quit_if_no_pads ();
}

void pad_show (pad_node *pad)
{
	if (pad->hidden)
	{
		/* if we are reshowing ourselves from a hide, we have to rebuild the pad */
		pad_renew (pad);
	}
	
	pad->hidden = FALSE;
	
	gtk_window_present (pad->window);
}

void pads_show_all (void)
{
	pad_node *temp = first_pad;
	
	while (temp)
	{
		pad_show (temp);
		
		temp = temp->next;
	}
}

void pad_show_all (pad_node *pad)
{
	pads_show_all ();
	pad_show (pad);
}

void pad_close_all (void)
{
	pad_node *temp = first_pad;
	
	while (temp)
	{
		if (temp->window)
		{
			fio_save_pad (temp);
		}
		
		pad_remove (temp);
		pad_free (temp);
		
		temp = first_pad;
	}
	
	gtk_main_quit ();
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
	
	g_free (accel_group);
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


static void
menuitem_cb (gpointer callback_data, guint callback_action, GtkWidget *widget)
{
	pad_node *pad;
	
	/**
	 * Sigh...  If the user presses the keyboard accelerator for one of the 
	 * itemfactory entries, this function is run, but without a useful widget value.
	 * Thus, we have no way of finding out what pad to use.  So, what we do is 
	 * iterate over windows, finding the one with focus.
	 */
	pad = first_pad;
	while (pad)
	{
		if (!pad->hidden)
		{
			GtkWidget *w;
			
			w = gtk_window_get_focus (pad->window);
			
			if (GTK_WIDGET_HAS_FOCUS (w))
				break;
		}
		
		pad = pad->next;
	}
	
	/* if no pad has focus, it must have been through popup menu */
	if (!pad)
		pad = (pad_node *) g_object_get_data (G_OBJECT (gtk_item_factory_from_widget (widget)), "pad");
	
	switch (callback_action)
	{
	case 1:
		pad_new ();
		break;
	
	case 2:
		pad_open_file (pad);
		break;
	
	case 3:
		pad_save_as_file (pad);
		break;
	
	case 4:
		pad_close (pad);
		break;
	
	case 5:
		pad_confirm_destroy (pad);
		break;
	
	case 6:
		pad_close_all ();
		break;
	
	case 7:
		preferences_open ();
		break;
	
	case 8:
		show_help ();
		break;
	
	case 9:
		about_dialog (pad);
		break;
	
	case 10:
		pad_show_all (pad);
		break;
	
	case 11:
		pad_edit_cut (pad);
		break;
	
	case 12:
		pad_edit_copy (pad);
		break;
	
	case 13:
		pad_edit_paste (pad);
		break;
	
	case 14:
		pad_clear (pad);
		break;
	
	default:
		break;
	}
	
	if (callback_action >= SHOW_ACTION_OFFSET)
	{
		pad_show ((pad_node *) callback_data);
	}
}

static void pad_popup (pad_node *pad, GdkEventButton *event)
{
	pad_node *p = first_pad;
	GtkWidget *tmp;
	gint n = 0, i = SHOW_ACTION_OFFSET;
	GtkItemFactoryEntry entry;
	const gchar submenu[12] = "/Windows";
	
	/**
	 * Remove old items.
	 * Here's the deal:  There is no good way to iterate through item factory, since
	 * changes don't keep unless you use itemfactory's api.  This api does not allow
	 * iteration.  Thus, we use guessable action numbers for temporary items, like 10000+
	 */
	do
	{
		tmp = gtk_item_factory_get_item_by_action (pad->menu, i++);
		
		if (tmp)
		{
			gtk_item_factory_delete_item (pad->menu, 
				gtk_item_factory_path_from_widget (tmp));
		}
	}
	while (tmp);
	
	gtk_item_factory_delete_item (pad->menu, "/Windows/sep");
	gtk_item_factory_delete_item (pad->menu, "/Windows/Show All");
	gtk_item_factory_delete_item (pad->menu, "/Windows/Close All");
	
	/**
	 * Populate list of windows.
	 */
	while (p)
	{
		gchar result [12 + TITLE_CHARS + 23];	/* 1 null, 1 num, 2 quotes, and 7 for possible markup */
		
		n++;
		
		sprintf (result, "%s/%i. ", submenu, n);
		/*
		if (p->hidden)
			strcat (result, "<i>");
		*/
		strcat (result, p->title);
		/*
		if (p->hidden)
			strcat (result, "</i>");
		*/
		/*
		menu_item = gtk_menu_item_new ();
		label = gtk_label_new (NULL);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
		gtk_label_set_markup (GTK_LABEL (label), result);
		gtk_container_add (GTK_CONTAINER (menu_item), label);
		*/
		
		entry.path = result;
		entry.accelerator = NULL;
		entry.callback = menuitem_cb;
		entry.callback_action = SHOW_ACTION_OFFSET + n - 1;
		entry.item_type = "<Item>";
		
		gtk_item_factory_create_item (pad->menu, &entry, p, 1);
		
		p = p->next;
	}
	
	entry.path = "/Windows/sep";
	entry.accelerator = NULL;
	entry.callback = NULL;
	entry.callback_action = 0;
	entry.item_type = "<Separator>";
	gtk_item_factory_create_item (pad->menu, &entry, NULL, 1);
	
	entry.path = "/Windows/_Show All";
	entry.accelerator = NULL;
	entry.callback = menuitem_cb;
	entry.callback_action = 10;
	entry.item_type = "<Item>";
	gtk_item_factory_create_item (pad->menu, &entry, NULL, 1);
		
	entry.path = "/Windows/_Close All";
	entry.accelerator = "<control>Q";
	entry.callback = menuitem_cb;
	entry.callback_action = 6;
	entry.item_type = "<StockItem>";
	entry.extra_data = GTK_STOCK_QUIT;
	gtk_item_factory_create_item (pad->menu, &entry, NULL, 1);
	
	block_toolbar_events (pad);
	
	gtk_item_factory_popup (pad->menu, event->x_root, event->y_root, event->button, event->time);
}

static void pad_background_draw (pad_node *pad, gint x, gint y);

static gboolean textbox_event_handler (GtkWidget *widget, GdkEvent *event, pad_node *pad)
{
	if (event == NULL)
		return FALSE;
	
	switch (event->type)
	{
#if DRAWING_ON
		case GDK_MOTION_NOTIFY:
		{
			GdkEventMotion *event_motion = (GdkEventMotion *) event;
			gint x, y;
			GdkModifierType mask;
			
			gdk_window_get_pointer (event_motion->window, &x, &y, &mask);
			
			if ((mask & GDK_CONTROL_MASK) &&
				(mask & GDK_SHIFT_MASK) &&
				(mask & GDK_BUTTON1_MASK))
			{
				pad_background_draw (pad, x, y);
				return TRUE;
			}
		}
		break;
		
		case GDK_BUTTON_RELEASE:
		{
			GdkEventButton *event_button = (GdkEventButton *) event;
			
			switch (event_button->button)
			{
				case 1:
					pad->last_draw_x = pad->last_draw_y = -1;
					return FALSE;
			}
		}
		break;
#endif
		
		case GDK_BUTTON_PRESS: 
		{
			GdkEventButton *event_button = (GdkEventButton *) event;
		
			switch (event_button->button)
			{
				case 1:
				/* raise window if clicked on */
				gtk_window_present (pad->window);
				
				if ((event_button->state & GDK_CONTROL_MASK) &&
					(event_button->state & GDK_SHIFT_MASK))
				{
					pad_background_draw (pad, event_button->x, event_button->y);
					return TRUE;
				}
				
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
	
	/* make sure the toolbar widget is up to date */
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_lock), TRUE);
}

void
pad_unlock_style (pad_node *pad)
{
	pad->locked = 0;
	pad_update_style (pad);
	
	/* make sure the toolbar widget is up to date */
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_lock), FALSE);
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

static void
pad_background_refresh (pad_node *pad)
{
#if DRAWING_ON
	GdkWindow *win;
	GdkRectangle rect = {0, 0, 0, 0};
	
	rect.width = pad->width;
	rect.height = pad->height;
	
	win = gtk_text_view_get_window (get_text (pad->window), GTK_TEXT_WINDOW_TEXT);
	
	gdk_window_invalidate_rect (win, &rect, FALSE);
#endif
}

static void
pad_background_draw (pad_node *pad, gint x, gint y)
{
#if DRAWING_ON
	GdkRectangle brush;
	GtkWidget *textbox;
	GdkWindow *textwin;
	GtkAdjustment *ha, *va;
	
	if (!pad->visible_back || !pad->background)
		return;
	
	brush.x = x ;
	brush.y = y ;
	brush.width = 1;
	brush.height = 1;
	
	textbox = GTK_WIDGET (get_text (pad->window));
	textwin = gtk_text_view_get_window (GTK_TEXT_VIEW (textbox), GTK_TEXT_WINDOW_TEXT);
	
	ha = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	va = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	
	/* draw brush on both current background and master background */
	if (pad->last_draw_x > -1 && pad->last_draw_y > -1)
	{
		gdk_draw_line (pad->visible_back,
						textbox->style->text_gc[GTK_STATE_NORMAL],
						pad->last_draw_x, pad->last_draw_y,
						brush.x, brush.y);
		gdk_draw_line (pad->background,
						textbox->style->text_gc[GTK_STATE_NORMAL],
						ha->value + pad->last_draw_x, va->value + pad->last_draw_y,
						ha->value + brush.x, va->value + brush.y);
	}
	else
	{
		gdk_draw_rectangle (pad->visible_back,
						textbox->style->text_gc[GTK_STATE_NORMAL],
						TRUE,
						brush.x, brush.y, brush.width, brush.height);
		gdk_draw_rectangle (pad->background,
						textbox->style->text_gc[GTK_STATE_NORMAL],
						TRUE,
						ha->value + brush.x, va->value + brush.y, brush.width, brush.height);
	}
	
	/* now make change visible */
	pad_background_refresh (pad);
	
	pad->last_draw_x = brush.x;
	pad->last_draw_y = brush.y;
#endif
}

static void
pad_background_update (pad_node *pad)
{
#if DRAWING_ON
	GtkAdjustment *ha, *va;
	GtkWidget *textbox;
	GdkWindow *textwin;
	GdkPixmap *pix;
	
	textbox = GTK_WIDGET (get_text (pad->window));
	textwin = gtk_text_view_get_window (GTK_TEXT_VIEW (textbox), GTK_TEXT_WINDOW_TEXT);
	
	ha = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	va = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	
	printf ("making %i, %i, from %i, %i\n", pad->width, pad->height, (int)ha->upper, (int)va->upper);
	
	pix = gdk_pixmap_new (textwin, pad->width, pad->height, -1);
	
	gdk_draw_rectangle (pix, textbox->style->bg_gc[GTK_STATE_NORMAL], 1,
		0, 0, pad->width, pad->height);
	gdk_draw_drawable (pix, textbox->style->bg_gc[GTK_STATE_NORMAL], pad->background,
		ha->value, va->value, 0, 0, pad->width, pad->height);
	
	gdk_window_set_back_pixmap (textwin, pix, FALSE);
	
	if (pad->visible_back) gdk_pixmap_unref (pad->visible_back);
	pad->visible_back = pix;
#endif
}

static void
pad_scrolled (GtkAdjustment *adjustment, pad_node *pad)
{
#if DRAWING_ON
	pad_background_update (pad);
	
	pad_background_refresh (pad);
#endif
}

static void
pad_resize_background (pad_node *pad)
{
#if DRAWING_ON
	GtkAdjustment *ha, *va;
	GtkWidget *textbox;
	GdkWindow *textwin;
	GdkPixmap *pix;
	gint height, width;
	
	textbox = GTK_WIDGET (get_text (pad->window));
	textwin = gtk_text_view_get_window (GTK_TEXT_VIEW (textbox), GTK_TEXT_WINDOW_TEXT);
	
	ha = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	va = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	
	width = ha->upper;
	height = va->upper;
	pix = gdk_pixmap_new (textwin, width, height, -1);
	
	printf ("background is %i, %i\n", width, height);
	
	if (pad->background)
	{
		gdk_draw_rectangle (pix, textbox->style->bg_gc[GTK_STATE_NORMAL], 1, 0, 0, width, height);
		gdk_draw_drawable (pix, textbox->style->bg_gc[GTK_STATE_NORMAL], pad->background, 0, 0, 0, 0, -1, -1);
		
		gdk_pixmap_unref (pad->background);
	}
	else
	{
		gdk_draw_rectangle (pix, textbox->style->bg_gc[GTK_STATE_NORMAL], 1, 0, 0, width, height);
	}
	
	pad->background = pix;
	
	pad_background_update (pad);
#endif
}

static void
pad_v_scroll_changed (GtkAdjustment *adjustment, pad_node *pad)
{
#if DRAWING_ON
	gint h;
	gboolean doit = FALSE;
	
	/* This is called if an adjustment member other than it's 'value'
		changed -- here we are concerned about the 'upper' member */
	
	/* Here we find out if the upper member was the actual member changed */
	printf ("v changed\n");
	
	if (!pad->background)
	{
		doit = TRUE;
	}
	
	if (!doit)
	{
		gdk_drawable_get_size (pad->background, NULL, &h);
		
		if (h != adjustment->upper)
			doit = TRUE;
	}
	
	if (doit)
	{
		printf ("v changed for real\n");
		if (GTK_WIDGET_REALIZED (GTK_WIDGET (get_text (pad->window))))
			pad_resize_background (pad);
	}
#endif
}

static void
pad_h_scroll_changed (GtkAdjustment *adjustment, pad_node *pad)
{
#if DRAWING_ON
	gint w;
	gboolean doit = FALSE;
	
	/* This is called if an adjustment member other than it's 'value'
		changed -- here we are concerned about the 'upper' member */
	
	/* Here we find out if the upper member was the actual member changed */
	
	printf ("h changed\n");
	
	if (!pad->background)
	{
		doit = TRUE;
	}
	
	if (!doit)
	{
		printf ("drawable exists\n");
		gdk_drawable_get_size (pad->background, &w, NULL);
		
		if (w != adjustment->upper)
			doit = TRUE;
	}
	
	if (doit)
	{
		printf ("v changed for real\n");
		if (GTK_WIDGET_REALIZED (GTK_WIDGET (get_text (pad->window))))
			pad_resize_background (pad);
	}
#endif
}

void pad_background_clear (pad_node *pad)
{
#if DRAWING_ON
	GtkWidget *textbox;
	gint w, h;
	
	textbox = GTK_WIDGET (get_text (pad->window));
	
	gdk_drawable_get_size (pad->background, &w, &h);
	gdk_draw_rectangle (pad->background, textbox->style->bg_gc[GTK_STATE_NORMAL], 1, 0, 0, w, h);
	
	pad_background_update (pad);
	
	pad_background_refresh (pad);
#endif
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
	
	pad_resize_background (pad);
}

static gboolean
grip_press_handler (GtkWidget *widget, GdkEventButton *event, pad_node *pad)
{
	if (event->button == 1)
	{
		pad_resize (pad, event);
	}
	else
	{
		pad_move (pad, event);
	}
	
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

gboolean state_handler (GtkWidget *window, GdkEvent *event, pad_node *pad)
{
	return TRUE;
}


void pad_set_title (pad_node *pad)
{
	GtkTextBuffer *buf;
	GtkTextIter s, e;
	gchar *content, *tmp;
	gint n;
	gchar result [TITLE_CHARS + 1];	/* 1 null and TITLE_CHARS characters */
	
	buf = gtk_text_view_get_buffer (get_text (pad->window));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
	content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);
	
	tmp = content;
	n = 0;
	
	/**
	 * Unfortunately, I had some real problems taking the UTF-8 encoded text from
	 * the text buffer to play nice with input to gtk_window_set_title.  Thus, this quick
	 * little utf8->ascii conversion must take place.
	 */
	
	while (n < TITLE_CHARS)
	{
		gunichar u;
		
		u = g_utf8_get_char (tmp);
		
		if (u <= 0x7F)	/* is ASCII*/
		{
			if (g_unichar_isgraph (u))
			{
				result[n++] = (char) u;
			}
			else if (g_unichar_isspace (u) && n > 0)
			{
				result[n++] = ' ';
			}
			else if (u == '\0')
			{
				break;
			}
		}
		
		tmp = g_utf8_next_char (tmp);
	}
	
	result[n] = '\0';
	
	sprintf (pad->title, "\"%s\"", result);
	
	gtk_window_set_title (pad->window, pad->title);
}


gboolean text_changed (GtkTextBuffer *buf, pad_node *pad)
{
	pad_set_title (pad);
	
	return TRUE;
}

static void
pad_alloc_gtk (pad_node *pad)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *textbox = gtk_text_view_new ();
	GtkWidget *eventbox = gtk_event_box_new ();
	GtkWidget *eventbox1 = gtk_event_box_new ();
	GtkWidget *box = gtk_vbox_new (FALSE, 0);
	GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
	GtkTextBuffer *textbuf;
	
	/* set textbox's properties */
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textbox), TRUE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textbox), GTK_WRAP_WORD);
	
	/* set up scrollbar */
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
		GTK_SHADOW_NONE);
	
	/* add textbox to window */
	gtk_container_add (GTK_CONTAINER (scroll), textbox);
	gtk_container_add (GTK_CONTAINER (eventbox), scroll);
	gtk_container_add (GTK_CONTAINER (eventbox1), eventbox);
	gtk_box_pack_start (GTK_BOX (box), eventbox1, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (window), box);
	
	textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textbox));
	
	/* We want to make xpad moveable anywhere a lower widget doesn't have priority */
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);
	g_signal_connect (window, "button-press-event", G_CALLBACK (window_button_handler), pad);
	
	g_signal_connect (textbox, "event", G_CALLBACK (textbox_event_handler), pad);
	/*g_signal_connect (eventbox1, "event", G_CALLBACK (eventbox_event_handler), pad);*/
	g_signal_connect (window, "destroy", G_CALLBACK (pad_window_destroyed), pad);
	g_signal_connect (window, "configure-event", G_CALLBACK (pad_save_location), pad);
	g_signal_connect_after (window, "focus-out-event", G_CALLBACK (focus_out_handler), pad);
	g_signal_connect_after (window, "focus-in-event", G_CALLBACK (focus_in_handler), pad);
/*	g_signal_connect (window, "window-state-event", G_CALLBACK (state_handler), pad);*/
	g_signal_connect (textbuf, "changed", G_CALLBACK (text_changed), pad);
	
	g_object_set_data (G_OBJECT (window), "pad", pad);
	
	pad->window = GTK_WINDOW (window);
	pad->eventbox = eventbox;
	pad->eventbox_outer = eventbox1;
	pad->box = box;
	pad->toolbar = NULL;
	pad->scrollbar = scroll;
	
	gtk_window_add_accel_group (pad->window, accel_group);
	pad->menu = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", accel_group);
	
	gtk_item_factory_create_items (pad->menu, G_N_ELEMENTS (menu_items), menu_items, pad);
	g_object_set_data (G_OBJECT (pad->menu), "pad", pad);
	g_signal_connect_swapped (G_OBJECT (gtk_item_factory_get_widget (pad->menu, "<main>")),
		"deactivate", G_CALLBACK (disable_popup_handler), pad);
	
	gtk_window_set_gravity (GTK_WINDOW (window), GDK_GRAVITY_STATIC);
	
	if (current_settings.toolbar)
		pad_add_toolbar (pad);
	
	/* set wm decorations */
	gtk_window_set_decorated (GTK_WINDOW(window), current_settings.decorations);
	
	pad_set_scrollbars (pad, current_settings.scrollbar);
	
	/* make sure that we save after pad is realized */
	g_signal_connect_after (textbox, "realize", G_CALLBACK 
		(pad_when_textbox_realized), pad);
	
	g_signal_connect (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar))
		, "value-changed", G_CALLBACK (pad_scrolled), pad);
	g_signal_connect (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar))
		, "value-changed", G_CALLBACK (pad_scrolled), pad);
	g_signal_connect (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar))
		, "changed", G_CALLBACK (pad_v_scroll_changed), pad);
	g_signal_connect (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar))
		, "changed", G_CALLBACK (pad_h_scroll_changed), pad);
}

/*
   creates and returns a pad with an *unshown* window -- to 
   be decorated 
*/
static pad_node *start_pad (void)
{
	pad_node *pad = (pad_node *) g_malloc(sizeof(pad_node));
	static gint num = 1;
	
	pad->next = NULL;
#if DRAWING_ON
	pad->background = NULL;
	pad->visible_back = NULL;
	pad->last_draw_x = pad->last_draw_y = -1;
#endif
	pad->num = num++;
	pad->hidden = FALSE;
	
	/* check if this is first pad made */
	if (first_pad == NULL)
	{
		first_pad = pad;
		last_pad = pad;
		
		accel_group = gtk_accel_group_new ();
	}
	else
	{
		last_pad->next = pad;
		last_pad = pad;
	}
	
	pad_alloc_gtk (pad);
	
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
	
	/* we need to especially set this widget because when toolbar was loaded, we didn't know lock value */
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_lock), (gboolean) pad->locked);
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_sticky), (gboolean) pad->sticky);
	
	pad_style_copy (&pad->style, &current_settings.style);
	pad_update_style (pad);
	
	gtk_window_set_position (pad->window, GTK_WIN_POS_MOUSE);
	pad->locked = 0;
	pad->sticky = 0;
	
	pad_set_title (pad);
	
	gtk_widget_show_all (pad->eventbox_outer);
	gtk_widget_show (pad->box);
	gtk_widget_show (GTK_WIDGET(pad->window));
/*	gtk_widget_grab_focus (GTK_WIDGET(get_text (pad->window))); */
	
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
	
	pad_set_sticky (pad, info->sticky);
	
	printf ("setting lock as %i, sticky as %i\n", pad->locked, pad->sticky);
	/* we need to especially set this widget because when toolbar was loaded, we didn't know lock value */
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_lock), (gboolean) pad->locked);
	
	pad_style_copy (&pad->style, &info->style);
	pad_update_style (pad);
	
	fio_open_pad_files (pad, FALSE);
	
	pad_set_title (pad);
	
	gtk_widget_show_all (pad->eventbox_outer);
	gtk_widget_show (pad->box);
	gtk_widget_show (GTK_WIDGET(pad->window));
/*	gtk_widget_grab_focus (GTK_WIDGET(get_text (pad->window))); */
	
	return pad;
}

static void
pad_renew (pad_node *pad)
{
	if (verbosity >= 2) printf ("Refreshing pad.\n");
	
	pad_alloc_gtk (pad);
	
	gtk_window_set_default_size (pad->window, pad->width, pad->height);
	gtk_window_move (pad->window, pad->x, pad->y);
	
	pad_fill_with_file (pad, pad->contentname);
	
	pad_set_sticky (pad, pad->sticky);
	
	pad_update_style (pad);
	
	pad_set_title (pad);
	
	gtk_widget_show_all (pad->eventbox_outer);
	gtk_widget_show (pad->box);
	gtk_widget_show (GTK_WIDGET(pad->window));
/*	gtk_widget_grab_focus (GTK_WIDGET(get_text (pad->window))); */
}
