/*

Copyright (c) 2001-2004 Michael Terry

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
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include "xpad-toolbar.h"
#include "xpad-settings.h"
#include "xpad-grip-tool-item.h"

G_DEFINE_TYPE(XpadToolbar, xpad_toolbar, GTK_TYPE_TOOLBAR)
#define XPAD_TOOLBAR_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_TOOLBAR, XpadToolbarPrivate))

enum {
	XPAD_BUTTON_TYPE_SEPARATOR,
	XPAD_BUTTON_TYPE_BUTTON,
	XPAD_BUTTON_TYPE_TOGGLE
};

struct XpadToolbarPrivate
{
	GtkToolItem *move_button;
	gboolean move_removed;
	guint move_index;
	guint move_motion_handler;
	guint move_button_release_handler;
	guint move_key_press_handler;
};

typedef struct
{
	const gchar *name;
	const gchar *stock;
	guint signal;
	guint type;
	const gchar *desc;
	const gchar *menu_desc;
} XpadToolbarButton;

enum
{
	ACTIVATE_NEW,
	ACTIVATE_CLOSE,
	ACTIVATE_DELETE,
	ACTIVATE_CLEAR,
	ACTIVATE_PREFERENCES,
	ACTIVATE_PROPERTIES,
	ACTIVATE_QUIT,
	POPUP,
	POPDOWN,
	LAST_SIGNAL
};

static const XpadToolbarButton buttons[] =
{
	{"Clear", "gtk-clear", ACTIVATE_CLEAR, XPAD_BUTTON_TYPE_BUTTON, N_("Clear Pad Contents"), N_("Add C_lear to Toolbar")},
	{"Close", "gtk-close", ACTIVATE_CLOSE, XPAD_BUTTON_TYPE_BUTTON, N_("Close and Save Pad"), N_("Add _Close to Toolbar")},
	{"Delete", "gtk-delete", ACTIVATE_DELETE, XPAD_BUTTON_TYPE_BUTTON, N_("Delete Pad"), N_("Add _Delete to Toolbar")},
	{"New", "gtk-new", ACTIVATE_NEW, XPAD_BUTTON_TYPE_BUTTON, N_("Open New Pad"), N_("Add _New to Toolbar")},
	{"Preferences", "gtk-preferences", ACTIVATE_PREFERENCES, XPAD_BUTTON_TYPE_BUTTON, N_("Edit Preferences"), N_("Add Pr_eferences to Toolbar")},
	{"Properties", "gtk-properties", ACTIVATE_PROPERTIES, XPAD_BUTTON_TYPE_BUTTON, N_("Edit Pad Properties"), N_("Add Proper_ties to Toolbar")},
	{"Quit", "gtk-quit", ACTIVATE_QUIT, XPAD_BUTTON_TYPE_BUTTON, N_("Close All Pads"), N_("Add Close _All to Toolbar")},
	{"sep", NULL, 0, XPAD_BUTTON_TYPE_SEPARATOR, NULL, N_("Add a Se_parator to Toolbar")} /* Separator */
	/*{"Minimize to Tray", "gtk-goto-bottom", 1, N_("Minimize Pads to System Tray")}*/
};


/*static void xpad_toolbar_popup_context_menu (XpadToolbar *toolbar, gint x, gint y, gint button);*/
static G_CONST_RETURN XpadToolbarButton *xpad_toolbar_button_lookup (XpadToolbar *toolbar, const gchar *name);
static GtkToolItem *xpad_toolbar_button_to_item (XpadToolbar *toolbar, const XpadToolbarButton *button);
static void xpad_toolbar_button_activated (GtkToolButton *button);
static void xpad_toolbar_change_buttons (XpadToolbar *toolbar);
static void xpad_toolbar_finalize (GObject *object);
static void xpad_toolbar_add_button (const gchar *button_name);
static void xpad_toolbar_remove_button (GtkWidget *button);
static gboolean xpad_toolbar_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gboolean xpad_toolbar_popup_context_menu (GtkToolbar *toolbar, gint x, gint y, gint button);
static gboolean xpad_toolbar_popup_button_menu (GtkWidget *button, GdkEventButton *event, XpadToolbar *toolbar);

