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

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "toolbar.h"
#include "pad.h"
#include "pref.h"
#include "fio.h"
#include "help.h"
#include "tray.h"
#include "settings.h"
#include "properties.h"

pad_node *first_pad = NULL;
pad_node *last_pad = NULL;

static void
menuitem_cb (gpointer callback_data, guint callback_action, GtkWidget *widget);

static GtkItemFactoryEntry menu_items[] = 
{
	{N_("/_Pad"), 				NULL,			0,		0, 	"<Branch>"},
	{N_("/Pad/_New"),			"<control>N",		menuitem_cb, 	1,	"<StockItem>",	GTK_STOCK_NEW},
	{"/Pad/sep1", 				NULL,			0,		0,	"<Separator>"},
	{N_("/Pad/_Sticky"),			NULL,			menuitem_cb,	16,	"<CheckItem>"},
	{N_("/Pad/Proper_ties"), 		NULL,			menuitem_cb,	17,	"<StockItem>", GTK_STOCK_PROPERTIES},
	{"/Pad/sep2", 				NULL,			0,		0,	"<Separator>"},
	{N_("/Pad/_Close"),			"<control>W",		menuitem_cb,	4,	"<StockItem>",	GTK_STOCK_CLOSE},
	{N_("/Pad/_Delete"),			NULL,			menuitem_cb,	5,	"<StockItem>",	GTK_STOCK_DELETE},
/*	{N_("/File/_Quit"),			"<control>Q",		menuitem_cb,	6,	"<StockItem>",	GTK_STOCK_QUIT},*/
	{N_("/_Edit"),				NULL,			0,		0,	"<Branch>"},
	{N_("/Edit/C_ut"),			"<control>X",		menuitem_cb,	11,	"<StockItem>",	GTK_STOCK_CUT},
	{N_("/Edit/_Copy"),			"<control>C",		menuitem_cb,	12,	"<StockItem>",	GTK_STOCK_COPY},
	{N_("/Edit/_Paste"),			"<control>V",		menuitem_cb,	13,	"<StockItem>",	GTK_STOCK_PASTE},
	{"/Edit/sep",				NULL,			0,		0,	"<Separator>"},
	{N_("/Edit/Prefere_nces"),		NULL,			menuitem_cb,	7,	"<StockItem>",	GTK_STOCK_PREFERENCES},
	{N_("/_Notes"),				NULL,			0,		0,	"<Branch>"},
	{N_("/Notes/_Show All"),		NULL,			menuitem_cb,	10,	"<Item>"},
	{N_("/Notes/_Close All"),		"<control>Q",		menuitem_cb,	6,	"<StockItem>",	GTK_STOCK_QUIT},
	{"/Notes/sep",				NULL,			0,		0,	"<Separator>"},
	{N_("/_Help"),				NULL,			0,		0,	"<Branch>"},
	{N_("/Help/_Contents"),			"F1",			menuitem_cb,	8,	"<StockItem>",	GTK_STOCK_HELP},
	{N_("/Help/_About"),			NULL,			menuitem_cb,	9,	"<StockItem>",	GTK_STOCK_DIALOG_INFO}
};

#define SHOW_ACTION_OFFSET		10000

const toolbar_button buttons[] =
{
	{"New", "gtk-new", 0, G_CALLBACK (pad_new), N_("Open New Pad")},
	{"Close", "gtk-close", 0, G_CALLBACK (pad_close), N_("Close and Save Pad")},
	{"Delete", "gtk-delete", 0, G_CALLBACK (pad_confirm_destroy), N_("Delete Pad")},
	{"Clear", "gtk-clear", 0, G_CALLBACK (pad_clear), N_("Clear Pad Contents")},
	{"Preferences", "gtk-preferences", 0, G_CALLBACK (preferences_open), N_("Edit Global Preferences")},
	{"Properties", "gtk-properties", 0, G_CALLBACK (properties_open), N_("Edit Pad Properties")},
	{"Quit", "gtk-quit", 0, G_CALLBACK (gtk_main_quit), N_("Close All Pads")},
	{"Sticky", "xpad-sticky", 1, G_CALLBACK (pad_toggle_sticky), N_("Toggle Stickiness")},
	{"Minimize to Tray", "gtk-goto-bottom", 1, G_CALLBACK (tray_toggle), N_("Minimize Pads to System Tray")}
};

