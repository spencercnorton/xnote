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
#include "pad.h"
#include "pref.h"
#include "fio.h"
#include "help.h"
#include "xpad-settings.h"
#include "properties.h"
#include "xpad-app.h"
#include "xpad-dashboard-frontend.h"
#include "xpad-tray.h"
#include "xpad-text-view.h"
#include "xpad-toolbar.h"

pad_node *first_pad = NULL;
pad_node *last_pad = NULL;

void pad_spawn (void);
void pad_edit_cut (pad_node *pad);
void pad_edit_copy (pad_node *pad);
void pad_edit_paste (pad_node *pad);
static void about_dialog (pad_node *pad);

GtkActionEntry pad_actions[] = 
{
	{"PadMenu", NULL, N_("_Pad"), NULL, NULL, NULL},
	{"EditMenu", NULL, N_("_Edit"), NULL, NULL, NULL},
	{"NotesMenu", NULL, N_("_Notes"), NULL, NULL, NULL},
	{"HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL},
	{"NewAction", GTK_STOCK_NEW, N_("_New"), "<control>N", N_("Create a new pad"), G_CALLBACK (menuitem_cb)},
	{"PreferencesAction", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL, N_("Edit xpad preferences"), G_CALLBACK (menuitem_cb)},
	{"CloseAllAction", GTK_STOCK_QUIT, N_("_Close All"), "<control>Q", N_("Close all pads"), G_CALLBACK (menuitem_cb)},
	{"ContentsAction", GTK_STOCK_HELP, N_("_Contents"), "F1", N_("Display information about using xpad"), G_CALLBACK (menuitem_cb)},
	{"PropertiesAction", GTK_STOCK_PROPERTIES, N_("Proper_ties"), NULL, N_("Edit pad properties"), G_CALLBACK (menuitem_cb)},
	{"CloseAction", GTK_STOCK_CLOSE, N_("_Close"), "<Control>W", N_("Close this pad"), G_CALLBACK (menuitem_cb)},
	{"DeleteAction", GTK_STOCK_DELETE, N_("_Delete"), NULL, N_("Delete this pad"), G_CALLBACK (menuitem_cb)},
	{"CutAction", GTK_STOCK_CUT, N_("C_ut"), "<Control>X", N_("Cut the selection"), G_CALLBACK (menuitem_cb)},
	{"CopyAction", GTK_STOCK_COPY, N_("_Copy"), "<Control>C", N_("Copy the selection"), G_CALLBACK (menuitem_cb)},
	{"PasteAction", GTK_STOCK_PASTE, N_("_Paste"), "<Control>V", N_("Paste the clipboard"), G_CALLBACK (menuitem_cb)},
	{"ShowAllAction", NULL, N_("_Show All"), NULL, N_("Show all existing pads"), G_CALLBACK (menuitem_cb)},
	{"AboutAction", GTK_STOCK_DIALOG_INFO, N_("_About"), NULL, N_("Display information about xpad"), G_CALLBACK (menuitem_cb)},
	{"BoldAction", GTK_STOCK_BOLD, N_("_Bold"), "<Control>B", N_("Bold the text"), G_CALLBACK (menuitem_cb)},
	{"ItalicAction", GTK_STOCK_ITALIC, N_("_Italic"), "<Control>I", N_("Italicize the text"), G_CALLBACK (menuitem_cb)},
	{"StrikethroughAction", GTK_STOCK_STRIKETHROUGH, N_("Stri_ke"), "<Control>K", N_("Strike the text"), G_CALLBACK (menuitem_cb)},
	{"BiggerAction", NULL, N_("Bi_gger"), NULL, N_("Make the text bigger"), G_CALLBACK (menuitem_cb)},
	{"SmallerAction", NULL, N_("S_maller"), NULL, N_("Make the text smaller"), G_CALLBACK (menuitem_cb)}
};

const gint num_pad_actions = G_N_ELEMENTS (pad_actions);

static GtkToggleActionEntry toggle_pad_actions[] = 
{
	{"StickyAction", "xpad-sticky", N_("_Sticky"), NULL, N_("Toggle stickiness"), G_CALLBACK (menuitem_cb), FALSE}
};

#define SHOW_ACTION_OFFSET		10000

#if 0
const toolbar_button buttons[] =
{
	{"New", "gtk-new", 0, G_CALLBACK (pad_spawn), N_("Open New Pad")},
	{"Close", "gtk-close", 0, G_CALLBACK (pad_close), N_("Close and Save Pad")},
	{"Delete", "gtk-delete", 0, G_CALLBACK (pad_confirm_destroy), N_("Delete Pad")},
	{"Clear", "gtk-clear", 0, G_CALLBACK (pad_clear), N_("Clear Pad Contents")},
	{"Preferences", "gtk-preferences", 0, G_CALLBACK (preferences_open), N_("Edit Global Preferences")},
	{"Properties", "gtk-properties", 0, G_CALLBACK (properties_open), N_("Edit Pad Properties")},
	{"Quit", "gtk-quit", 0, G_CALLBACK (pads_close_all), N_("Close All Pads")},
	{"Sticky", "xpad-sticky", 1, G_CALLBACK (pad_toggle_sticky), N_("Toggle Stickiness")}/*,
	{"Minimize to Tray", "gtk-goto-bottom", 1, G_CALLBACK (tray_toggle), N_("Minimize Pads to System Tray")}*/
};
#endif

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
	if (!pad->hidden)
	{
		gtk_widget_hide (GTK_WIDGET (pad->window));
		
		gtk_window_set_decorated (pad->window, decor);
		
		/* we move it so wm's know where to place it */
		gtk_window_move (pad->window, pad->x, pad->y);
		gtk_widget_show (GTK_WIDGET (pad->window));
	}
}