static gboolean xpad_toolbar_move_button_start (XpadToolbar *toolbar, GtkWidget *button);
static gboolean xpad_toolbar_move_button_move (XpadToolbar *toolbar, GdkEventMotion *event);
static gboolean xpad_toolbar_move_button_move_keyboard (XpadToolbar *toolbar, GdkEventKey *event);
static gboolean xpad_toolbar_move_button_end (XpadToolbar *toolbar);


static guint signals[LAST_SIGNAL] = { 0 };

GtkWidget *
xpad_toolbar_new (void)
{
	return GTK_WIDGET (g_object_new (XPAD_TYPE_TOOLBAR, NULL));
}

static void
xpad_toolbar_class_init (XpadToolbarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkToolbarClass *gtktoolbar_class = GTK_TOOLBAR_CLASS (klass);
	
	gtktoolbar_class->popup_context_menu = xpad_toolbar_popup_context_menu;
	gobject_class->finalize = xpad_toolbar_finalize;
	
	/* Signals */
	
	signals[ACTIVATE_NEW] = 
		g_signal_new ("activate-new",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, activate_new),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[ACTIVATE_CLOSE] = 
		g_signal_new ("activate-close",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, activate_close),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[ACTIVATE_QUIT] = 
		g_signal_new ("activate-quit",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, activate_quit),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[ACTIVATE_CLEAR] = 
		g_signal_new ("activate-clear",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, activate_clear),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[ACTIVATE_PROPERTIES] = 
		g_signal_new ("activate-properties",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, activate_properties),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[ACTIVATE_PREFERENCES] = 
		g_signal_new ("activate-preferences",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, activate_preferences),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[ACTIVATE_DELETE] = 
		g_signal_new ("activate-delete",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, activate_delete),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[POPUP] = 
		g_signal_new ("popup",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, popup),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, GTK_TYPE_MENU);
	
	signals[POPDOWN] = 
		g_signal_new ("popdown",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadToolbarClass, popdown),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, GTK_TYPE_MENU);
	
	g_type_class_add_private (gobject_class, sizeof (XpadToolbarPrivate));
}

static void
xpad_toolbar_init (XpadToolbar *toolbar)
{
	toolbar->priv = XPAD_TOOLBAR_GET_PRIVATE (toolbar);
	
	toolbar->priv->move_motion_handler = 0;
	toolbar->priv->move_button_release_handler = 0;
	toolbar->priv->move_key_press_handler = 0;
	
	g_object_set (G_OBJECT (toolbar),
	              "icon-size", GTK_ICON_SIZE_MENU,
	              "show-arrow", FALSE,
	              "toolbar-style", GTK_TOOLBAR_ICONS,
	              NULL);
	
	g_signal_connect_swapped (xpad_settings (), "change-buttons", G_CALLBACK (xpad_toolbar_change_buttons), toolbar);
	
	xpad_toolbar_change_buttons (toolbar);
}

static void
xpad_toolbar_finalize (GObject *object)
{
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);
	
	if (toolbar->priv->move_button)
		g_object_unref (toolbar->priv->move_button);
	
	g_signal_handlers_disconnect_matched (xpad_settings (), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, toolbar);
	
	G_OBJECT_CLASS (xpad_toolbar_parent_class)->finalize (object);
}

static gboolean
xpad_toolbar_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	/* Ignore double-clicks and triple-clicks */
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
	{
		XpadToolbar *toolbar = XPAD_TOOLBAR (g_object_get_data (G_OBJECT (widget), "xpad-toolbar"));
		xpad_toolbar_popup_button_menu (widget, event, toolbar);
		return TRUE;
	}
	else if (event->button == 2 && event->type == GDK_BUTTON_PRESS)
	{
		XpadToolbar *toolbar = XPAD_TOOLBAR (g_object_get_data (G_OBJECT (widget), "xpad-toolbar"));
		xpad_toolbar_move_button_start (toolbar, widget);
		return TRUE;
	}
	
	return FALSE;
}

static G_CONST_RETURN XpadToolbarButton *
xpad_toolbar_button_lookup (XpadToolbar *toolbar, const gchar *name)
{
	gint i;
	for (i = 0; i < G_N_ELEMENTS (buttons); i++)
		if (!g_ascii_strcasecmp (name, buttons[i].name))
			return &buttons[i];
	
	return NULL;
}