const char num_buttons = G_N_ELEMENTS (buttons);

static GtkAccelGroup *accel_group = NULL;

const toolbar_button *get_toolbar_button_by_func (GCallback func)
{
	gint i;
	
	for (i = 0; i < num_buttons; i++)
		if (buttons[i].func == func)
			return &buttons[i];
	
	return NULL;
}

const toolbar_button *get_toolbar_button_by_name (const gchar *name)
{
	gint i;
	
	for (i = 0; i < num_buttons; i++)
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


/* This function returns 'string' with all instances
   of 'obj' replaced with instances of 'replacement'
   It modifies string and re-allocs it.
   */
gchar *str_replace_tokens (gchar **string, gchar obj, gchar *replacement)
{
	gchar *p;
	gint rsize = strlen (replacement);
	gint osize = 1;
	gint diff = rsize - osize;
	
	p = *string;
	while ((p = strchr (p, obj)))
	{
		*string = g_realloc (*string, strlen (*string) + diff + 1);
		g_memmove (p + rsize, p + osize, strlen (p + osize) + 1);
		
		memcpy (p, replacement, rsize);
		
		p = p + rsize;
	}
	
	return *string;
}


/* since reshowing all pads presents them, caller
    param will be presented afterward */
void pad_set_decorations (pad_node *pad, gboolean decor)
{
	gtk_window_set_decorated (pad->window, decor);
	
	if (!pad->hidden)
	{
		gtk_widget_hide (GTK_WIDGET (pad->window));
		
		/* we move it so wm's know where to place it */
		gtk_window_move (pad->window, pad->x, pad->y);
		gtk_widget_show (GTK_WIDGET (pad->window));
	}
}

void pads_set_toolbar (gboolean toolbar)
{
	pad_node *temp;
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (toolbar)
		{
			pad_add_toolbar (temp);
			
			if (!xpad_settings_get_auto_hide_toolbar ())
				toolbar_show (temp);
		}
		else
			pad_remove_toolbar (temp);
	}
}

void pads_set_auto_hide_toolbar (gboolean auto_hide_toolbar)
{
	pad_node *temp;
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (auto_hide_toolbar)
		{
			toolbar_start_timeout (temp);	/* safe, since the cursor is unlikely to be on the pad? */
		}
		else
		{
			if (temp->toolbar->timeout)
				toolbar_end_timeout (temp);
			
			toolbar_show (temp);
		}
	}
}


void pad_set_editable (pad_node *pad, gboolean editable)
{
	GdkCursor *cursor;
	
	gtk_text_view_set_editable (get_text (pad->window), editable);
	gtk_text_view_set_cursor_visible (get_text (pad->window), editable);
	
	cursor = editable ? gdk_cursor_new (GDK_XTERM) : NULL;
	
	gdk_window_set_cursor (gtk_text_view_get_window (get_text (
		pad->window), GTK_TEXT_WINDOW_TEXT), cursor);
	
	if (cursor) gdk_cursor_unref (cursor);
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

	rv = strcmp (g_strstrip (content), "") == 0;
	
	g_free (content);
	
	return rv;
}

void pads_set_editable (gboolean editable)
{
	pad_node *temp;

	for (temp = first_pad; temp; temp = temp->next)
		pad_set_editable (temp, editable);
}

void pad_set_back_color (pad_node *pad, GdkColor *c)
{
	GtkWidget *text = GTK_WIDGET (get_text (pad->window));
	
	gtk_widget_modify_base (text, GTK_STATE_NORMAL, c);
	gtk_widget_modify_bg (text, GTK_STATE_NORMAL, c);
	
	if (pad->locked)
	{
		if (c)
		{
			pad->style.back = *c;
			pad->style.use_back = 1;
		}
		else
			pad->style.use_back = 0;
		
		fio_save_pad_info (pad);
	}
}

