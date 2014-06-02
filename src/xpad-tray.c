/*

Copyright (c) 2002 Jamis Buck
Copyright (c) 2003-2007 Michael Terry
Copyright (c) 2013 Arthur Borsboom

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "../config.h"
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "fio.h"
#include "xpad-app.h"
#include "xpad-pad.h"
#include "xpad-pad-group.h"
#include "xpad-preferences.h"
#include "xpad-settings.h"
#include "xpad-tray.h"

enum
{
	DO_NOTHING,
	TOGGLE_SHOW_ALL,
	LIST_OF_PADS,
	NEW_PAD
};
// tray icon left click handler
static void xpad_tray_activate_cb (GtkStatusIcon *icon);
// tray icon right click handler
static void xpad_tray_popup_menu_cb (GtkStatusIcon *icon, guint button, guint time);
// "toggle show all" menu item handler
static void xpad_tray_show_hide_all (void);
// "show pads" menu item handler
static void xpad_tray_show_windows_list (GtkStatusIcon *icon);
// helper function to append pad window title as item to menu
static void xpad_tray_append_pad_window_titles_to_menu (GtkWidget *menu);

static GtkStatusIcon  *docklet = NULL;
static GtkWidget *menu = NULL;

void
xpad_tray_open ()
{
	GtkIconTheme *theme;

	theme = gtk_icon_theme_get_default ();

	if (!gtk_icon_theme_has_icon (theme, PACKAGE)) {
		return;
	}

	if (gtk_icon_theme_has_icon (theme, "xpad-panel"))
	{
		docklet = gtk_status_icon_new_from_icon_name ("xpad-panel");
	}
	else
	{
	    docklet = gtk_status_icon_new_from_icon_name (PACKAGE);
	}

	if (docklet)
	{
		g_signal_connect (docklet, "activate", G_CALLBACK (xpad_tray_activate_cb), NULL);
		g_signal_connect (docklet, "popup-menu", G_CALLBACK (xpad_tray_popup_menu_cb), NULL);
	}
}

void
xpad_tray_close (void)
{
	if (docklet) {
		g_object_unref (docklet);
		docklet = NULL;
	}

	if (menu)
		gtk_widget_destroy(menu);
}

gboolean
xpad_tray_is_open (void)
{
	if (docklet)
		return gtk_status_icon_is_embedded (docklet);
	else
		return FALSE;
}

static gint
menu_title_compare (GtkWindow *a, GtkWindow *b)
{
	gchar *title_a = g_utf8_casefold (gtk_window_get_title (a), -1);
	gchar *title_b = g_utf8_casefold (gtk_window_get_title (b), -1);
	
	gint rv = g_utf8_collate (title_a, title_b);
	
	g_free (title_a);
	g_free (title_b);
	
	return rv;
}

static void
menu_show_all (XpadPadGroup *group)
{
	GSList *pads = xpad_pad_group_get_pads (group);
	g_slist_foreach (pads, (GFunc) gtk_window_present, NULL);
	g_slist_free (pads);
}

static void 
xpad_tray_show_hide_all (void)
{
	GSList *pads = xpad_pad_group_get_pads (xpad_app_get_pad_group ());
	// find if any pad is visible
	gboolean open = FALSE;
	GSList *i;
	for(i = pads; i != NULL; i = i->next)
	{
		if (gtk_widget_get_visible(GTK_WIDGET(i->data)))
		{
			open = TRUE;
			break;
		}
	}
	g_slist_foreach(pads, (GFunc) (open ? gtk_widget_hide : gtk_widget_show), NULL);
	g_slist_free (pads);
}

static void
menu_spawn (XpadPadGroup *group)
{
	GtkWidget *pad = xpad_pad_new (group);
	gtk_widget_show (pad);
}

static void
xpad_tray_popup_menu_cb (GtkStatusIcon *icon, guint button, guint time)
{
	GtkWidget *item;
	GSList *pads;
	gboolean no_any_pad = FALSE;
	
	menu = gtk_menu_new ();
	pads = xpad_pad_group_get_pads (xpad_app_get_pad_group ());
	if (!pads)
		no_any_pad = TRUE;
	g_slist_free (pads);
	
	item = gtk_menu_item_new_with_mnemonic (_("_New"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menu_spawn), xpad_app_get_pad_group ());
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	item = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);

	item = gtk_menu_item_new_with_mnemonic (_("_Show All"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menu_show_all), xpad_app_get_pad_group ());
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	if (no_any_pad)
		gtk_widget_set_sensitive (item, FALSE);
	
	item = gtk_menu_item_new_with_mnemonic (_("_Close All"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_pad_group_close_all), xpad_app_get_pad_group ());
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	if (no_any_pad)
		gtk_widget_set_sensitive (item, FALSE);
	
	item = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	// append window titles
	xpad_tray_append_pad_window_titles_to_menu (menu);

	item = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	item = gtk_menu_item_new_with_mnemonic (_("_Preferences"));	
	g_signal_connect (item, "activate", G_CALLBACK (xpad_preferences_open), NULL);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	item = gtk_menu_item_new_with_mnemonic (_("_Quit"));	
	g_signal_connect (item, "activate", G_CALLBACK (xpad_app_quit), NULL);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gtk_status_icon_position_menu, icon, button, time);
}

static void
xpad_tray_activate_cb (GtkStatusIcon *icon)
{
	switch (xpad_settings_get_tray_click_handler(xpad_global_settings))
	{
		case TOGGLE_SHOW_ALL:
			xpad_tray_show_hide_all();
			break;
		case LIST_OF_PADS:
			xpad_tray_show_windows_list(icon);
			break;
		case NEW_PAD:
			menu_spawn(xpad_app_get_pad_group());
			break;
	}
}

static void 
xpad_tray_show_windows_list (GtkStatusIcon *icon)
{
	GtkWidget* menu = gtk_menu_new ();
	xpad_tray_append_pad_window_titles_to_menu (menu);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gtk_status_icon_position_menu, icon, 0, gtk_get_current_event_time());
}

static void
xpad_tray_append_pad_window_titles_to_menu (GtkWidget *menu)
{
	GSList *pads, *l;
	GtkWidget *item;
	gint n;

	pads = xpad_pad_group_get_pads (xpad_app_get_pad_group ());
	// Order pads according to title.
	pads = g_slist_sort (pads, (GCompareFunc) menu_title_compare);
	// Populate list of windows.
	for (l = pads, n = 1; l; l = l->next, n++)
	{
		gchar *title;
		gchar *tmp_title;
		
		tmp_title = g_strdup (gtk_window_get_title (GTK_WINDOW (l->data)));
		str_replace_tokens (&tmp_title, '_', "__");
		if (n < 10)
			title = g_strdup_printf ("_%i. %s", n, tmp_title);
		else
			title = g_strdup_printf ("%i. %s", n, tmp_title);
		g_free (tmp_title);
		
		item = gtk_menu_item_new_with_mnemonic (title);
		g_signal_connect_swapped (item, "activate", G_CALLBACK (gtk_window_present), l->data);
		gtk_container_add (GTK_CONTAINER (menu), item);
		gtk_widget_show (item);
		
		g_free (title);
	}
	g_slist_free (pads);
}