void pad_clear (pad_node *pad)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	
	gtk_text_buffer_set_text (buf, "", 0);
	
	pad_background_clear (pad);
}

static gboolean pad_is_empty (pad_node *pad)
{
	GtkTextIter s, e;
	GtkTextBuffer *buf;
	gchar *content;
	gboolean rv;
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
	content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);

	rv = strcmp (g_strstrip (content), "") == 0;
	
	g_free (content);
	
	return rv;
}

void pad_set_back_color (pad_node *pad, const GdkColor *c)
{
	gtk_widget_modify_base (pad->textview, GTK_STATE_NORMAL, c);
	
	if (!xpad_text_view_get_follow_global_style (XPAD_TEXT_VIEW (pad->textview)))
	{
		fio_save_pad_info (pad);
	}
}

void pad_set_text_color (pad_node *pad, const GdkColor *c)
{
	gtk_widget_modify_text (pad->textview, GTK_STATE_NORMAL, c);
	
	if (!xpad_text_view_get_follow_global_style (XPAD_TEXT_VIEW (pad->textview)))
	{
		fio_save_pad_info (pad);
	}
}

void pad_set_fontname (pad_node *pad, const gchar *fontname)
{
	PangoFontDescription *fontdesc;
	
	fontdesc = fontname ? pango_font_description_from_string (fontname) : NULL;
	
	gtk_widget_modify_font (pad->textview, fontdesc);
	
	if (!xpad_text_view_get_follow_global_style (XPAD_TEXT_VIEW (pad->textview)))
	{
		fio_save_pad_info (pad);
	}
}

static void
pad_set_sticky (pad_node *pad, gboolean on)
{
	if (on) gtk_window_stick (pad->window);
	else    gtk_window_unstick (pad->window);
}

void pad_toggle_sticky (pad_node *pad)
{
	pad_set_sticky (pad, pad->sticky ? FALSE : TRUE);
	
	fio_save_pad_info (pad);
}

void pad_edit_cut_for_clipboard (pad_node *pad, GtkClipboard *clipboard)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	
	gtk_text_buffer_cut_clipboard (buf, clipboard, TRUE);
}