void pad_set_text_color (pad_node *pad, GdkColor *c)
{
	GtkWidget *text = GTK_WIDGET (get_text (pad->window));
	
	gtk_widget_modify_text (text, GTK_STATE_NORMAL, c);
	
	if (pad->locked)
	{
		if (c)
		{
			pad->style.text = *c;
			pad->style.use_text = 1;
		}
		else
			pad->style.use_text = 0;
		
		fio_save_pad_info (pad);
	}
}

void pad_set_border_color (pad_node *pad, GdkColor *c)
{
	gtk_widget_modify_bg (pad->eventbox_outer, GTK_STATE_NORMAL, c);
	
	if (pad->locked)
	{
		if (c)
			pad->style.border = *c;
		
		fio_save_pad_info (pad);
	}
}

void pad_set_padding (pad_node *pad, gint p)
{
	GtkWidget *text = GTK_WIDGET (get_text (pad->window));
	
	gtk_container_set_border_width (GTK_CONTAINER (text), p);
	
	if (pad->locked)
	{
		pad->style.padding = p;
		fio_save_pad_info (pad);
	}
}

void pad_set_border_width (pad_node *pad, gint w)
{
	gtk_container_set_border_width (GTK_CONTAINER (pad->eventbox), w);
	
	if (pad->locked)
	{
		pad->style.border_width = w;
		fio_save_pad_info (pad);
	}
}

void pad_set_fontname (pad_node *pad, const gchar *fontname)
{
	GtkWidget *text = GTK_WIDGET (get_text (pad->window));
	PangoFontDescription *fontdesc;
	
	fontdesc = fontname ? pango_font_description_from_string (fontname) :
		NULL;
	
	gtk_widget_modify_font (text, fontdesc);
	
	if (pad->locked)
	{
		pad->style.fontname = g_strdup (fontname);
		
		fio_save_pad_info (pad);
	}
}


static void pad_update_style (pad_node *pad)
{
	pad_style pstyle;
	GtkWidget *text, *outline;
	
	if (pad->locked)
		pstyle = pad->style;
	else
		pstyle = xpad_settings_get_style ();
	
	text = GTK_WIDGET (get_text (pad->window));
	outline = pad->eventbox_outer;
	
	gtk_widget_modify_base (text, GTK_STATE_NORMAL, 
		pstyle.use_back ? &pstyle.back : NULL);
	
	gtk_widget_modify_bg (text, GTK_STATE_NORMAL, &text->style->base[GTK_STATE_NORMAL]);
	
	gtk_widget_modify_text (text, GTK_STATE_NORMAL,
		pstyle.use_text ? &pstyle.text : NULL);
	
	gtk_widget_modify_font (text, pstyle.fontname ? 
		pango_font_description_from_string (pstyle.fontname) : NULL);
	
	
	gtk_widget_modify_bg (outline, GTK_STATE_NORMAL, &pstyle.border);
	
	
	gtk_container_set_border_width (GTK_CONTAINER (get_text (pad->window)), pstyle.padding);
	gtk_container_set_border_width (GTK_CONTAINER (pad->eventbox), pstyle.border_width);
	
	gtk_widget_queue_draw (GTK_WIDGET (pad->window));
	
	if (!pad->locked)
		pad_style_free (&pstyle);
	else
	{
		fio_save_pad_info (pad);
	}
}

void pad_style_copy (pad_style *dest, pad_style *source)
{
	dest->back = source->back;
	dest->text = source->text;
	dest->border = source->border;
	dest->use_back = source->use_back;
	dest->use_text = source->use_text;
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
	
	list = toolbar_get_buttons (pad->toolbar);
	
	for (tmp = list; tmp; tmp = tmp->next)
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
	}
	
	g_list_free (list);
}

