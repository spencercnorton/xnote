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

#include "../config.h"
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "eggtrayicon.h"
#include "fio.h"
#include "xpad-app.h"
#include "xpad-pad.h"
#include "xpad-pad-group.h"
#include "xpad-preferences.h"
#include "xpad-tray.h"

/* much of this code is shamelessly copied from docklet.c, in the gaim/plugins/docklet
 * directory... since it was one of the only good examples that I could find for using
 * the system tray API. */

static void xpad_tray_popup (GdkEventButton *event);
static void xpad_tray_button_press_event_cb (GtkWidget *button, GdkEventButton *event, gpointer user_data);
static void xpad_tray_toggle (void);

static GtkWidget      *docklet = NULL;
static gboolean        pads_showing;


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
	
	pads_showing = TRUE;
}

void
xpad_tray_close (void)
{
	if (docklet) {
    	g_object_unref (docklet);
		docklet = NULL;
	}
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
	GSList *pads, *i;
	
	pads = xpad_pad_group_get_pads (group);
	
	for (i = pads; i; i = i->next)
		gtk_window_present (GTK_WINDOW (i->data));
	
	g_slist_free (pads);
}

static void
menu_spawn (XpadPadGroup *group)
{
	GtkWidget *pad = xpad_pad_new (group);
	gtk_widget_show (pad);
}

static void
xpad_tray_popup (GdkEventButton *event)
{
	GtkWidget *menu, *item, *imgwidget;
	gint i = 0;
	GSList *pads, *l;
	gint n;
	
	menu = gtk_menu_new ();
	
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menu_spawn), xpad_app_get_pad_group ());
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1); i++;
	gtk_widget_show (item);
	
	item = gtk_separator_menu_item_new ();
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1); i++;
	gtk_widget_show (item);
	
	item = gtk_menu_item_new_with_mnemonic (_("_Show All"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menu_show_all), xpad_app_get_pad_group ());
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1); i++;
	gtk_widget_show (item);
	
	item = gtk_image_menu_item_new_with_mnemonic (_("_Close All"));
	imgwidget = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), imgwidget);
	g_signal_connect (item, "activate", G_CALLBACK (gtk_main_quit), NULL);
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1); i++;
	gtk_widget_show (item);
	
	item = gtk_separator_menu_item_new ();
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1); i++;
	gtk_widget_show (item);
	
	/**
	 * Order pads according to title.
	 */
	pads = xpad_pad_group_get_pads (xpad_app_get_pad_group ());
	
	g_slist_sort (pads, (GCompareFunc) menu_title_compare);
	
	/**
	 * Populate list of windows.
	 */
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
		gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1); i++;
		gtk_widget_show (item);
		
		g_free (title);
	}
	g_slist_free (pads);
	
	item = gtk_separator_menu_item_new ();
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1); i++;
	gtk_widget_show (item);
	
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect (item, "activate", G_CALLBACK (xpad_preferences_open), NULL);
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1); i++;
	gtk_widget_show (item);
	
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
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