void pad_edit_cut (pad_node *pad)
{
	pad_edit_cut_for_clipboard (pad, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

void pad_edit_copy_for_clipboard (pad_node *pad, GtkClipboard *clipboard)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	
	gtk_text_buffer_copy_clipboard (buf, clipboard);
}

void pad_edit_copy (pad_node *pad)
{
	pad_edit_copy_for_clipboard (pad, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

void pad_edit_paste_for_clipboard (pad_node *pad, GtkClipboard *clipboard)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	
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
	
	/* Don't shut down if a pad or the tray is open */
	if (!p && !xpad_tray_is_open ())
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

static gboolean pad_window_destroyed (GtkWidget *window, GdkEvent *event, pad_node *pad);
static void pad_renew (pad_node *pad, gboolean move);

static void
pad_free_gtk (pad_node *pad)
{
	/* have we already freed gtk stuff? */
	if (!pad->window)
		return;
	pad_remove_toolbar (pad);
	properties_close (pad);
	gtk_widget_destroy (GTK_WIDGET (pad->window));
	g_object_unref (pad->ui_manager);
	if (pad->popup_notes_actions)
		g_object_unref (pad->popup_notes_actions);
	
	pad->window = NULL;
}

static void pad_free (pad_node *pad)
{
	pad_free_gtk (pad);
	g_free (pad->contentname);
	g_free (pad->infoname);
	g_free (pad);
}

void pad_destroy (pad_node *pad)
{
	fio_remove_pad_files (pad);
	pad_remove (pad);

	pad_free (pad);

	quit_if_no_pads ();
}

/* returns true if pad destroyed */
gboolean pad_confirm_destroy (pad_node *pad)
{
	gboolean do_destroy = TRUE;

	if (!pad_is_empty (pad) && xpad_settings_get_confirm_destroy (xpad_settings ()))
	{
		GtkWidget *dialog;
		
		dialog = xpad_app_alert_new (pad->window, GTK_STOCK_DIALOG_WARNING,
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
	pad->hidden = TRUE;
	
	fio_save_pad_info (pad);
	
	pad_free_gtk (pad);
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
		pad_renew (pad, TRUE);
		gtk_widget_show_all (GTK_WIDGET (pad->window));
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

void pads_show_all (void)
{
	PAD_ITERATE_START
	pad_show (PAD);
	PAD_ITERATE_END
}

void pad_show_all (pad_node *pad)
{
	pads_show_all ();
	if (pad)
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

static gboolean pad_window_destroyed (GtkWidget *window, GdkEvent *event, pad_node *pad)
{
	pad_close (pad);
	return TRUE;
}

void cleanup (void)
{
	pads_close_all();
}


static gboolean pad_fill_with_file (pad_node *pad, const gchar *filename)
{
	gchar *contentbuf;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	
	contentbuf = fio_get_file (filename);
	if (!contentbuf)
		return FALSE;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
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
	gchar *text;
	
	text = g_strdup_printf (_("You are using xpad %s with GTK+ %i.%i.%i."),
		VERSION, gtk_major_version, gtk_minor_version, gtk_micro_version);
	
	dialog = xpad_app_alert_new (pad->window, GTK_STOCK_DIALOG_INFO,
		text,
		_("Visit http://xpad.sourceforge.net for more information about xpad."));
	g_free (text);
	
	if (!dialog)
		return;
	
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, 1, NULL);
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	
	gtk_widget_destroy (dialog);
}

static gboolean
enter_handler (GtkWidget *widget, GdkEventCrossing *event, pad_node *pad)
{
	/**
	 * Here we add a toolbar so the user has quick access to several features.
	 */
	if (xpad_settings_get_has_toolbar (xpad_settings ()) &&
		event->detail != GDK_NOTIFY_INFERIOR)
	{
		/*if (pad->toolbar->timeout)
			toolbar_end_timeout (pad);
		
		gtk_widget_show (pad->toolbar);*/
	}
	
	return FALSE;
}

static gboolean
leave_handler (GtkWidget *widget, GdkEventCrossing *event, pad_node *pad)
{
	if (xpad_settings_get_autohide_toolbar (xpad_settings ()))
	{
		/**
		 * Here we remove the toolbar.
		 */
		if (xpad_settings_get_has_toolbar (xpad_settings ()) &&
			event->detail != GDK_NOTIFY_INFERIOR &&
			event->mode == GDK_CROSSING_NORMAL)
		{
		    /*toolbar_start_timeout (pad);*/
		}
	}
	
	return FALSE;
}

static void
block_toolbar_events (pad_node *pad)
{
	if (pad->toolbar)
	{
		/*if (pad->toolbar->timeout)
			toolbar_end_timeout (pad);*/
		
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
		/*if (pad->toolbar->timeout)
			toolbar_end_timeout (pad);*/
		
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
		
		if (xpad_settings_get_autohide_toolbar (xpad_settings ()))
		{
			GdkRectangle rect;
			
			/**
			 * We must check if we disabled off of pad and start the timeout if so.
			 */
			gdk_window_get_pointer (GTK_WIDGET (pad->window)->window,
				&rect.x, &rect.y, NULL);
			
			rect.width = 1;
			rect.height = 1;
			
			/*if (!gtk_widget_intersect (GTK_WIDGET (pad->window), &rect, NULL))
				toolbar_start_timeout (pad);*/
		}
	}
	
	/* we must also re-enable menu widgets for cut/copy/paste, since we want the 
	 user to be able to cut/copy/paste */
	tmp = gtk_ui_manager_get_widget (pad->ui_manager, "/HighlightPopupItem/PasteItem");
	gtk_widget_set_sensitive (tmp, TRUE);
	tmp = gtk_ui_manager_get_widget (pad->ui_manager, "/PopupItem/EditItem/PasteItem");
	gtk_widget_set_sensitive (tmp, TRUE);
}

void pad_toggle_tag (pad_node *pad, gchar *name)
{
	GtkTextBuffer *buffer;
	GtkTextTagTable *table;
	GtkTextTag *tag;
	GtkTextIter start, end, i;
	gboolean all_tagged;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	table = gtk_text_buffer_get_tag_table (buffer);
	tag = gtk_text_tag_table_lookup (table, name);
	gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
	
	if (!tag)
		return;
	
	for (all_tagged = TRUE, i = start; !gtk_text_iter_equal (&i, &end); gtk_text_iter_forward_char (&i))
	{
		if (!gtk_text_iter_has_tag (&i, tag))
		{
			all_tagged = FALSE;
			break;
		}
	}
	
	if (all_tagged)
		gtk_text_buffer_remove_tag (buffer, tag, &start, &end);
	else
		gtk_text_buffer_apply_tag (buffer, tag, &start, &end);
}

void pad_adjust_tag_scale (pad_node *pad, gboolean bigger)
{
	GtkTextBuffer *buffer;
	GtkTextTagTable *table;
	GtkTextIter start, end, i;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	table = gtk_text_buffer_get_tag_table (buffer);
	gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
	
	for (i = start; !gtk_text_iter_equal (&i, &end); gtk_text_iter_forward_char (&i))
	{
		GSList *tags = gtk_text_iter_get_tags (&i), *j;
		gboolean sets_scale = FALSE;
		gdouble scale;
		GtkTextTag *tag;
		GtkTextIter next;
		
		next = i;
		gtk_text_iter_forward_char (&next);
		
		for (j = tags; j; j = j->next)
		{
			tag = (GtkTextTag *) j->data;
			
			g_object_get (tag, "scale-set", &sets_scale, NULL);
			if (sets_scale)
			{
				g_object_get (tag, "scale", &scale, NULL);
				gtk_text_buffer_remove_tag (buffer, tag, &i, &next);
				break;
			}
		}
		
		if (!sets_scale)
			scale = 1.0;
		/* This is subject to rounding errors... */
		scale *= (bigger ? 1.2 : (1/1.2));
		tag = gtk_text_buffer_create_tag (buffer, NULL, "scale", scale, NULL);
		gtk_text_buffer_apply_tag (buffer, tag, &i, &next);
		
		g_slist_free (tags);
	}
}

void
menuitem_cb (GtkAction *action, gpointer user_data)
{
	pad_node *pad = (pad_node *) user_data;
	const gchar *action_name = gtk_action_get_name (action);
	
	if (strcmp (action_name, "NewAction") == 0) {
		pad_node *newpad = pad_new ();
		xpad_pad_group_add (xpad_app_get_pad_group (), newpad);
	}
	else if (strcmp (action_name, "CloseAction") == 0) {
		pad_close (pad);
	}
	else if (strcmp (action_name, "DeleteAction") == 0) {
		pad_confirm_destroy (pad);
	}
	else if (strcmp (action_name, "CloseAllAction") == 0) {
		pads_close_all ();
	}
	else if (strcmp (action_name, "PreferencesAction") == 0) {
		preferences_open (pad);
	}
	else if (strcmp (action_name, "ContentsAction") == 0) {
		show_help ();
	}
	else if (strcmp (action_name, "AboutAction") == 0) {
		about_dialog (pad);
	}
	else if (strcmp (action_name, "ShowAllAction") == 0) {
		pad_show_all (pad);
	}
	else if (strcmp (action_name, "CutAction") == 0) {
		pad_edit_cut (pad);
	}
	else if (strcmp (action_name, "CopyAction") == 0) {
		pad_edit_copy (pad);
	}
	else if (strcmp (action_name, "PasteAction") == 0) {
		pad_edit_paste (pad);
	}
	else if (strcmp (action_name, "StickyAction") == 0) {
		GtkToggleAction *toggle_action = GTK_TOGGLE_ACTION (action);
		
		pad_set_sticky (pad, gtk_toggle_action_get_active (toggle_action));
	}
	else if (strcmp (action_name, "PropertiesAction") == 0) {
		properties_open (pad);
	}
	else if (g_str_has_prefix (action_name, "ShowNoteAction-")) {
		pad_show (pad);
	}
	else if (strcmp (action_name, "BoldAction") == 0) {
		pad_toggle_tag (pad, "bold");
	}
	else if (strcmp (action_name, "ItalicAction") == 0) {
		pad_toggle_tag (pad, "italic");
	}
	else if (strcmp (action_name, "StrikethroughAction") == 0) {
		pad_toggle_tag (pad, "strikethrough");
	}
	else if (strcmp (action_name, "BiggerAction") == 0) {
		pad_adjust_tag_scale (pad, TRUE);
	}
	else if (strcmp (action_name, "SmallerAction") == 0) {
		pad_adjust_tag_scale (pad, FALSE);
	}
}

static gchar *create_popup_ui (int num_pads)
{
	gchar *ui, *new_ui;
	gint i;
	
	ui = g_strdup ("<ui><popup name='PopupItem'><menu name='NotesItem' action='NotesMenu'><placeholder name='NotesListItem'>");
	
	for (i = 1; i <= num_pads; i++) {
		new_ui = g_strdup_printf ("%s<menuitem name='ShowNoteItem-%i' action='ShowNoteAction-%i'/>", ui, i, i);
		g_free (ui);
		ui = new_ui;
	}
	
	new_ui = g_strdup_printf ("%s</placeholder></menu></popup></ui>", ui);
	g_free (ui);
	ui = new_ui;
	
	return ui;
}

static gint pad_title_compare (pad_node *a, pad_node *b)
{
	gchar *title_a = g_utf8_casefold (a->title, -1);
	gchar *title_b = g_utf8_casefold (b->title, -1);
	
	gint rv = g_utf8_collate (title_a, title_b);
	
	g_free (title_a);
	g_free (title_b);
	
	return rv;
}

static void pad_popup_no_highlight (pad_node *pad, GdkEventButton *event)
{
	pad_node *p;
	GtkWidget *tmp;
	gint n = 0;
	GtkClipboard *clipboard;
	GList *pads = NULL, *l;
	gchar *ui;
	
	if (pad->popup_notes_actions) {
		gtk_ui_manager_remove_ui (pad->ui_manager, pad->popup_notes_merge_id);
		gtk_ui_manager_remove_action_group (pad->ui_manager, pad->popup_notes_actions);
		g_free (pad->popup_notes_actions);
	}
	
	pad->popup_notes_actions = gtk_action_group_new (PACKAGE "-notes");
	
	/**
	 * Order pads according to title.
	 */
	for (p = first_pad; p; p = p->next)
	{
		pads = g_list_insert_sorted (pads, p, (GCompareFunc) pad_title_compare);
	}
	
	/**
	 * Populate list of windows.
	 */
	for (l = pads, n = 1; l; l = l->next, n++)
	{
		gchar *title;
		gchar *tmp_title;
		gchar *action_name;
		GtkActionEntry entry;
		
		tmp_title = g_strdup (((pad_node *) l->data)->title);
		str_replace_tokens (&tmp_title, '_', "__");
		if (n < 10)
			title = g_strdup_printf ("_%i. %s", n, tmp_title);
		else
			title = g_strdup (tmp_title);
		g_free (tmp_title);
		
		action_name = g_strdup_printf ("ShowNoteAction-%i", n);
		
		entry.name = action_name;
		entry.stock_id = NULL;
		entry.label = title;
		entry.accelerator = NULL;
		entry.tooltip = NULL;
		entry.callback = G_CALLBACK (menuitem_cb);
		
		gtk_action_group_add_actions (pad->popup_notes_actions, &entry, 1, l->data);
		
		g_free (title);
		g_free (action_name);
	}
	g_list_free (pads);
	
	ui = create_popup_ui (n - 1);
	pad->popup_notes_merge_id = gtk_ui_manager_add_ui_from_string (pad->ui_manager, ui, -1, NULL);
	g_free (ui);
	
	gtk_ui_manager_insert_action_group (pad->ui_manager, pad->popup_notes_actions, 0);
	
	block_toolbar_events (pad);
	
	/* set checkboxes */
	tmp = gtk_ui_manager_get_widget (pad->ui_manager, "/PopupItem/PadItem/StickyItem");
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tmp), pad->sticky);
	
	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	tmp = gtk_ui_manager_get_widget (pad->ui_manager, "/PopupItem/EditItem/PasteItem");
	gtk_widget_set_sensitive (tmp, gtk_clipboard_wait_is_text_available (clipboard));
	
	tmp = gtk_ui_manager_get_widget (pad->ui_manager, "/PopupItem");
	gtk_menu_popup (GTK_MENU (tmp), NULL, NULL, NULL, NULL, event->button, event->time);
}

static void pad_popup_highlight (pad_node *pad, GdkEventButton *event)
{
	GtkClipboard *clipboard;
	GtkWidget *tmp;
	
	block_toolbar_events (pad);
	
	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	tmp = gtk_ui_manager_get_widget (pad->ui_manager, "/HighlightPopupItem/PasteItem");
	gtk_widget_set_sensitive (tmp, gtk_clipboard_wait_is_text_available (clipboard));
	
	tmp = gtk_ui_manager_get_widget (pad->ui_manager, "/HighlightPopupItem");
	gtk_menu_popup (GTK_MENU (tmp), NULL, NULL, NULL, NULL, event->button, event->time);
}

static void pad_popup (pad_node *pad, GdkEventButton *event)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	
	if (gtk_text_buffer_get_selection_bounds (buf, NULL, NULL))
		pad_popup_highlight (pad, event);
	else
		pad_popup_no_highlight (pad, event);
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
						(xpad_settings_get_edit_lock (xpad_settings ()) && 
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
	xpad_text_view_set_follow_global_style (XPAD_TEXT_VIEW (pad->textview), FALSE);
	
	fio_save_pad_info (pad);
}

void
pad_unlock_style (pad_node *pad)
{
	xpad_text_view_set_follow_global_style (XPAD_TEXT_VIEW (pad->textview), TRUE);
	
	fio_save_pad_info (pad);
}

void
pad_toggle_lock (pad_node *pad)
{
	if (xpad_text_view_get_follow_global_style (XPAD_TEXT_VIEW (pad->textview)))
		pad_lock_style (pad);
	else
		pad_unlock_style (pad);
}


#if DRAWING_ON
static void
pad_background_refresh (pad_node *pad)
{
	GdkWindow *win;
	GdkRectangle rect = {0, 0, 0, 0};
	
	rect.width = pad->width;
	rect.height = pad->height;
	
	win = gtk_text_view_get_window (GTK_TEXT_VIEW (pad->textview), GTK_TEXT_WINDOW_TEXT);
	
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
	
	textbox = pad->textview;
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
	
	textbox = pad->textview;
	textwin = gtk_text_view_get_window (GTK_TEXT_VIEW (textbox), GTK_TEXT_WINDOW_TEXT);
	
	ha = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	va = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	
	g_print ("making %i, %i, from %i, %i\n", pad->width, pad->height, (int)ha->upper, (int)va->upper);
	
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
	
	textbox = pad->textview;
	textwin = gtk_text_view_get_window (GTK_TEXT_VIEW (textbox), GTK_TEXT_WINDOW_TEXT);
	
	ha = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	va = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar));
	
	width = ha->upper;
	height = va->upper;
	pix = gdk_pixmap_new (textwin, width, height, -1);
	
	g_print ("background is %i, %i\n", width, height);
	
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
	g_print ("v changed\n");
	
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
		g_print ("v changed for real\n");
		if (GTK_WIDGET_REALIZED (pad->textview))
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
	
	g_print ("h changed\n");
	
	if (!pad->background)
	{
		doit = TRUE;
	}
	
	if (!doit)
	{
		g_print ("drawable exists\n");
		gdk_drawable_get_size (pad->background, &w, NULL);
		
		if (w != adjustment->upper)
			doit = TRUE;
	}
	
	if (doit)
	{
		g_print ("v changed for real\n");
		if (GTK_WIDGET_REALIZED (pad->textview))
			pad_resize_background (pad);
	}
#endif
}

void pad_background_clear (pad_node *pad)
{
#if DRAWING_ON
	gint w, h;
	
	gdk_drawable_get_size (pad->background, &w, &h);
	gdk_draw_rectangle (pad->background, pad->textview->style->bg_gc[GTK_STATE_NORMAL], 1, 0, 0, w, h);
	
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
	pad_resize_background (pad);
	
	/* show the toolbar now that we are realized */
	if (xpad_settings_get_has_toolbar (xpad_settings ()) && 
	    !xpad_settings_get_autohide_toolbar (xpad_settings ()))
		gtk_widget_show (pad->toolbar);
}
/*
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
}*/

void
pad_remove_toolbar (pad_node *pad)
{
	if (pad->toolbar)
	{
		disconnect_toolbar_events (pad);
		
		gtk_widget_hide (pad->toolbar);
		gtk_widget_destroy (pad->toolbar);
		pad->toolbar = NULL;
	}
}

void
pad_add_toolbar (pad_node *pad)
{
	if (pad->toolbar)
		return;
	
	pad->toolbar = xpad_toolbar_new ();
	
	gtk_box_pack_start (GTK_BOX (pad->box), pad->toolbar, FALSE, FALSE, 0);
	
	g_signal_connect_swapped (pad->toolbar, "activate-new", G_CALLBACK (pad_spawn), pad);
	g_signal_connect_swapped (pad->toolbar, "activate-clear", G_CALLBACK (pad_clear), pad);
	g_signal_connect_swapped (pad->toolbar, "activate-close", G_CALLBACK (pad_close), pad);
	g_signal_connect_swapped (pad->toolbar, "activate-delete", G_CALLBACK (pad_confirm_destroy), pad);
	g_signal_connect_swapped (pad->toolbar, "activate-properties", G_CALLBACK (properties_open), pad);
	g_signal_connect_swapped (pad->toolbar, "activate-preferences", G_CALLBACK (preferences_open), pad);
	g_signal_connect_swapped (pad->toolbar, "activate-quit", G_CALLBACK (pads_close_all), pad);
	g_signal_connect_swapped (pad->toolbar, "activate-sticky", G_CALLBACK (pad_toggle_sticky), pad);
	
	/*g_signal_connect (G_OBJECT (pad->toolbar->grip), "button-press-event", 
		G_CALLBACK (grip_press_handler), pad);*/
	
	connect_toolbar_events (pad);
	
	gtk_widget_show (pad->toolbar);
}

void pad_set_title (pad_node *pad)
{
	GtkTextBuffer *buf;
	GtkTextIter s, e;
	gchar *content, *end;
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
	content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);
	
	end = g_utf8_strchr (content, -1, '\n');
	
	if (end)
	{
		end[0] = '\0';
	}
	
	g_free (pad->title);
	pad->title = g_strstrip (g_strdup (content));
	g_free (content);
	
	gtk_window_set_title (pad->window, pad->title);
	
	if (pad->properties)
	{
		gchar *properties_title = g_strdup_printf (_("%s Properties"), pad->title);
		gtk_window_set_title (GTK_WINDOW (pad->properties), properties_title);
		g_free (properties_title);
	}
}


gboolean text_changed (GtkTextBuffer *buf, pad_node *pad)
{
	pad_set_title (pad);
	fio_save_pad_content (pad);
	
	return TRUE;
}


/**
 * Sometimes values get screwed up.  This is here for sanity checking.
 */
static void
normalize_dimensions (gint *x, gint *y, gint *width, gint *height)
{
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
}

static void
pad_notify_has_decorations (pad_node *pad)
{
	pad_set_decorations (pad, xpad_settings_get_has_decorations (xpad_settings ()));
}

static void
pad_notify_has_toolbar (pad_node *pad)
{
	if (xpad_settings_get_has_toolbar (xpad_settings ()))
	{
		pad_add_toolbar (pad);
		
		if (!xpad_settings_get_autohide_toolbar (xpad_settings ()))
			gtk_widget_show (pad->toolbar);
	}
	else
		pad_remove_toolbar (pad);
}

static void
pad_notify_autohide_toolbar (pad_node *pad)
{
	if (xpad_settings_get_autohide_toolbar (xpad_settings ()))
	{
		/*toolbar_start_timeout (pad);*/	/* safe, since the cursor is unlikely to be on the pad? */
	}
	else
	{
		/*if (pad->toolbar->timeout)
			toolbar_end_timeout (pad);*/
		
		gtk_widget_show (pad->toolbar);
	}
}

static void
pad_notify_has_scrollbar (pad_node *pad)
{
	if (xpad_settings_get_has_scrollbar (xpad_settings ()))
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

static gboolean
pad_window_state_event (GtkWidget *widget, GdkEventWindowState *event, pad_node *pad)
{
	g_print ("start window state event\n");
	pad->sticky = (event->new_window_state & GDK_WINDOW_STATE_STICKY) ? TRUE : FALSE;
	g_print ("sticky is now %i\n", pad->sticky);
	
	if (pad->toolbar)
	{
		xpad_toolbar_set_sticky_active (XPAD_TOOLBAR (pad->toolbar), pad->sticky);
	}
	
	g_print ("end window state event\n");
	return FALSE;
}

static void
pad_alloc_gtk (pad_node *pad, const gchar *role)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *textbox = xpad_text_view_new ();
	GtkTextBuffer *textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textbox));
	GtkWidget *box = gtk_vbox_new (FALSE, 0);
	GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
	GtkActionGroup *actions;
	gchar *ui_filename;
	
	/* set up scrollbar */
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
		GTK_SHADOW_NONE);
	
	/* add textbox to window */
	gtk_container_add (GTK_CONTAINER (scroll), textbox);
	gtk_box_pack_start (GTK_BOX (box), scroll, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (window), box);
	textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textbox));
	
	/* We want to make xpad moveable anywhere a lower widget doesn't have priority */
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK | GDK_PROPERTY_CHANGE_MASK);
	g_signal_connect (window, "button-press-event", G_CALLBACK (window_button_handler), pad);
	g_signal_connect (textbox, "event", G_CALLBACK (textbox_event_handler), pad);
	g_signal_connect (window, "delete-event", G_CALLBACK (pad_window_destroyed), pad);
	g_signal_connect (window, "configure-event", G_CALLBACK (pad_save_location), pad);
	g_signal_connect (textbuf, "end-user-action", G_CALLBACK (text_changed), pad);
	g_signal_connect (window, "window-state-event", G_CALLBACK (pad_window_state_event), pad);
	g_signal_connect_swapped (xpad_settings (), "notify::has-decorations", G_CALLBACK (pad_notify_has_decorations), pad);
	g_signal_connect_swapped (xpad_settings (), "notify::has-toolbar", G_CALLBACK (pad_notify_has_toolbar), pad);
	g_signal_connect_swapped (xpad_settings (), "notify::autohide-toolbar", G_CALLBACK (pad_notify_autohide_toolbar), pad);
	g_signal_connect_swapped (xpad_settings (), "notify::has-scrollbar", G_CALLBACK (pad_notify_has_scrollbar), pad);
	
	g_object_set_data (G_OBJECT (window), "pad", pad);
	g_object_set_data (G_OBJECT (window), "textbox", textbox);
	
	gtk_window_set_role (GTK_WINDOW (window), role);
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_UTILITY);
	
	pad->window = GTK_WINDOW (window);
	pad->box = box;
	pad->toolbar = NULL;
	pad->scrollbar = scroll;
	pad->textview = textbox;
	pad->title = NULL;
	pad->popup_notes_actions = NULL;
	pad->popup_notes_merge_id = 0;
	
	/* set up action group */
	actions = gtk_action_group_new (PACKAGE);
	gtk_action_group_add_actions (actions, pad_actions, G_N_ELEMENTS (pad_actions), pad);
	gtk_action_group_add_toggle_actions (actions, toggle_pad_actions, G_N_ELEMENTS (toggle_pad_actions), pad);
	gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);
	
	/* set up ui manager */
	ui_filename = g_build_filename (PKGDATADIR, "xpad.ui", NULL);
	pad->ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (pad->ui_manager, actions, 0);
	gtk_ui_manager_add_ui_from_file (pad->ui_manager, ui_filename, NULL);
	gtk_window_add_accel_group (pad->window, gtk_ui_manager_get_accel_group (pad->ui_manager));
	g_object_unref (actions);
	gtk_ui_manager_ensure_update (pad->ui_manager);
	g_free (ui_filename);
	
	g_signal_connect_swapped (G_OBJECT (gtk_ui_manager_get_widget (pad->ui_manager, "/PopupItem")), "deactivate", G_CALLBACK (disable_popup_handler), pad);
	g_signal_connect_swapped (G_OBJECT (gtk_ui_manager_get_widget (pad->ui_manager, "/HighlightPopupItem")), "deactivate", G_CALLBACK (disable_popup_handler), pad);
	
	gtk_window_set_gravity (GTK_WINDOW (window), GDK_GRAVITY_STATIC);
	
	pad_notify_has_toolbar (pad);
	pad_notify_has_decorations (pad);
	pad_notify_has_scrollbar (pad);
	
	/* make sure that we save after pad is realized */
	g_signal_connect_after (textbox, "realize", G_CALLBACK 
		(pad_when_textbox_realized), pad);
	
	g_signal_connect (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar)), "value-changed", G_CALLBACK (pad_scrolled), pad);
	g_signal_connect (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar)), "value-changed", G_CALLBACK (pad_scrolled), pad);
	g_signal_connect (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar)), "changed", G_CALLBACK (pad_v_scroll_changed), pad);
	g_signal_connect (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->scrollbar)), "changed", G_CALLBACK (pad_h_scroll_changed), pad);
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
	pad->toolbar = NULL;
	pad->window = NULL;
	
	/* check if this is first pad made */
	if (first_pad == NULL)
	{
		first_pad = pad;
		last_pad = pad;
	}
	else
	{
		last_pad->next = pad;
		last_pad = pad;
	}
	
	return pad;
}