/* sets a toggleable widget to a particular value */
static void
pad_toolbar_set_widget (pad_node *pad, GCallback target_func, gboolean value)
{
	GList *list, *tmp;
	GCallback func = NULL;
	GtkWidget *widget = NULL;
	
	if (target_func == NULL)
		return;
	
	list = toolbar_get_buttons (pad->toolbar);
	
	for (tmp = list; tmp; tmp = tmp->next)
	{
		widget = GTK_WIDGET (tmp->data);
		
		func = ((const toolbar_button *) g_object_get_data 
			(G_OBJECT (widget), "tb"))->func;
		
		if (func == target_func)
			break;
	}
	
	if (tmp)
	{
		/* Found target_func; func points at it now. */
		g_signal_handlers_block_by_func (widget, (gpointer) func, pad);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				value);
		g_signal_handlers_unblock_by_func (widget, (gpointer) func, pad);
	}
	
	g_list_free (list);
}

static void
pad_set_sticky (pad_node *pad, gboolean on)
{
	if (on) gtk_window_stick (pad->window);
	else    gtk_window_unstick (pad->window);
	
	pad->sticky = on;
	
	/* make sure the toolbar widget is up to date */
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_sticky), on);
}

void pad_toggle_sticky (pad_node *pad)
{
	pad_set_sticky (pad, pad->sticky ? 0 : 1);
	
	fio_save_pad_info (pad);
}

void pad_edit_cut_for_clipboard (pad_node *pad, GtkClipboard *clipboard)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	
	gtk_text_buffer_cut_clipboard (buf, clipboard, TRUE);
}

void pad_edit_cut (pad_node *pad)
{
	pad_edit_cut_for_clipboard (pad, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

void pad_edit_copy_for_clipboard (pad_node *pad, GtkClipboard *clipboard)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	
	gtk_text_buffer_copy_clipboard (buf, clipboard);
}

void pad_edit_copy (pad_node *pad)
{
	pad_edit_copy_for_clipboard (pad, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

void pad_edit_paste_for_clipboard (pad_node *pad, GtkClipboard *clipboard)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	
	gtk_text_buffer_paste_clipboard (buf, clipboard, NULL, TRUE);
}

void pad_edit_paste (pad_node *pad)
{
	pad_edit_paste_for_clipboard (pad, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}


static void quit_if_no_pads (void)
{
	pad_node *p;
	
	for (p = first_pad; p && p->hidden; p = p->next)
	{
		/* Find first non-hidden pad */
	}
	
	if (!p)
		pads_close_all ();
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
	properties_close (pad);
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
	gboolean do_destroy = TRUE;

	if (!pad_is_empty (pad) && xpad_settings_get_confirm_destroy ())
	{
		GtkWidget *dialog;
		
		dialog = xpad_alert_new (pad->window, GTK_STOCK_DIALOG_WARNING,
			_("Delete this pad?"),
			_("All text of this pad will be irrevocably lost."));
		
		if (!dialog)
			return FALSE;
		
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, 1, GTK_STOCK_DELETE, 2, NULL);
		
		do_destroy = gtk_dialog_run (GTK_DIALOG (dialog)) == 2;
		
		gtk_widget_destroy (dialog);
	}

	if (do_destroy)
		pad_destroy (pad);
	
	return do_destroy;
}

void pad_hide (pad_node *pad)
{
	if (verbosity >= 1) printf ("Closing pad [%s].\n", pad->infoname);
	
	toolbar_hide (pad);
	
	pad_free_gtk (pad);
	
	pad->hidden = TRUE;
}

void pad_close (pad_node *pad)
{
	pad_hide (pad);
	pad->closed = TRUE;
	
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
	pad->closed = FALSE;
	
	gtk_window_present (pad->window);
}

void pad_show_by_num (gint n)
{
	pad_node *temp;
	
	for (temp = first_pad; temp && (n > 1); temp = temp->next)
	  n--;

	if (temp)
		pad_show (temp);
	
	/* else, silently ignore */
}

void pads_hide_all (void)
{
	pad_node *temp = first_pad;
	
	while (temp)
	{
		if (temp->window)
			pad_hide (temp);
		
		temp = temp->next;
	}
}

void pads_unhide_all (void)
{
	PAD_ITERATE_START
	if (!PAD->closed)
	{
		pad_show (PAD);
	}
	PAD_ITERATE_END
}

void pads_show_all (void)
{
	PAD_ITERATE_START
	pad_show (PAD);
	PAD_ITERATE_END
}

void pad_show_all (pad_node *pad)
{
	pads_show_all ();
	pad_show (pad);
}

void pads_close_all (void)
{
	pad_node *temp;
	
	for (temp = first_pad; temp; temp = first_pad)
	{
		pad_remove (temp);
		pad_free (temp);
	}
	
	gtk_main_quit ();
}

static gboolean pad_window_destroyed (GtkWidget *window, pad_node *pad)
{
	switch (xpad_settings_get_wm_close ())
	{
	case XPAD_WM_CLOSE_ALL: /* close all */
		pads_close_all ();
		return TRUE;
		break;
	case XPAD_WM_CLOSE_PAD: /* close this pad */
		pad_close (pad);
		return TRUE;
		break;
	case XPAD_WM_DESTROY_PAD: /* delete this pad */
		pad_destroy (pad);
		return TRUE;
		break;
	}
	return FALSE;
}

void cleanup (void)
{
	pads_close_all();
	
	g_free (accel_group);
}


static gboolean pad_fill_with_file (pad_node *pad, const gchar *filename)
{
	gchar *contentbuf;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTextView *textbox = get_text (pad->window);
	
	contentbuf = fio_get_file (filename);
	if (!contentbuf)
		return FALSE;
	
	buffer = gtk_text_view_get_buffer (textbox);
	gtk_text_buffer_set_text (buffer, contentbuf, -1);
	g_free (contentbuf);
	
	gtk_text_buffer_get_start_iter (buffer, &iter);
	gtk_text_buffer_place_cursor (buffer, &iter);
	
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
	GtkWidget *dialog;
	gchar text [524];
	
	sprintf (text, _("You are using xpad %s with GTK+ %i.%i.%i."),
		VERSION, gtk_major_version, gtk_minor_version, gtk_micro_version);
	
	dialog = xpad_alert_new (pad->window, GTK_STOCK_DIALOG_INFO,
		text,
		_("Visit http://xpad.sourceforge.net for more information about xpad."));
	
	if (!dialog)
		return;
	
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, 1, NULL);
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	
	gtk_widget_destroy (dialog);
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
			if (verbosity >= 1)	fprintf(stderr, "Could not open new pad\n");
			return;
		}
	}
	
	if (!pad_fill_with_file(pad, filename) && newPad)
		pad_destroy (pad);	/* no need to open a new pad, if no content */
}

