/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2013-2024 Arthur Borsboom

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

#include "constants.h"
#include "xpad-toolbar.h"
#include "xpad-app.h"

struct XpadToolbarPrivate
{
	gboolean move_removed;
	guint move_index;
	guint move_motion_handler;
	guint move_button_release_handler;
	guint move_key_press_handler;
	XpadPad *pad;
};

/* GTK 4 removed GtkToolbar/GtkToolItem; XpadToolbar is now a horizontal GtkBox
   of plain GtkButtons, separators and the resize grip. */
G_DEFINE_TYPE_WITH_PRIVATE (XpadToolbar, xpad_toolbar, GTK_TYPE_BOX)

enum {
	XPAD_BUTTON_TYPE_SEPARATOR,
	XPAD_BUTTON_TYPE_BUTTON,
	XPAD_BUTTON_TYPE_TOGGLE,
	XPAD_BUTTON_TYPE_COLOR
};

typedef struct
{
	const gchar *name;
	const gchar *icon_name;
	guint signal;
	guint type;
	const gchar *desc;
	const gchar *menu_desc;
} XpadToolbarButton;

enum
{
	ACTIVATE_NEW,
	ACTIVATE_CLOSE,
	ACTIVATE_UNDO,
	ACTIVATE_REDO,
	ACTIVATE_CUT,
	ACTIVATE_COPY,
	ACTIVATE_PASTE,
	ACTIVATE_SEARCH,
	ACTIVATE_DELETE,
	ACTIVATE_CLEAR,
	ACTIVATE_PREFERENCES,
	ACTIVATE_PROPERTIES,
	ACTIVATE_QUIT,
	POPUP,
	POPDOWN,
	LAST_SIGNAL
};

/* Symbolic icons so the buttons recolor with the OSD pill / theme. */
static const XpadToolbarButton buttons[] =
{
	{"Clear", "edit-clear-all-symbolic", ACTIVATE_CLEAR, XPAD_BUTTON_TYPE_BUTTON, N_("Clear Pad Contents"), N_("Add C_lear button")},
	{"Close", "window-close-symbolic", ACTIVATE_CLOSE, XPAD_BUTTON_TYPE_BUTTON, N_("Close and Save Pad"), N_("Add _Close button")},
	{"Color", NULL, 0, XPAD_BUTTON_TYPE_COLOR, N_("Change Sticky-Note Color"), N_("Add Co_lor button")},
	{"Copy", "edit-copy-symbolic", ACTIVATE_COPY, XPAD_BUTTON_TYPE_BUTTON, N_("Copy to Clipboard"), N_("Add C_opy button")},
	{"Cut", "edit-cut-symbolic", ACTIVATE_CUT, XPAD_BUTTON_TYPE_BUTTON, N_("Cut to Clipboard"), N_("Add C_ut button")},
	{"Delete", "user-trash-symbolic", ACTIVATE_DELETE, XPAD_BUTTON_TYPE_BUTTON, N_("Delete Pad"), N_("Add _Delete button")},
	{"Find", "edit-find-symbolic", ACTIVATE_SEARCH, XPAD_BUTTON_TYPE_BUTTON, N_("Find text"), N_("Add _Find button")},
	{"New", "list-add-symbolic", ACTIVATE_NEW, XPAD_BUTTON_TYPE_BUTTON, N_("Open New Pad"), N_("Add _New button")},
	{"Paste", "edit-paste-symbolic", ACTIVATE_PASTE, XPAD_BUTTON_TYPE_BUTTON, N_("Paste from Clipboard"), N_("Add Pa_ste button")},
	{"Preferences", "preferences-system-symbolic", ACTIVATE_PREFERENCES, XPAD_BUTTON_TYPE_BUTTON, N_("Edit Preferences"), N_("Add Pr_eferences button")},
	{"Properties", "document-properties-symbolic", ACTIVATE_PROPERTIES, XPAD_BUTTON_TYPE_BUTTON, N_("Edit Pad Properties"), N_("Add Proper_ties button")},
	{"Redo", "edit-redo-symbolic", ACTIVATE_REDO, XPAD_BUTTON_TYPE_BUTTON, N_("Redo"), N_("Add _Redo button")},
	{"Quit", "application-exit-symbolic", ACTIVATE_QUIT, XPAD_BUTTON_TYPE_BUTTON, N_("Close All Pads"), N_("Add Close _All button")},
	{"Undo", "edit-undo-symbolic", ACTIVATE_UNDO, XPAD_BUTTON_TYPE_BUTTON, N_("Undo"), N_("Add _Undo button")},
	{"Separator", NULL, 0, XPAD_BUTTON_TYPE_SEPARATOR, NULL, N_("Add Se_parator")}
};