void pad_spawn (void)
{
	pad_node *pad = pad_new ();
	xpad_pad_group_add (xpad_app_get_pad_group (), pad);
}

pad_node *pad_new (void)
{
	pad_node *pad;
	
	pad = start_pad ();
	
	fio_open_pad_files (pad, TRUE);
	
	pad->sticky = FALSE;
	pad->width = xpad_settings_get_width (xpad_settings ());
	pad->height = xpad_settings_get_height (xpad_settings ());
	pad->hidden = FALSE;
	
	pad_renew (pad, FALSE);
	
	pad_set_sticky (pad, xpad_settings_get_sticky (xpad_settings ()));
	
	gtk_widget_show_all (GTK_WIDGET (pad->window));
	
	return pad;
}

pad_node *pad_new_with_info (pad_info *info)
{
	pad_node *pad;
	
	pad = start_pad ();
	
	pad->infoname = info->infoname;
	pad->contentname = info->contentname;
	fio_open_pad_files (pad, FALSE);
	
	normalize_dimensions (&info->x, &info->y, &info->width, &info->height);
	
	pad->sticky = FALSE;
	pad->width = info->width;
	pad->height = info->height;
	pad->x = info->x;
	pad->y = info->y;
	pad->hidden = info->hidden;
	
	if (!pad->hidden)
	{
		pad_renew (pad, TRUE);
		pad_set_sticky (pad, info->sticky);
	}
	else
	{
		/* need to load title */
		gchar *content = fio_get_file (pad->contentname);
		gchar *end = g_utf8_strchr (content, -1, '\n');
		
		if (end)
			end[0] = '\0';
		
		pad->title = g_strstrip (g_strdup (content));
		g_free (content);
	}
	
	if (info->locked)
	{
		xpad_text_view_set_follow_global_style (XPAD_TEXT_VIEW (pad->textview), FALSE);
		pad_set_back_color (pad, info->back);
		pad_set_text_color (pad, info->text);
		pad_set_fontname (pad, info->fontname);
	}
	
	if (!pad->hidden)
		gtk_widget_show_all (GTK_WIDGET (pad->window));
	
	gdk_color_free (info->text);
	gdk_color_free (info->back);
	g_free (info->fontname);
	
	return pad;
}

static void
pad_renew (pad_node *pad, gboolean move)
{
	pad_alloc_gtk (pad, pad->infoname);
	
	gtk_window_set_default_size (pad->window, pad->width, pad->height);
	if (move)
		gtk_window_move (pad->window, pad->x, pad->y);
	
	pad_fill_with_file (pad, pad->contentname);
	
	pad_set_title (pad);
	
	pad_set_sticky (pad, pad->sticky);
	
	xpad_dashboard_frontend_init_for_pad (pad);
}