void pad_open_file (pad_node *pad)
{
	GtkWidget *filedialog;

	filedialog = gtk_file_selection_new (_("Open which file?"));

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

	filedialog = gtk_file_selection_new (_("Save as which file?"));

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
	if (xpad_settings_get_has_toolbar () &&
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
	if (xpad_settings_get_auto_hide_toolbar ())
	{
		/**
		 * Here we remove the toolbar.
		 */
		if (xpad_settings_get_has_toolbar () &&
			event->detail != GDK_NOTIFY_INFERIOR &&
			event->mode == GDK_CROSSING_NORMAL)
		{
		    toolbar_start_timeout (pad);
		}
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
	GtkWidget *tmp;
	
	if (pad->toolbar)
	{
		unblock_toolbar_events (pad);
		
		if (xpad_settings_get_auto_hide_toolbar ())
		{
			GdkRectangle rect;
			
			/**
			 * We must check if we disabled off of pad and start the timeout if so.
			 */
			gdk_window_get_pointer (GTK_WIDGET (pad->window)->window,
				&rect.x, &rect.y, NULL);
			
			rect.width = 1;
			rect.height = 1;
			
			if (!gtk_widget_intersect (GTK_WIDGET (pad->window), &rect, NULL))
				toolbar_start_timeout (pad);
		}
	}
	
	/* we must also re-enable menu widgets for cut/copy/paste, since we want the 
	 user to be able to cut/copy/paste */
	tmp = gtk_item_factory_get_item (pad->menu, _("/Edit/Cut"));
	gtk_widget_set_sensitive (tmp, TRUE);
	tmp = gtk_item_factory_get_item (pad->menu, _("/Edit/Copy"));
	gtk_widget_set_sensitive (tmp, TRUE);
	tmp = gtk_item_factory_get_item (pad->menu, _("/Edit/Paste"));
	gtk_widget_set_sensitive (tmp, TRUE);
}


static void
menuitem_cb (gpointer callback_data, guint callback_action, GtkWidget *widget)
{
	pad_node *pad;
	gboolean foundfocus = FALSE;
	
	/**
	 * Sigh...  If the user presses the keyboard accelerator for one of the 
	 * itemfactory entries, this function is run, but without a useful widget value.
	 * Thus, we have no way of finding out what pad to use.  So, what we do is 
	 * iterate over windows, finding the one with focus.
	 */
	for (pad = first_pad; pad; pad = pad->next)
	{
		if (!pad->hidden)
		{
			GtkWidget *w;
			
			w = gtk_window_get_focus (pad->window);
			if (!w) continue;
			foundfocus = GTK_WIDGET_HAS_FOCUS (w);
			
			if (foundfocus)
				break;
		}
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
		pads_close_all ();
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
	
	case 15:
		/* only can get here through the menu, so we can assume widget is valid */
		if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)))
			pad_lock_style (pad);
		else
			pad_unlock_style (pad);
		break;
	
	case 16:
		/* only can get here through the menu, so we can assume widget is valid */
		pad_set_sticky (pad, gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)));
		break;

	case 17:
		properties_open (pad);
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
	pad_node *p;
	GtkWidget *tmp;
	gint n = 0, i = SHOW_ACTION_OFFSET;
	GtkItemFactoryEntry entry;
	GtkClipboard *clipboard;
	GtkTextBuffer *buf;
	gboolean is_selection;
	const gchar *submenu = _("/Notes");
	
	/**
	 * Remove old items.
	 * Here's the deal:  There is no good way to iterate through item factory, since
	 * changes don't keep unless you use itemfactory's api.  This api does not allow
	 * iteration.  Thus, we use guessable action numbers for temporary items, like 10000+
	 */
	
	tmp = gtk_item_factory_get_item_by_action (pad->menu, i++);
	while (tmp)
	{
		gtk_item_factory_delete_item (pad->menu, 
				gtk_item_factory_path_from_widget (tmp));
		tmp = gtk_item_factory_get_item_by_action (pad->menu, i++);
	}
	
	/**
	 * Populate list of windows.
	 */
	for (p = first_pad; p; p = p->next)
	{
		gchar *title;
		gchar *tmp_title;
		
		n++;
		
		tmp_title = g_strdup (p->title);
		str_replace_tokens (&tmp_title, '_', "__");
		title = g_strdup_printf ("%s/_%i. %s", submenu, n, tmp_title);
		g_free (tmp_title);
		
		entry.path = title;
		entry.accelerator = NULL;
		entry.callback = menuitem_cb;
		entry.callback_action = SHOW_ACTION_OFFSET + n - 1;
		entry.item_type = "<Item>";
		
		gtk_item_factory_create_item (pad->menu, &entry, p, 1);
		
		g_free (title);
	}
	
	block_toolbar_events (pad);
	
	/* set checkboxes */
	tmp = gtk_item_factory_get_item (pad->menu, _("/Pad/Sticky"));
	GTK_CHECK_MENU_ITEM (tmp)->active = pad->sticky;
	
	/* setup copy/cut/paste sensitivity */
	buf = gtk_text_view_get_buffer (get_text (GTK_WINDOW (pad->window)));
	is_selection = gtk_text_buffer_get_selection_bounds (buf, NULL, NULL);
	tmp = gtk_item_factory_get_item (pad->menu, _("/Edit/Cut"));
	gtk_widget_set_sensitive (tmp, is_selection);
	tmp = gtk_item_factory_get_item (pad->menu, _("/Edit/Copy"));
	gtk_widget_set_sensitive (tmp, is_selection);
	
	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	tmp = gtk_item_factory_get_item (pad->menu, _("/Edit/Paste"));
	gtk_widget_set_sensitive (tmp, gtk_clipboard_wait_is_text_available (clipboard));
	
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
						(xpad_settings_get_edit_lock () && 
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
				if (xpad_settings_get_edit_lock () && pad_get_editable (pad) == FALSE) {
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
	pad->style = xpad_settings_get_style ();
	pad->locked = 1;
	
	/* make sure the toolbar widget is up to date */
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_lock), TRUE);
	
	fio_save_pad_info (pad);
}