static void xpad_toolbar_constructed (GObject *object);
static void xpad_toolbar_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_toolbar_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_toolbar_dispose (GObject *object);
static const XpadToolbarButton *xpad_toolbar_button_lookup (XpadToolbar *toolbar, const gchar *name);
static GtkWidget *xpad_toolbar_button_to_item (XpadToolbar *toolbar, const XpadToolbarButton *button);
static void xpad_toolbar_button_activated (GtkButton *button);
static void xpad_toolbar_change_buttons (XpadToolbar *toolbar);
static void xpad_toolbar_add_button (GtkButton *menu_button, XpadSettings *settings);
static void xpad_toolbar_right_click (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer data);

static guint signals[LAST_SIGNAL] = { 0 };

enum
{
	PROP_0,
	PROP_PAD,
	LAST_PROP
};

GtkWidget *
xpad_toolbar_new (XpadPad *pad)
{
	return GTK_WIDGET (g_object_new (XPAD_TYPE_TOOLBAR, "pad", pad, NULL));
}

static void
xpad_toolbar_class_init (XpadToolbarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructed = xpad_toolbar_constructed;
	gobject_class->set_property = xpad_toolbar_set_property;
	gobject_class->get_property = xpad_toolbar_get_property;
	gobject_class->dispose = xpad_toolbar_dispose;
	/* GtkToolbar's popup_context_menu vfunc is gone; the context menu is now
	   driven by a secondary-button GtkGestureClick set up in _init(). */

	signals[ACTIVATE_NEW] = 
		g_signal_new ("activate-new",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_CLOSE] = 
		g_signal_new ("activate-close",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_UNDO] = 
		g_signal_new ("activate-undo",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_REDO] = 
		g_signal_new ("activate-redo",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_CUT] = 
		g_signal_new ("activate-cut",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_COPY] = 
		g_signal_new ("activate-copy",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_PASTE] = 
		g_signal_new ("activate-paste",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_SEARCH] =
		g_signal_new ("activate-search",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_QUIT] = 
		g_signal_new ("activate-quit",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_CLEAR] = 
		g_signal_new ("activate-clear",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[ACTIVATE_PROPERTIES] = 
		g_signal_new ("activate-properties",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_PREFERENCES] = 
		g_signal_new ("activate-preferences",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_DELETE] = 
		g_signal_new ("activate-delete",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[POPUP] =
		g_signal_new ("popup",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);

	signals[POPDOWN] =
		g_signal_new ("popdown",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);

	g_object_class_install_property (gobject_class,
					 PROP_PAD,
					 g_param_spec_pointer ("pad",
									"Pad",
									"Pad associated with this toolbar",
									G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
xpad_toolbar_init (XpadToolbar *toolbar)
{
	toolbar->priv = xpad_toolbar_get_instance_private(toolbar);

	toolbar->priv->move_motion_handler = 0;
	toolbar->priv->move_button_release_handler = 0;
	toolbar->priv->move_key_press_handler = 0;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar), GTK_ORIENTATION_HORIZONTAL);

	/* Secondary (right) click anywhere on the toolbar opens the
	   add/remove-buttons context menu. */
	GtkGesture *click = gtk_gesture_click_new ();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), GDK_BUTTON_SECONDARY);
	g_signal_connect (click, "pressed", G_CALLBACK (xpad_toolbar_right_click), toolbar);
	gtk_widget_add_controller (GTK_WIDGET (toolbar), GTK_EVENT_CONTROLLER (click));
}

static void xpad_toolbar_constructed (GObject *object)
{
	XpadSettings *settings;
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);

	xpad_toolbar_change_buttons (toolbar);

	/* Add CSS style class, so the styling can be overridden by a GTK theme.
	   GTK 4 manages style classes directly on the widget. The "osd" + "toolbar"
	   combination picks up the theme's translucent floating-toolbar look; the
	   pill rounding lives in the app CSS keyed on "XpadToolbar". */
	gtk_widget_add_css_class (GTK_WIDGET (toolbar), "XpadToolbar");
	gtk_widget_add_css_class (GTK_WIDGET (toolbar), "toolbar");
	gtk_widget_add_css_class (GTK_WIDGET (toolbar), "osd");

	g_object_get (toolbar->priv->pad, "settings", &settings, NULL);
	/* settings is the app-lifetime singleton shared by every pad, so this handler
	   must be torn down when the toolbar dies. g_signal_connect_object (unlike
	   ..._connect_swapped) auto-disconnects when the toolbar is finalized —
	   without it, deleting a pad leaves a dangling handler that fires on the freed
	   toolbar the next time any pad adds/removes a toolbar button. */
	g_signal_connect_object (settings, "change-buttons", G_CALLBACK (xpad_toolbar_change_buttons), toolbar, G_CONNECT_SWAPPED);
}

