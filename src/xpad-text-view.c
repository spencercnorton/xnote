/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2013-2014 Arthur Borsboom

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

#include <gtk/gtk.h>
#include "xpad-text-view.h"
#include "xpad-text-buffer.h"
#include "xpad-app.h"

struct XpadTextViewPrivate 
{
	gboolean follow_font_style;
	gboolean follow_color_style;
	gulong notify_text_handler;
	gulong notify_back_handler;
	gulong notify_font_handler;
	XpadTextBuffer *buffer;
};

G_DEFINE_TYPE_WITH_PRIVATE(XpadTextView, xpad_text_view, GTK_TYPE_TEXT_VIEW)

static void xpad_text_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_text_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_text_view_dispose (GObject *object);
static void xpad_text_view_finalize (GObject *object);
static void xpad_text_view_realize (XpadTextView *widget);
static gboolean xpad_text_view_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gboolean xpad_text_view_focus_out_event (GtkWidget *widget, GdkEventFocus *event);
static void xpad_text_view_notify_edit_lock (XpadTextView *view);
static void xpad_text_view_notify_editable (XpadTextView *view);
static void xpad_text_view_notify_fontname (XpadTextView *view);
static void xpad_text_view_notify_colors (XpadTextView *view);

enum
{
  PROP_0,
  PROP_FOLLOW_FONT_STYLE,
  PROP_FOLLOW_COLOR_STYLE,
  LAST_PROP
};

GtkWidget *
xpad_text_view_new (void)
{
	return GTK_WIDGET (g_object_new (XPAD_TYPE_TEXT_VIEW, "follow-font-style", TRUE, "follow-color-style", TRUE, NULL));
}

static void
xpad_text_view_class_init (XpadTextViewClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = xpad_text_view_dispose;
	gobject_class->finalize = xpad_text_view_finalize;
	gobject_class->set_property = xpad_text_view_set_property;
	gobject_class->get_property = xpad_text_view_get_property;
	
	/* Properties */
	
	g_object_class_install_property (gobject_class,
	                                 PROP_FOLLOW_FONT_STYLE,
	                                 g_param_spec_boolean ("follow-font-style",
	                                                       "Follow Font Style",
	                                                       "Whether to use the default xpad font style",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_FOLLOW_COLOR_STYLE,
	                                 g_param_spec_boolean ("follow-color-style",
	                                                       "Follow Color Style",
	                                                       "Whether to use the default xpad color style",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
}

static void
xpad_text_view_init (XpadTextView *view)
{
	gchar *name;
	
	view->priv = xpad_text_view_get_instance_private(view);
	
	view->priv->follow_font_style = TRUE;
	view->priv->follow_color_style = TRUE;
	
	view->priv->buffer = xpad_text_buffer_new();
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (view), GTK_TEXT_BUFFER (view->priv->buffer));
	
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);
	gtk_container_set_border_width (GTK_CONTAINER (view), 5);
	
	name = g_strdup_printf ("%p", (void *) view);
	gtk_widget_set_name (GTK_WIDGET (view), name);
	g_free (name);
	
	g_signal_connect (view, "button-press-event", G_CALLBACK (xpad_text_view_button_press_event), NULL);
	g_signal_connect_after (view, "focus-out-event", G_CALLBACK (xpad_text_view_focus_out_event), NULL);
	g_signal_connect (view, "realize", G_CALLBACK (xpad_text_view_realize), NULL);
	g_signal_connect (view, "notify::editable", G_CALLBACK (xpad_text_view_notify_editable), NULL);
	g_signal_connect_swapped (xpad_global_settings, "notify::edit-lock", G_CALLBACK (xpad_text_view_notify_edit_lock), view);
	view->priv->notify_font_handler = g_signal_connect_swapped (xpad_global_settings, "notify::fontname", G_CALLBACK (xpad_text_view_notify_fontname), view);
	view->priv->notify_text_handler = g_signal_connect_swapped (xpad_global_settings, "notify::text-color", G_CALLBACK (xpad_text_view_notify_colors), view);
	view->priv->notify_back_handler = g_signal_connect_swapped (xpad_global_settings, "notify::back-color", G_CALLBACK (xpad_text_view_notify_colors), view);
	xpad_text_view_notify_colors (view);
	xpad_text_view_notify_fontname (view);
}

static void
xpad_text_view_dispose (GObject *object)
{
	XpadTextView *view = XPAD_TEXT_VIEW (object);

	if (view->priv->buffer) {
		g_object_unref (view->priv->buffer);
	}
	
	G_OBJECT_CLASS (xpad_text_view_parent_class)->dispose (object);
}

static void
xpad_text_view_finalize (GObject *object)
{
	XpadTextView *view = XPAD_TEXT_VIEW (object);

	if (xpad_global_settings)
		g_signal_handlers_disconnect_matched (xpad_global_settings, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, view);

	G_OBJECT_CLASS (xpad_text_view_parent_class)->finalize (object);
}

static void
xpad_text_view_realize (XpadTextView *view)
{
	gboolean edit_lock;
	g_object_get (xpad_global_settings, "edit-lock", &edit_lock, NULL);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), !edit_lock);
}

static gboolean
xpad_text_view_focus_out_event (GtkWidget *widget, GdkEventFocus *event)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) event;

	gboolean edit_lock;
	g_object_get (xpad_global_settings, "edit-lock", &edit_lock, NULL);

	if (edit_lock)
	{
		gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
xpad_text_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	gboolean edit_lock;
	g_object_get (xpad_global_settings, "edit-lock", &edit_lock, NULL);

	if (event->button == 1 &&
	    edit_lock &&
	    !gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)))
	{
		if (event->type == GDK_2BUTTON_PRESS)
		{
			gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), TRUE);
			return TRUE;
		}
		else if (event->type == GDK_BUTTON_PRESS)
		{
			gtk_window_begin_move_drag (GTK_WINDOW (gtk_widget_get_toplevel (widget)), (gint) event->button, (gint) event->x_root, (gint) event->y_root, event->time);
			return TRUE;
		}
	}
	
	return FALSE;
}