void
pad_unlock_style (pad_node *pad)
{
	pad->locked = 0;
	pad_update_style (pad);
	
	/* make sure the toolbar widget is up to date */
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_lock), FALSE);
	
	fio_save_pad_info (pad);
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
	if (xpad_settings_get_edit_lock () && pad_get_editable (pad))
		pad_set_editable (pad, FALSE);
	
	return FALSE;
}

#if DRAWING_ON
static void
pad_background_refresh (pad_node *pad)
{
	GdkWindow *win;
	GdkRectangle rect = {0, 0, 0, 0};
	
	rect.width = pad->width;
	rect.height = pad->height;
	
	win = gtk_text_view_get_window (get_text (pad->window), GTK_TEXT_WINDOW_TEXT);
	
	gdk_window_invalidate_rect (win, &rect, FALSE);
}
#endif

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

#if DRAWING_ON
static void
pad_background_update (pad_node *pad)
{
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
}
#endif

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
	
	fio_save_pad_info (pad);
	
	return FALSE;
}

static void
pad_when_textbox_realized (GtkWidget *widget, pad_node *pad)
{
	/* set editable */
	pad_set_editable (pad, xpad_settings_get_edit_lock () == 0);
	
	pad_resize_background (pad);
	
	/* show the toolbar now that we are realized */
	if (xpad_settings_get_has_toolbar () && 
	    !xpad_settings_get_auto_hide_toolbar ())
		toolbar_show (pad);
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
	gchar *content, *end;
	
	buf = gtk_text_view_get_buffer (get_text (pad->window));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
	content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);
	
	end = g_utf8_strchr (content, -1, '\n');
	
	if (end)
	{
		end[0] = '\0';
	}
	
	g_free (pad->title);
	pad->title = g_strdup (content);
	
	gtk_window_set_title (pad->window, pad->title);
	
	g_free (content);
}