static void
xpad_toolbar_dispose (GObject *object)
{
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);

	toolbar->priv->pad = NULL;  /* non-owning back-reference; nothing to unref */

	G_OBJECT_CLASS (xpad_toolbar_parent_class)->dispose (object);
}

void
xpad_toolbar_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);

	switch (prop_id)
	{
	case PROP_PAD:
		/* Non-owning back-reference: the pad owns this toolbar (a child in the
		   pad's widget tree) and always outlives it, so taking a ref here would
		   form a cycle that stops the pad from ever finalizing (per-delete leak). */
		if (G_VALUE_HOLDS_POINTER (value) && G_IS_OBJECT (g_value_get_pointer (value)))
			toolbar->priv->pad = g_value_get_pointer (value);
		else
			toolbar->priv->pad = NULL;

		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	 }
}

void
xpad_toolbar_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);

	switch (prop_id)
	{
	case PROP_PAD:
		g_value_set_pointer (value, toolbar->priv->pad);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static const XpadToolbarButton *
xpad_toolbar_button_lookup (XpadToolbar *toolbar, const gchar *name)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) toolbar;

	guint i;
	for (i = 0; i < G_N_ELEMENTS (buttons); i++)
		if (!g_ascii_strcasecmp (name, buttons[i].name))
			return &buttons[i];

	return NULL;
}

/* Paints a filled circle. With an "xpad-fixed-color" GdkRGBA on the area it
   paints that palette entry; without one it shows the pad's current
   background color (the toolbar swatch). */
static void
color_swatch_draw (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
	XpadToolbar *toolbar = XPAD_TOOLBAR (data);
	const GdkRGBA *fixed = g_object_get_data (G_OBJECT (area), "xpad-fixed-color");
	GdkRGBA color;

	if (fixed)
		color = *fixed;
	else
		xpad_pad_get_back_color (toolbar->priv->pad, &color);

	double r = MIN (width, height) / 2.0 - 1.0;

	cairo_arc (cr, width / 2.0, height / 2.0, r, 0, 2 * G_PI);
	gdk_cairo_set_source_rgba (cr, &color);
	cairo_fill_preserve (cr);

	/* Thin darker rim so pale colors stay visible on the OSD pill. */
	cairo_set_source_rgba (cr, color.red * 0.6, color.green * 0.6, color.blue * 0.6, 1.0);
	cairo_set_line_width (cr, 1.0);
	cairo_stroke (cr);
}

