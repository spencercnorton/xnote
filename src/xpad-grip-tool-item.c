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

#include <gtk/gtk.h>

#include "xpad-grip-tool-item.h"
#include "xpad-pad.h"

struct XpadGripToolItemPrivate
{
	int unused;
};

G_DEFINE_TYPE_WITH_PRIVATE(XpadGripToolItem, xpad_grip_tool_item, GTK_TYPE_DRAWING_AREA)

static void xpad_grip_tool_item_draw (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
static void xpad_grip_tool_item_pressed (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer data);

GtkWidget *
xpad_grip_tool_item_new (void)
{
	return GTK_WIDGET (g_object_new (XPAD_TYPE_GRIP_TOOL_ITEM, NULL));
}

static void
xpad_grip_tool_item_class_init (XpadGripToolItemClass *klass)
{
	(void) klass;
}

static void
xpad_grip_tool_item_init (XpadGripToolItem *grip)
{
	grip->priv = xpad_grip_tool_item_get_instance_private(grip);

	gtk_widget_set_size_request (GTK_WIDGET (grip), 18, 18);
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (grip), xpad_grip_tool_item_draw, NULL, NULL);

	/* Corner-resize cursor. GTK 4 sets cursors by CSS name on the widget. */
	if (gtk_widget_get_direction (GTK_WIDGET (grip)) == GTK_TEXT_DIR_LTR)
		gtk_widget_set_cursor_from_name (GTK_WIDGET (grip), "se-resize");
	else
		gtk_widget_set_cursor_from_name (GTK_WIDGET (grip), "sw-resize");

	/* GTK 4 uses event controllers instead of the button-press-event signal. */
	GtkGesture *click = gtk_gesture_click_new ();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), GDK_BUTTON_PRIMARY);
	g_signal_connect (click, "pressed", G_CALLBACK (xpad_grip_tool_item_pressed), grip);
	gtk_widget_add_controller (GTK_WIDGET (grip), GTK_EVENT_CONTROLLER (click));
}

/* Starts an interactive resize of the pad window. */
static void
xpad_grip_tool_item_pressed (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer data)
{
	(void) n_press;
	(void) x;
	(void) y;

	GtkWidget *grip = GTK_WIDGET (data);

	gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
	xpad_pad_begin_window_drag (grip, gesture, FALSE);
}

static void
xpad_grip_tool_item_draw (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
	(void) data;

	/* gtk_render_handle() draws nothing under modern themes, so render the
	   classic diagonal grip lines ourselves in the theme's foreground color. */
	GdkRGBA color;
	gboolean ltr = gtk_widget_get_direction (GTK_WIDGET (area)) != GTK_TEXT_DIR_RTL;
	int i;

	gtk_widget_get_color (GTK_WIDGET (area), &color);
	color.alpha = 0.55f;
	gdk_cairo_set_source_rgba (cr, &color);
	cairo_set_line_width (cr, 1.5);

	for (i = 1; i <= 3; i++)
	{
		double off = i * 4.5;
		if (ltr)
		{
			cairo_move_to (cr, width - off, height - 1.5);
			cairo_line_to (cr, width - 1.5, height - off);
		}
		else
		{
			cairo_move_to (cr, off, height - 1.5);
			cairo_line_to (cr, 1.5, height - off);
		}
	}
	cairo_stroke (cr);
}