gboolean text_changed (GtkTextBuffer *buf, pad_node *pad)
{
	pad_set_title (pad);
	fio_save_pad_content (pad);
	
	return TRUE;
}

static void
pad_add_menu_items (pad_node *pad)
{
	GtkItemFactoryEntry e;
	int i;
	
	for (i = 0; i < G_N_ELEMENTS (menu_items); i++)
	{
		e = menu_items[i];
		
		e.path = _(menu_items[i].path);
		
		gtk_item_factory_create_item (pad->menu, &e, pad, 1);
	}
}

/**
 * Sometimes values get screwed up.  This is here for sanity checking.
 */
static void
normalize_dimensions (gint *x, gint *y, gint *width, gint *height)
{
#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION >= 2))
	GdkScreen *screen;
	gint screenw, screenh;
	
	screen = gdk_screen_get_default ();
	
	screenw = gdk_screen_get_width (screen);
	screenh = gdk_screen_get_height (screen);
	*width = MIN (*width, screenw);
	*height = MIN (*height, screenh);
	
	if (*x >= screenw)
		*x %= screenw;
	
	if (*y >= screenh)
		*y %= screenh;
#endif
}


static void
pad_alloc_gtk (pad_node *pad, const gchar *role)
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
	
	gtk_window_set_role (GTK_WINDOW (window), role);
	
	pad->window = GTK_WINDOW (window);
	pad->eventbox = eventbox;
	pad->eventbox_outer = eventbox1;
	pad->box = box;
	pad->toolbar = NULL;
	pad->scrollbar = scroll;
	pad->title = NULL;
	
	gtk_window_add_accel_group (pad->window, accel_group);
	pad->menu = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", accel_group);
	pad_add_menu_items (pad);
	g_signal_connect_swapped (G_OBJECT (gtk_item_factory_get_widget (pad->menu, "<main>")),
		"deactivate", G_CALLBACK (disable_popup_handler), pad);
	
	g_object_set_data (G_OBJECT (pad->menu), "pad", pad);
	
	gtk_window_set_gravity (GTK_WINDOW (window), GDK_GRAVITY_STATIC);
	
	if (xpad_settings_get_has_toolbar ())
		pad_add_toolbar (pad);
	
	/* set wm decorations */
	gtk_window_set_decorated (GTK_WINDOW(window), xpad_settings_get_has_decorations ());
	
	pad_set_scrollbars (pad, xpad_settings_get_has_scrollbar ());
	
	/* make sure that we save after pad is realized */
	g_signal_connect_after (textbox, "realize", G_CALLBACK 
		(pad_when_textbox_realized), pad);
	
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_UTILITY);
	