static void
color_chosen (GtkButton *button, XpadToolbar *toolbar)
{
	const GdkRGBA *color = g_object_get_data (G_OBJECT (button), "xpad-color");
	GtkMenuButton *menu_button = g_object_get_data (G_OBJECT (button), "xpad-menu-button");

	xpad_pad_set_back_color (toolbar->priv->pad, color);
	gtk_menu_button_popdown (menu_button);
}

/* Keep the autohide toolbar pinned while the palette popover is open, the
   same way the pad menus do. */
static void
color_popover_mapped (GtkWidget *popover, XpadToolbar *toolbar)
{
	g_signal_emit (toolbar, signals[POPUP], 0, popover);
}

static void
color_popover_closed (GtkPopover *popover, XpadToolbar *toolbar)
{
	g_signal_emit (toolbar, signals[POPDOWN], 0, popover);
}

static GtkWidget *
xpad_toolbar_make_color_item (XpadToolbar *toolbar)
{
	GtkWidget *item = gtk_menu_button_new ();
	gtk_widget_add_css_class (item, "flat");

	GtkWidget *swatch = gtk_drawing_area_new ();
	gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (swatch), 16);
	gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (swatch), 16);
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (swatch), color_swatch_draw, toolbar, NULL);
	gtk_menu_button_set_child (GTK_MENU_BUTTON (item), swatch);

	GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

	for (guint i = 0; i < xpad_sticky_colors_n; i++)
	{
		GdkRGBA rgba;

		if (!gdk_rgba_parse (&rgba, xpad_sticky_colors[i].hex))
			continue;

		GtkWidget *choice = gtk_button_new ();
		gtk_widget_add_css_class (choice, "flat");
		gtk_widget_set_tooltip_text (choice, _(xpad_sticky_colors[i].name));

		GtkWidget *choice_swatch = gtk_drawing_area_new ();
		gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (choice_swatch), 22);
		gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (choice_swatch), 22);
		g_object_set_data_full (G_OBJECT (choice_swatch), "xpad-fixed-color", gdk_rgba_copy (&rgba), (GDestroyNotify) gdk_rgba_free);
		gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (choice_swatch), color_swatch_draw, toolbar, NULL);
		gtk_button_set_child (GTK_BUTTON (choice), choice_swatch);

		g_object_set_data_full (G_OBJECT (choice), "xpad-color", gdk_rgba_copy (&rgba), (GDestroyNotify) gdk_rgba_free);
		g_object_set_data (G_OBJECT (choice), "xpad-menu-button", item);
		g_signal_connect (choice, "clicked", G_CALLBACK (color_chosen), toolbar);

		gtk_box_append (GTK_BOX (row), choice);
	}

	GtkWidget *popover = gtk_popover_new ();
	gtk_popover_set_child (GTK_POPOVER (popover), row);
	g_signal_connect (popover, "map", G_CALLBACK (color_popover_mapped), toolbar);
	g_signal_connect (popover, "closed", G_CALLBACK (color_popover_closed), toolbar);
	gtk_menu_button_set_popover (GTK_MENU_BUTTON (item), popover);

	return item;
}

/* GTK 4 caches per-widget render nodes; invalidating the toolbar does not
   re-snapshot an unchanged child drawing area, so the pad pokes the swatch
   directly whenever its color changes. */
void
xpad_toolbar_update_color_swatch (XpadToolbar *toolbar)
{
	GtkWidget *item = GTK_WIDGET (g_object_get_data (G_OBJECT (toolbar), "Color"));
	if (item)
		gtk_widget_queue_draw (gtk_menu_button_get_child (GTK_MENU_BUTTON (item)));
}