static GtkToolItem *
xpad_toolbar_button_to_item (XpadToolbar *toolbar, const XpadToolbarButton *button)
{
	GtkToolItem *item;
	GtkWidget *child;
	
	switch (button->type)
	{
	case XPAD_BUTTON_TYPE_BUTTON:
		item = GTK_TOOL_ITEM (gtk_tool_button_new_from_stock (button->stock));
		g_signal_connect (item, "clicked", G_CALLBACK (xpad_toolbar_button_activated), NULL);
		break;
	case XPAD_BUTTON_TYPE_TOGGLE:
		item = GTK_TOOL_ITEM (gtk_toggle_tool_button_new_from_stock (button->stock));
		g_signal_connect (item, "toggled", G_CALLBACK (xpad_toolbar_button_activated), NULL);
		break;
	case XPAD_BUTTON_TYPE_SEPARATOR:
		item = GTK_TOOL_ITEM (gtk_separator_tool_item_new ());
		break;
	default:
		return NULL;
	}
	
	g_object_set_data (G_OBJECT (item), "xpad-toolbar", toolbar);
	g_object_set_data (G_OBJECT (item), "xpad-tb", (gpointer) button);
	
	if (button->desc)
		gtk_tool_item_set_tooltip_text (item, _(button->desc));
	
	child = gtk_bin_get_child (GTK_BIN (item));
	if (child)
	{
		g_signal_connect_swapped (child, "button-press-event", G_CALLBACK (xpad_toolbar_button_press_event), item);
	}
	
	return item;
}

static void
xpad_toolbar_button_activated (GtkToolButton *button)
{
	XpadToolbar *toolbar;
	const XpadToolbarButton *tb;
	
	toolbar = XPAD_TOOLBAR (g_object_get_data (G_OBJECT (button), "xpad-toolbar"));
	tb = (const XpadToolbarButton *) g_object_get_data (G_OBJECT (button), "xpad-tb");
	
	g_signal_emit (toolbar, signals[tb->signal], 0);
}

static void
xpad_toolbar_change_buttons (XpadToolbar *toolbar)
{
	GList *list, *temp;
	const GSList *slist, *stemp;
	gint i = 0;
	GtkToolItem *item;
	
	list = gtk_container_get_children (GTK_CONTAINER (toolbar));
	
	for (temp = list; temp; temp = temp->next)
		gtk_widget_destroy (temp->data);
	
	g_list_free (list);
	
	slist = xpad_settings_get_toolbar_buttons (xpad_settings ());
	for (stemp = slist; stemp; stemp = stemp->next)
	{
		const XpadToolbarButton *button;
		
		button = xpad_toolbar_button_lookup (toolbar, stemp->data);
		if (!button)
			continue;
		
		item = xpad_toolbar_button_to_item (toolbar, button);
		
		if (item)
		{
			g_object_set_data (G_OBJECT (item), "xpad-button-num", GINT_TO_POINTER (i));
			gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
			gtk_widget_show_all (GTK_WIDGET (item));
			i++;
		}
	}
	
	item = gtk_separator_tool_item_new ();
	g_object_set_data (G_OBJECT (item), "xpad-button-num", GINT_TO_POINTER (i));
	gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
	gtk_tool_item_set_expand (item, TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	gtk_widget_show_all (GTK_WIDGET (item));
	i++;
	
	item = xpad_grip_tool_item_new ();
	g_object_set_data (G_OBJECT (item), "xpad-button-num", GINT_TO_POINTER (i));
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	gtk_widget_show_all (GTK_WIDGET (item));
	i++;
}

static void
xpad_toolbar_add_button (const gchar *name)
{
	xpad_settings_add_toolbar_button (xpad_settings (), name);
}

static void
xpad_toolbar_remove_button (GtkWidget *button)
{
	gint button_num;
	
	button_num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "xpad-button-num"));
	
	xpad_settings_remove_toolbar_button (xpad_settings (), button_num);
}