#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION >= 2))
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
#endif
	
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
	pad->infoname = NULL;
	pad->contentname = NULL;
	pad->properties = NULL;
	pad->closed = FALSE;
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
	
	return pad;
}

pad_node *pad_new (void)
{
	pad_node *pad;
	
	if (verbosity >= 2) printf ("Making new pad.\n");
	
	pad = start_pad ();
	
	fio_open_pad_files (pad, TRUE);
	
	pad_alloc_gtk (pad, pad->infoname);
	
	pad->width = xpad_settings_style_get_padding () + 
		xpad_settings_style_get_border_width () +
		xpad_settings_get_default_width ();
	pad->height = xpad_settings_style_get_padding () + 
		xpad_settings_style_get_border_width () +
		xpad_settings_get_default_height ();
	gtk_window_set_default_size (pad->window, pad->width, pad->height);
	
	pad->locked = 0;
	pad->sticky = 0;
	
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_lock), (gboolean) pad->locked);
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_sticky), (gboolean) pad->sticky);
	
	pad->style = xpad_settings_get_style ();
	pad_update_style (pad);
	
	gtk_window_set_position (pad->window, GTK_WIN_POS_MOUSE);
	
	pad_set_title (pad);
	
	gtk_widget_show_all (pad->eventbox_outer);
	gtk_widget_show (pad->box);
	gtk_widget_show (GTK_WIDGET(pad->window));
	
	return pad;
}

pad_node *pad_new_with_info (pad_info *info)
{
	pad_node *pad;
	
	if (verbosity >= 2) printf ("Making new pad with info.\n");
	
	pad = start_pad ();
	
	pad_alloc_gtk (pad, info->infoname);
	
	pad->infoname = info->infoname;
	pad->contentname = info->contentname;
	
	fio_open_pad_files (pad, FALSE);
	
	normalize_dimensions (&info->x, &info->y, &info->width, &info->height);
	gtk_window_set_default_size (pad->window, info->width, info->height);
	gtk_window_move (pad->window, info->x, info->y);
	
	pad_fill_with_file (pad, info->contentname);
	
	pad_set_title (pad);
	
	pad->locked = info->locked;
	pad->width = info->width;
	pad->height = info->height;
	pad->x = info->x;
	pad->y = info->y;
	
	pad_set_sticky (pad, info->sticky);
	
	/* we need to especially set this widget because when toolbar was loaded, we didn't know lock value */
	pad_toolbar_set_widget (pad, G_CALLBACK (pad_toggle_lock), (gboolean) pad->locked);
	
	pad_style_copy (&pad->style, &info->style);
	pad_update_style (pad);
	
	gtk_widget_show_all (pad->eventbox_outer);
	gtk_widget_show (pad->box);
	gtk_widget_show (GTK_WIDGET(pad->window));
	
	return pad;
}

static void
pad_renew (pad_node *pad)
{
	if (verbosity >= 2) printf ("Refreshing pad.\n");
	
	pad_alloc_gtk (pad, pad->infoname);
	
	gtk_window_set_default_size (pad->window, pad->width, pad->height);
	gtk_window_move (pad->window, pad->x, pad->y);
	
	pad_fill_with_file (pad, pad->contentname);
	
	pad_set_title (pad);
	
	pad_set_sticky (pad, pad->sticky);
	
	pad_update_style (pad);
	
	gtk_widget_show_all (pad->eventbox_outer);
	gtk_widget_show (pad->box);
	gtk_widget_show (GTK_WIDGET(pad->window));
}