static GtkWidget *
xpad_toolbar_button_to_item (XpadToolbar *toolbar, const XpadToolbarButton *button)
{
	GtkWidget *item = GTK_WIDGET (g_object_get_data (G_OBJECT (toolbar), button->name));

	if (item)
		return item;

	switch (button->type)
	{
	case XPAD_BUTTON_TYPE_BUTTON:
	case XPAD_BUTTON_TYPE_TOGGLE:
		item = gtk_button_new_from_icon_name (button->icon_name);
		gtk_widget_add_css_class (item, "flat");
		g_signal_connect (item, "clicked", G_CALLBACK (xpad_toolbar_button_activated), NULL);
		break;
	case XPAD_BUTTON_TYPE_COLOR:
		item = xpad_toolbar_make_color_item (toolbar);
		break;
	case XPAD_BUTTON_TYPE_SEPARATOR:
		item = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
		break;
	default:
		return NULL;
	}

	g_object_set_data (G_OBJECT (item), "xpad-toolbar", toolbar);
	g_object_set_data (G_OBJECT (item), "xpad-tb", (gpointer) button);

	g_object_set_data (G_OBJECT (toolbar), button->name, item);

	if (button->desc)
		gtk_widget_set_tooltip_text (item, _(button->desc));

	return item;
}

static void
xpad_toolbar_button_activated (GtkButton *button)
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
	const GSList *slist, *stemp;
	guint i = 0, j = 0;
	GtkWidget *item, *child;
	XpadSettings *settings;

	/* Remove every existing child from the box. */
	while ((child = gtk_widget_get_first_child (GTK_WIDGET (toolbar))) != NULL)
		gtk_box_remove (GTK_BOX (toolbar), child);

	for (j = 0; j < G_N_ELEMENTS (buttons); j++)
		g_object_set_data (G_OBJECT (toolbar), buttons[j].name, NULL);

	g_object_get (toolbar->priv->pad, "settings", &settings, NULL);

	slist = xpad_settings_get_toolbar_buttons (settings);

	for (stemp = slist; stemp; stemp = stemp->next)
	{
		const XpadToolbarButton *button;

		button = xpad_toolbar_button_lookup (toolbar, stemp->data);

		/* An unknown button name in default-style (hand edit, downgrade,
		   future rename) must not crash the app at startup. */
		if (!button)
		{
			g_warning ("Ignoring unknown toolbar button '%s' in settings.", (const gchar *) stemp->data);
			continue;
		}

		if (button->type == XPAD_BUTTON_TYPE_SEPARATOR)
			item = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
		else if (button->type == XPAD_BUTTON_TYPE_BUTTON || button->type == XPAD_BUTTON_TYPE_COLOR)
			item = xpad_toolbar_button_to_item (toolbar, button);
		else
			continue;

		if (item)
		{
			g_object_set_data (G_OBJECT (item), "xpad-button-num", GINT_TO_POINTER (i));
			gtk_box_append (GTK_BOX (toolbar), item);
			i++;
		}
	}

	/* The resize grip is no longer part of the toolbar; the pad overlays it in
	   its own bottom-trailing corner so the pill stays a pure button cluster. */

	if (toolbar->priv->pad)
	{
		xpad_pad_notify_has_selection (toolbar->priv->pad);
		xpad_pad_notify_clipboard_owner_changed (toolbar->priv->pad);
	}
}

static void
xpad_toolbar_add_button (GtkButton *menu_button, XpadSettings *settings)
{
	const gchar *name = g_object_get_data (G_OBJECT (menu_button), "xpad-add-name");
	xpad_settings_add_toolbar_button (settings, name);
}

static void
menu_closed (GtkPopover *popover, XpadToolbar *toolbar)
{
	g_signal_emit (toolbar, signals[POPDOWN], 0, popover);
	/* The popover is single-shot; drop it once it closes. */
	gtk_widget_unparent (GTK_WIDGET (popover));
}

/* Helper: append a flat, left-aligned button acting as a menu row. */
static GtkWidget *
context_menu_row (GtkBox *box, const gchar *mnemonic)
{
	GtkWidget *row = gtk_button_new_with_mnemonic (mnemonic);
	gtk_button_set_has_frame (GTK_BUTTON (row), FALSE);
	gtk_widget_set_halign (gtk_button_get_child (GTK_BUTTON (row)), GTK_ALIGN_START);
	gtk_box_append (box, row);
	return row;
}