static gboolean
xpad_toolbar_move_button_start (XpadToolbar *toolbar, GtkWidget *button)
{
	GdkGrabStatus  status;
	GdkCursor     *fleur_cursor;
	GtkWidget *widget;
	
	widget = GTK_WIDGET (toolbar);
	gtk_grab_add (widget);
	
	fleur_cursor = gdk_cursor_new (GDK_FLEUR);
	
	g_object_ref (button);
	toolbar->priv->move_removed = FALSE;
	toolbar->priv->move_button = GTK_TOOL_ITEM (button);
	toolbar->priv->move_index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "xpad-button-num"));
	
	toolbar->priv->move_button_release_handler = g_signal_connect (toolbar, "button-release-event", G_CALLBACK (xpad_toolbar_move_button_end), NULL);
	toolbar->priv->move_key_press_handler = g_signal_connect (toolbar, "key-press-event", G_CALLBACK (xpad_toolbar_move_button_move_keyboard), NULL);
	toolbar->priv->move_motion_handler = g_signal_connect (toolbar, "motion-notify-event", G_CALLBACK (xpad_toolbar_move_button_move), NULL);
	
	status = gdk_pointer_grab (widget->window, FALSE,
				   GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK, NULL,
				   fleur_cursor, gtk_get_current_event_time ());
	
	gdk_cursor_unref (fleur_cursor);
	gdk_flush ();
	
	if (status != GDK_GRAB_SUCCESS)
	{
		xpad_toolbar_move_button_end (toolbar);
	}
	
	return TRUE;
}

static gboolean
xpad_toolbar_move_button_move_keyboard (XpadToolbar *toolbar, GdkEventKey *event)
{
	if (event->keyval == GDK_Left || event->keyval == GDK_KP_Left)
	{
		if (!toolbar->priv->move_removed)
		{
			toolbar->priv->move_removed = TRUE;
			gtk_container_remove (GTK_CONTAINER (toolbar), GTK_WIDGET (toolbar->priv->move_button));
		}
		
		if (toolbar->priv->move_index > 0)
			toolbar->priv->move_index--;
		
		gtk_toolbar_set_drop_highlight_item (GTK_TOOLBAR (toolbar), toolbar->priv->move_button, toolbar->priv->move_index);
	}
	else if (event->keyval == GDK_Right || event->keyval == GDK_KP_Right)
	{
		gint max;
		
		if (!toolbar->priv->move_removed)
		{
			toolbar->priv->move_removed = TRUE;
			gtk_container_remove (GTK_CONTAINER (toolbar), GTK_WIDGET (toolbar->priv->move_button));
		}
		
		max = gtk_toolbar_get_n_items (GTK_TOOLBAR (toolbar)) - 2;
		
		if (toolbar->priv->move_index < max)
			toolbar->priv->move_index++;
		
		gtk_toolbar_set_drop_highlight_item (GTK_TOOLBAR (toolbar), toolbar->priv->move_button, toolbar->priv->move_index);
	}
	else if (event->keyval == GDK_space || event->keyval == GDK_KP_Space || event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
	{
		xpad_toolbar_move_button_end (toolbar);
		return TRUE;
	}
	
	return TRUE;
}

static gboolean
xpad_toolbar_move_button_move (XpadToolbar *toolbar, GdkEventMotion *event)
{
	gint max;
	
	if (!toolbar->priv->move_removed)
	{
		toolbar->priv->move_removed = TRUE;
		gtk_container_remove (GTK_CONTAINER (toolbar), GTK_WIDGET (toolbar->priv->move_button));
	}
	
	toolbar->priv->move_index = gtk_toolbar_get_drop_index (GTK_TOOLBAR (toolbar), event->x, event->y);
	
	/* Must not move past separator or grip */
	max = gtk_toolbar_get_n_items (GTK_TOOLBAR (toolbar)) - 2;
	toolbar->priv->move_index = MIN (toolbar->priv->move_index, max);
	
	gtk_toolbar_set_drop_highlight_item (GTK_TOOLBAR (toolbar), toolbar->priv->move_button, toolbar->priv->move_index);
	
	return TRUE;
}

static gboolean
xpad_toolbar_move_button_end (XpadToolbar *toolbar)
{
	gint old_spot;
	gint max;
	
	g_signal_handler_disconnect (toolbar, toolbar->priv->move_button_release_handler);
	g_signal_handler_disconnect (toolbar, toolbar->priv->move_key_press_handler);
	g_signal_handler_disconnect (toolbar, toolbar->priv->move_motion_handler);
	toolbar->priv->move_button_release_handler = 0;
	toolbar->priv->move_key_press_handler = 0;
	toolbar->priv->move_motion_handler = 0;
	
	gtk_toolbar_set_drop_highlight_item (GTK_TOOLBAR (toolbar), NULL, 0);
	
	old_spot = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (toolbar->priv->move_button), "xpad-button-num"));
	
	/* Must not move past separator or grip */
	max = gtk_toolbar_get_n_items (GTK_TOOLBAR (toolbar)) - 2;
	toolbar->priv->move_index = MIN (toolbar->priv->move_index, max);
	
	if (!xpad_settings_move_toolbar_button (xpad_settings (), old_spot,	toolbar->priv->move_index) &&
	    toolbar->priv->move_removed)
	{
		gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolbar->priv->move_button, toolbar->priv->move_index);
	}
	
	g_object_unref (toolbar->priv->move_button);
	toolbar->priv->move_button = NULL;
	
	gtk_grab_remove (GTK_WIDGET (toolbar));
	gdk_pointer_ungrab (gtk_get_current_event_time ());
	return TRUE;
}

