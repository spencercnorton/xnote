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
#include "toolbar.h"
#include "pad.h"
#include "settings.h"

static gboolean
grip_expose_handler (GtkWidget *widget, GdkEventExpose *event, xpad_toolbar *xt)
{
	gtk_paint_resize_grip (
		widget->style,
		widget->window,
		GTK_WIDGET_STATE (widget),
		NULL,
		widget,
		"xpad-grip",
		GDK_WINDOW_EDGE_SOUTH_EAST,
		0, 0,
		xt->height - xt->bar->style->xthickness,
		xt->height - xt->bar->style->ythickness);
	
	return TRUE;
}

static GtkWidget *
toolbar_get_box (GtkWidget *toolbar)
{
	return gtk_container_get_children (GTK_CONTAINER (toolbar))->data;
}

static void
toolbar_set_visible (xpad_toolbar *xt, gboolean v)
{
	xt->visible = v;
}

gboolean
toolbar_is_visible (xpad_toolbar *xt)
{
	return xt && xt->visible;
}

void
toolbar_show (pad_node *pad)
{
	if (!toolbar_is_visible (pad->toolbar))
	{
		pad->height += pad->toolbar->height;
		gtk_window_resize (pad->window, pad->width, pad->height);
		gtk_widget_show_all (pad->toolbar->bar);
		
		toolbar_set_visible (pad->toolbar, TRUE);
	}
}

void
toolbar_hide (pad_node *pad)
{
	if (toolbar_is_visible (pad->toolbar))
	{
		pad->height -= pad->toolbar->height;
		gtk_widget_hide (pad->toolbar->bar);
		gtk_window_resize (pad->window, pad->width, pad->height);
		
		toolbar_set_visible (pad->toolbar, FALSE);
	}
}

static gboolean
toolbar_hide_timeout (gpointer data)
{
	pad_node *pad = (pad_node *) data;
	
	if (!pad->toolbar->timeout)
		return FALSE;
	
	if (toolbar_is_visible (pad->toolbar))
	{
		pad->toolbar->timeout = 0;
		
		toolbar_hide (pad);
	}
	
	return FALSE;
}

void
toolbar_start_timeout (pad_node *pad)
{
	if (!pad->toolbar->timeout)
		pad->toolbar->timeout = g_timeout_add (1000, toolbar_hide_timeout, pad);
}

void
toolbar_end_timeout (pad_node *pad)
{
	pad->toolbar->timeout = 0;
}

GtkToolItem *
toolbar_button_new (const toolbar_button *tb, GtkTooltips *tooltips)
{
	GtkToolItem *button;
	
	switch (tb->type)
	{
	default:
	case 0:
		button = gtk_tool_button_new_from_stock (tb->stock);
		break;
	case 1:
		button = gtk_toggle_tool_button_new_from_stock (tb->stock);
		break;
	}
	
	gtk_tool_item_set_tooltip (button, tooltips, _(tb->desc), _(tb->desc));
	
	g_object_set_data (G_OBJECT (button), "tb", (void *) tb);
	
	return button;
}

GtkToolItem *
toolbar_separator_new ()
{
	GtkToolItem *sep;
	
	sep = gtk_separator_tool_item_new ();
	
	return sep;
}

static void
toolbar_add_separator (xpad_toolbar *xt)
{
	GtkToolItem *sep;
	
	sep = toolbar_separator_new ();
	
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar_get_box (xt->bar)),
		sep, -1);
}

static void
toolbar_add_button (xpad_toolbar *xt, const toolbar_button *tb)
{
	GtkToolItem *button;
	
	button = toolbar_button_new (tb, xt->tooltips);
	
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar_get_box (xt->bar)),
		button, -1);
}

static void
toolbar_add_item (gpointer gtb, gpointer gxt)
{
	const toolbar_button *tb = (const toolbar_button *) gtb;
	xpad_toolbar *xt = (xpad_toolbar *) gxt;
	
	if (!g_ascii_strcasecmp (tb->name, "sep"))
	{
		toolbar_add_separator (xt);
	}
	else
	{
		toolbar_add_button (xt, tb);
	}
}

GList *toolbar_get_children (xpad_toolbar *xt)
{
	return gtk_container_get_children (GTK_CONTAINER (
		toolbar_get_box (xt->bar)));
}

gboolean toolbar_is_button (GtkWidget *widget)
{
	const gchar *type;
	
	type = GTK_OBJECT_TYPE_NAME (GTK_OBJECT (widget));
	
	return !strcmp (type, "GtkToolButton") || !strcmp (type, "GtkToggleToolButton");
}

GList *toolbar_get_buttons (xpad_toolbar *xt)
{
	GList *list, *tmp;
	GList *rv = NULL;

	if (!xt) return NULL;

	list = toolbar_get_children (xt);
	
	for (tmp = list; tmp; tmp = tmp->next)
	{
		GtkWidget *widget = GTK_WIDGET (tmp->data);
		/*
		GtkWidget *button = gtk_bin_get_child (GTK_BIN (eb));
		*/
		if (toolbar_is_button (widget))
			rv = g_list_append (rv, (void *) widget);
	}
	
	g_list_free (list);
	
	return rv;
}

GtkWidget *
toolbar_get_container (xpad_toolbar *xt)
{
	return gtk_container_get_children (GTK_CONTAINER (xt->bar))->data;
}

void
toolbar_update (xpad_toolbar *xt)
{
	GtkRequisition req;
	GList *list, *temp;
	GSList *slist;
	GtkWidget *box;
	
	if (!xt)
		return;
	
	box = toolbar_get_container (xt);
	
	list = gtk_container_get_children (GTK_CONTAINER (box));
	
	for (temp = list; temp; temp = temp->next)
		gtk_container_remove (GTK_CONTAINER (box), temp->data);
	
	g_list_free (list);
	
	slist = xpad_settings_get_toolbar_buttons ();
	g_slist_foreach (slist, toolbar_add_item, xt);
	g_slist_free (slist);
	
	gtk_widget_show_all (box);
	
	gtk_widget_realize (xt->bar);
	
	gtk_widget_size_request (xt->bar, &req);
	
	req.height = MAX (req.height, 18);	/* must make grip at least 18 pixels */
	
	gtk_widget_set_size_request (gtk_bin_get_child (GTK_BIN (xt->grip)), req.height, req.height);
	
	xt->height = req.height;
	xt->timeout = 0;
}


xpad_toolbar *toolbar_new (void)
{
	GtkWidget *toolbar = gtk_toolbar_new ();
	GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
	GtkWidget *grip = gtk_drawing_area_new ();
	GtkWidget *align = gtk_alignment_new (1, 1, 1, 1);
	xpad_toolbar *xt = (xpad_toolbar *) g_malloc (sizeof (xpad_toolbar));
	
	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), align, FALSE, FALSE, 0);
	
	gtk_widget_add_events (grip, GDK_BUTTON_PRESS_MASK);
	g_signal_connect (G_OBJECT (grip), "expose-event", 
		G_CALLBACK (grip_expose_handler), xt);
	
	gtk_container_add (GTK_CONTAINER (align), grip);
	
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
	
	xt->visible = FALSE;
	xt->bar = hbox;
	xt->grip = align;
	xt->timeout = 0;
	xt->tooltips = gtk_tooltips_new ();
	
	return xt;
}
