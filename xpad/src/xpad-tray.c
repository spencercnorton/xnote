/*

Copyright (c) 2002 Jamis Buck
Copyright (c) 2003-2004 Michael Terry

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

#include <gtk/gtk.h>
#include "defines.h"
#include "eggtrayicon.h"
#include "pad.h"
#include "xpad-app.h"
#include "xpad-pad-group.h"
#include "xpad-tray.h"

/* much of this code is shamelessly copied from docklet.c, in the gaim/plugins/docklet
 * directory... since it was one of the only good examples that I could find for using
 * the system tray API. */

static void xpad_tray_popup (GdkEventButton *event);
static void xpad_tray_button_press_event_cb (GtkWidget *button, GdkEventButton *event, gpointer user_data);
static void xpad_tray_toggle (void);

static GtkWidget      *docklet = NULL;
static gboolean        pads_showing;

static GtkUIManager   *ui_manager = NULL;
static GtkActionGroup *popup_notes_actions = NULL;
static gint            popup_notes_merge_id;


gboolean
xpad_tray_is_open (void)
{
	return docklet && GTK_WIDGET_VISIBLE (docklet);
}

void
xpad_tray_open (void)
{
	GtkWidget      *box;
	GtkWidget      *image;
	GdkPixbuf      *pixbuf;
	GtkActionGroup *actions;
	gchar          *ui_filename;
	
	xpad_tray_close ();
	
	docklet = GTK_WIDGET (egg_tray_icon_new (PACKAGE));
	box = gtk_event_box_new ();
	image = gtk_image_new ();
	
	g_signal_connect (box, "button-press-event", G_CALLBACK (xpad_tray_button_press_event_cb), NULL);
	
	gtk_container_add (GTK_CONTAINER (box), image);
	gtk_container_add (GTK_CONTAINER (docklet), box);
	gtk_widget_show_all (docklet);
	
	g_object_ref (docklet);
	
	pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
	                                   PACKAGE,
	                                   24,
	                                   0,
	                                   NULL);
	
	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
	g_object_unref (pixbuf);
	
	/* set up action group */
	actions = gtk_action_group_new (PACKAGE "-tray");
	gtk_action_group_add_actions (actions, pad_actions, num_pad_actions, NULL);
	gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);
	
	/* set up ui manager */
	ui_filename = g_build_filename (PKGDATADIR, "xpad-tray.ui", NULL);
	ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui_manager, actions, 0);
	gtk_ui_manager_add_ui_from_file (ui_manager, ui_filename, NULL);
/*	gtk_window_add_accel_group (pad->window, gtk_ui_manager_get_accel_group (ui_manager));*/
	g_object_unref (actions);
	gtk_ui_manager_ensure_update (ui_manager);
	g_free (ui_filename);
	
	pads_showing = TRUE;
}

void
xpad_tray_close (void)
{
	if (docklet) {
    		g_object_unref (docklet);
    		g_object_unref (ui_manager);
    		g_object_unref (popup_notes_actions);
		docklet = NULL;
		ui_manager = NULL;
		popup_notes_actions = NULL;
	}
}

static gchar *
create_popup_ui (int num_pads)
{
	gchar *ui, *new_ui;
	gint i;
	
	ui = g_strdup ("<ui><popup name='TrayPopupItem'><placeholder name='NotesListItem'>");
	
	for (i = 1; i <= num_pads; i++) {
		new_ui = g_strdup_printf ("%s<menuitem name='ShowNoteItem-%i' action='ShowNoteAction-%i'/>", ui, i, i);
		g_free (ui);
		ui = new_ui;
	}
	
	new_ui = g_strdup_printf ("%s</placeholder></popup></ui>", ui);
	g_free (ui);
	ui = new_ui;
	
	return ui;
}

static gint
pad_title_compare (pad_node *a, pad_node *b)
{
	gchar *title_a = g_utf8_casefold (a->title, -1);
	gchar *title_b = g_utf8_casefold (b->title, -1);
	
	gint rv = g_utf8_collate (title_a, title_b);
	
	g_free (title_a);
	g_free (title_b);
	
	return rv;
}

static void
xpad_tray_popup (GdkEventButton *event)
{
	pad_node *p;
	GtkWidget *tmp;
	gint n = 0;
	GList *pads = NULL, *l;
	gchar *ui;
	
	if (popup_notes_actions) {
		gtk_ui_manager_remove_ui (ui_manager, popup_notes_merge_id);
		gtk_ui_manager_remove_action_group (ui_manager, popup_notes_actions);
		g_free (popup_notes_actions);
	}
	
	popup_notes_actions = gtk_action_group_new (PACKAGE "-tray-notes");
	
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
		
		gtk_action_group_add_actions (popup_notes_actions, &entry, 1, l->data);
		
		g_free (title);
		g_free (action_name);
	}
	g_list_free (pads);
	
	ui = create_popup_ui (n - 1);
	popup_notes_merge_id = gtk_ui_manager_add_ui_from_string (ui_manager, ui, -1, NULL);
	g_free (ui);
	
	gtk_ui_manager_insert_action_group (ui_manager, popup_notes_actions, 0);
	
	tmp = gtk_ui_manager_get_widget (ui_manager, "/TrayPopupItem");
	gtk_menu_popup (GTK_MENU (tmp), NULL, NULL, NULL, NULL, event->button, event->time);
}

static void
xpad_tray_button_press_event_cb (GtkWidget *button, GdkEventButton *event, gpointer user_data)
{
	if (event->type != GDK_BUTTON_PRESS)
		return;
	
	switch (event->button) {
	case 1:
		xpad_tray_toggle ();
		break;
	case 3:
		xpad_tray_popup (event);
		break;
	}
}

static void
xpad_tray_toggle (void)
{
	xpad_pad_group_toggle_hide (xpad_app_get_pad_group ());
}