static void
move_menu_item_activated (GtkWidget *button)
{
	XpadToolbar *toolbar;
	
	toolbar = XPAD_TOOLBAR (g_object_get_data (G_OBJECT (button), "xpad-toolbar"));
	
	xpad_toolbar_move_button_start (toolbar, button);
}

static void
menu_deactivated (GtkWidget *menu, GtkToolbar *toolbar)
{
	g_signal_emit (toolbar, signals[POPDOWN], 0, menu);
}

static gboolean
xpad_toolbar_popup_button_menu (GtkWidget *button, GdkEventButton *event, XpadToolbar *toolbar)
{
	GtkWidget *menu;
	GtkWidget *item, *image;
	
	menu = gtk_menu_new ();
	
	
	item = gtk_image_menu_item_new_with_mnemonic (_("_Remove From Toolbar"));
	
	image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	
	g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_toolbar_remove_button), button);
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, 0, 1);
	gtk_widget_show (item);
	
	
	item = gtk_menu_item_new_with_mnemonic (_("_Move"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (move_menu_item_activated), button);
	gtk_menu_attach (GTK_MENU (menu), item, 0, 1, 1, 2);
	gtk_widget_show (item);
	
	
	g_signal_connect (menu, "deactivate", G_CALLBACK (menu_deactivated), toolbar);
	
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event ? event->button : 0, gtk_get_current_event_time ());
	
	g_signal_emit (toolbar, signals[POPUP], 0, menu);
	
	return TRUE;
}

static gboolean
xpad_toolbar_popup_context_menu (GtkToolbar *toolbar, gint x, gint y, gint button)
{
	GtkWidget *menu;
	const GSList *current_buttons;
	gint i;
	
	menu = gtk_menu_new ();
	
	current_buttons = xpad_settings_get_toolbar_buttons (xpad_settings ());
	
	for (i = 0; i < G_N_ELEMENTS (buttons); i++)
	{
		const GSList *j;
		GtkWidget *item, *image;
		
		if (strcmp (buttons[i].name, "sep") != 0)
		{
			for (j = current_buttons; j; j = j->next)
				if (!g_ascii_strcasecmp (j->data, buttons[i].name))
					break;
			
			if (j)
				continue;
		}
		else
		{
			/* Don't let user add separators until we can allow clicks on them. */
			continue;
		}
		
		item = gtk_image_menu_item_new_with_mnemonic (buttons[i].menu_desc);
		
		image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
		
		g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_toolbar_add_button), (gpointer) buttons[i].name);
		
		gtk_menu_attach (GTK_MENU (menu), item, 0, 1, i, i + 1);
		gtk_widget_show (item);
	}
	
	g_signal_connect (menu, "deactivate", G_CALLBACK (menu_deactivated), toolbar);
	
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, (button < 0) ? 0 : button, gtk_get_current_event_time ());
	
	g_signal_emit (toolbar, signals[POPUP], 0, menu);
	
	return TRUE;
}