/* Secondary-click handler: builds and pops up the add/remove-buttons menu as a
   GtkPopover anchored at the click position. Replaces the GtkToolbar
   popup_context_menu vfunc + GtkMenu used under GTK 3. */
static void
xpad_toolbar_right_click (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer data)
{
	(void) n_press;

	XpadToolbar *toolbar = XPAD_TOOLBAR (data);
	guint i;
	gboolean is_button = FALSE;
	XpadSettings *settings;

	g_object_get (toolbar->priv->pad, "settings", &settings, NULL);
	const GSList *current_buttons = xpad_settings_get_toolbar_buttons (settings);

	GtkWidget *popover = gtk_popover_new ();
	gtk_widget_set_parent (popover, GTK_WIDGET (toolbar));
	gtk_popover_set_has_arrow (GTK_POPOVER (popover), FALSE);
	gtk_popover_set_pointing_to (GTK_POPOVER (popover),
	                             &(const GdkRectangle){ (int) x, (int) y, 1, 1 });

	GtkBox *box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
	gtk_popover_set_child (GTK_POPOVER (popover), GTK_WIDGET (box));

	for (i = 0; i < G_N_ELEMENTS (buttons); i++)
	{
		const GSList *j;
		GtkWidget *item;

		for (j = current_buttons; j; j = j->next)
			if (g_ascii_strcasecmp (j->data, "Separator") && !g_ascii_strcasecmp (j->data, buttons[i].name))
				break;

		if (j)
		{
			is_button = TRUE;
			continue;
		}

		item = context_menu_row (box, buttons[i].menu_desc);
		/* Carry the canonical button name on the row instead of abusing accel_path. */
		g_object_set_data (G_OBJECT (item), "xpad-add-name", (gpointer) buttons[i].name);
		g_signal_connect (item, "clicked", G_CALLBACK (xpad_toolbar_add_button), settings);
		g_signal_connect_swapped (item, "clicked", G_CALLBACK (gtk_popover_popdown), popover);
	}

	if (is_button)
	{
		GtkWidget *item;

		item = context_menu_row (box, N_("Remove All _Buttons"));
		g_signal_connect_swapped (item, "clicked", G_CALLBACK (xpad_settings_remove_all_toolbar_buttons), settings);
		g_signal_connect_swapped (item, "clicked", G_CALLBACK (gtk_popover_popdown), popover);

		item = context_menu_row (box, N_("Remo_ve Last Button"));
		g_signal_connect_swapped (item, "clicked", G_CALLBACK (xpad_settings_remove_last_toolbar_button), settings);
		g_signal_connect_swapped (item, "clicked", G_CALLBACK (gtk_popover_popdown), popover);
	}

	g_signal_connect (popover, "closed", G_CALLBACK (menu_closed), toolbar);

	g_signal_emit (toolbar, signals[POPUP], 0, popover);
	gtk_popover_popup (GTK_POPOVER (popover));
}

static void
xpad_toolbar_enable_button (XpadToolbar *toolbar, const XpadToolbarButton *button, gboolean enable)
{
	g_return_if_fail (button);
	/* Only touch buttons actually present on the toolbar; creating items here
	   would orphan widgets for buttons the user removed. */
	GtkWidget *item = GTK_WIDGET (g_object_get_data (G_OBJECT (toolbar), button->name));
	if (item)
		gtk_widget_set_sensitive (item, enable);
}

void
xpad_toolbar_enable_undo_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Undo");
	xpad_toolbar_enable_button (toolbar, button, enable);
}

void
xpad_toolbar_enable_redo_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Redo");
	xpad_toolbar_enable_button (toolbar, button, enable);
}

void
xpad_toolbar_enable_cut_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Cut");
	xpad_toolbar_enable_button (toolbar, button, enable);
}

void
xpad_toolbar_enable_copy_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Copy");
	xpad_toolbar_enable_button (toolbar, button, enable);
}

void
xpad_toolbar_enable_paste_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Paste");
	xpad_toolbar_enable_button (toolbar, button, enable);
}