static void
xpad_text_view_notify_edit_lock (XpadTextView *view)
{
	/* chances are good that they don't have the text view focused while it changed, so make non-editable if edit lock turned on */
	gboolean edit_lock;
	g_object_get (xpad_global_settings, "edit-lock", &edit_lock, NULL);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), !edit_lock);
}

static void
xpad_text_view_notify_editable (XpadTextView *view)
{
	GdkCursor *cursor;
	gboolean editable;
	GdkWindow *view_window;
	
	editable = gtk_text_view_get_editable (GTK_TEXT_VIEW (view));
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), editable);
	
	cursor = editable ? gdk_cursor_new (GDK_XTERM) : NULL;
	
	/* Only set for pads which are currently visible */ 
	view_window = gtk_text_view_get_window (GTK_TEXT_VIEW (view), GTK_TEXT_WINDOW_TEXT);
	if (view_window != NULL)
		gdk_window_set_cursor (view_window, cursor);
	
	if (cursor)
		g_object_unref (cursor);
}

static void
xpad_text_view_notify_fontname (XpadTextView *view)
{
	const gchar *font;
	g_object_get (xpad_global_settings, "fontname", &font, NULL);
	PangoFontDescription *fontdesc;
	
	fontdesc = font ? pango_font_description_from_string (font) : NULL;
	gtk_widget_override_font (GTK_WIDGET (view), fontdesc);
	if (fontdesc)
		pango_font_description_free (fontdesc);
}

/* Update the colors of the textview */
static void
xpad_text_view_notify_colors (XpadTextView *view)
{
	/* Set the colors of this individual pad to the global setting preference. */
	const GdkRGBA *text_color, *back_color;
	g_object_get (xpad_global_settings, "text-color", &text_color, "back-color", &back_color, NULL);

	gtk_widget_override_cursor (GTK_WIDGET (view), text_color, text_color);
	gtk_widget_override_color (GTK_WIDGET (view), GTK_STATE_FLAG_NORMAL, text_color);
	gtk_widget_override_background_color (GTK_WIDGET (view), GTK_STATE_FLAG_NORMAL, back_color);

	/* Inverse the text and background colors for selected text, so it is likely to be visible by any choice of the colors. */
	gtk_widget_override_color (GTK_WIDGET (view), GTK_STATE_FLAG_SELECTED, back_color);
	gtk_widget_override_background_color (GTK_WIDGET (view), GTK_STATE_FLAG_SELECTED, text_color);
}

void
xpad_text_view_set_follow_font_style (XpadTextView *view, gboolean follow)
{
	g_return_if_fail (view);

	if (follow != view->priv->follow_font_style)
	{
		if (follow)
		{
			g_signal_handler_unblock (xpad_global_settings, view->priv->notify_font_handler);
			xpad_text_view_notify_fontname (view);
		}
		else
		{
			g_signal_handler_block (xpad_global_settings, view->priv->notify_font_handler);
		}
	}
	
	view->priv->follow_font_style = follow;
	
	g_object_notify (G_OBJECT (view), "follow_font_style");
}

gboolean
xpad_text_view_get_follow_font_style (XpadTextView *view)
{
	if (view == NULL)
		return TRUE;
	else
		return view->priv->follow_font_style;
}

void
xpad_text_view_set_follow_color_style (XpadTextView *view, gboolean follow)
{
	g_return_if_fail (view);

	if (follow != view->priv->follow_color_style)
	{
		if (follow)
		{
			xpad_text_view_notify_colors (view);
			g_signal_handler_unblock (xpad_global_settings, view->priv->notify_text_handler);
			g_signal_handler_unblock (xpad_global_settings, view->priv->notify_back_handler);
		}
		else
		{
			g_signal_handler_block (xpad_global_settings, view->priv->notify_text_handler);
			g_signal_handler_block (xpad_global_settings, view->priv->notify_back_handler);
		}

		view->priv->follow_color_style = follow;
	}

	g_object_notify (G_OBJECT (view), "follow_color_style");
}

gboolean
xpad_text_view_get_follow_color_style (XpadTextView *view)
{
	if (view == NULL)
		return TRUE;
	else
		return view->priv->follow_color_style;
}

static void
xpad_text_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadTextView *view;
	
	view = XPAD_TEXT_VIEW (object);
	
	switch (prop_id)
	{
	case PROP_FOLLOW_FONT_STYLE:
		xpad_text_view_set_follow_font_style (view, g_value_get_boolean (value));
		break;
	
	case PROP_FOLLOW_COLOR_STYLE:
		xpad_text_view_set_follow_color_style (view, g_value_get_boolean (value));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_text_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadTextView *view;
	
	view = XPAD_TEXT_VIEW (object);
	
	switch (prop_id)
	{
	case PROP_FOLLOW_FONT_STYLE:
		g_value_set_boolean (value, xpad_text_view_get_follow_font_style (view));
		break;
	
	case PROP_FOLLOW_COLOR_STYLE:
		g_value_set_boolean (value, xpad_text_view_get_follow_color_style (view));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

XpadPad *xpad_text_view_get_pad (XpadTextView *view)
{
	return xpad_text_buffer_get_pad (view->priv->buffer);
}

void xpad_text_view_set_pad (XpadTextView *view, XpadPad *pad)
{
	xpad_text_buffer_set_pad (view->priv->buffer, pad);
}
