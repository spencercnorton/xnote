#include "toolbar.h"
#include "pad.h"
#include "main.h"
#include <string.h>

static gboolean
grip_press_handler (GtkWidget *widget, GdkEventButton *event, pad_node *pad)
{
	if (event->button == 1)
		gtk_window_begin_resize_drag (pad->window,
			GDK_WINDOW_EDGE_SOUTH_EAST,
			event->button,
			event->x_root, event->y_root,
			event->time);
	else
		gtk_window_begin_move_drag (pad->window,
			event->button,
			event->x_root, event->y_root,
			event->time);
	
	return TRUE;
}

static void
get_grip_rect (GtkWidget *widget,
               GdkRectangle *rect)
{
  gint w, h;
  
  /* These are in effect the max/default size of the grip. */
  w = 18;
  h = 18;

  if (w > (widget->allocation.width))
    w = widget->allocation.width;

  if (h > (widget->allocation.height - widget->style->ythickness))
    h = widget->allocation.height - widget->style->ythickness;
  
  rect->x = widget->allocation.x + widget->allocation.width - w;
  rect->y = widget->allocation.y + widget->allocation.height - h;
  rect->width = w;
  rect->height = h;
}

static gboolean
grip_expose_handler (GtkWidget *widget, GdkEventExpose *event, pad_node *pad)
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
		pad->toolbar_height - pad->toolbar->style->xthickness,
		pad->toolbar_height - pad->toolbar->style->ythickness);
	
	return TRUE;
}

static GtkWidget *
toolbar_get_box (GtkWidget *toolbar)
{
	return gtk_container_get_children (GTK_CONTAINER (toolbar))->data;
}

void
toolbar_show (pad_node *pad)
{
	if (!GTK_WIDGET_VISIBLE (pad->toolbar))
	{
		gtk_window_resize (pad->window, pad->width, pad->height + pad->toolbar_height);
		gtk_widget_show_all (pad->toolbar);
	}
}

int
toolbar_hide (gpointer data)
{
	pad_node *pad = (pad_node *) data;
	
	if (GTK_WIDGET_VISIBLE (pad->toolbar))
	{
		pad->toolbar_timeout = 0;
		
		gtk_widget_hide (pad->toolbar);
		gtk_window_resize (pad->window, pad->width, pad->height - pad->toolbar_height);
	}
	
	return 0;
}

void
toolbar_start_timeout (pad_node *pad)
{
	pad->toolbar_timeout = gtk_timeout_add (1000, toolbar_hide, pad);
}

void
toolbar_end_timeout (pad_node *pad)
{
	gtk_timeout_remove (pad->toolbar_timeout);
	pad->toolbar_timeout = 0;
}


static void
toolbar_add_button (const gchar *name, pad_node *pad)
{
	const toolbar_button *tb = NULL;
	gint i;
	GtkWidget *box = toolbar_get_box (pad->toolbar);
	
	for (i = 0; i < NUM_BUTTONS; i++)
	{
		if (!g_ascii_strcasecmp (buttons[i].name, name))
		{
			tb = &buttons[i];
			break;
		}
	}
	
	if (i == NUM_BUTTONS)
		return;
	
	gtk_toolbar_insert_stock (GTK_TOOLBAR (box),
		tb->stock, tb->desc, tb->desc, tb->func, pad, -1);
}

static void
toolbar_add_separator (pad_node *pad)
{
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar_get_box (pad->toolbar)));
}

static void
toolbar_add_item (gpointer p1, gpointer p2)
{
	const gchar *name = (const gchar *) p1;
	pad_node *pad = (pad_node *) p2;
	
	if (!g_ascii_strcasecmp (name, "sep"))
	{
		toolbar_add_separator (pad);
	}
	else
	{
		toolbar_add_button (name, pad);
	}
}

static void
toolbar_show_item (GtkWidget *widget, gpointer p2)
{
	gtk_widget_show_all (widget);
}

void
toolbar_update (pad_node *pad)
{
	GtkRequisition req;
	GList *list, *temp;
	GtkWidget *box = toolbar_get_box (pad->toolbar);
	
	if (verbosity >= 2) printf ("Updating toolbar.\n");
	
	list = temp = gtk_container_get_children (GTK_CONTAINER (box));
	
	while (temp)
	{
		gtk_container_remove (GTK_CONTAINER (box), temp->data);
		
		temp = temp->next;
	}
	
	g_list_free (list);
	
	g_slist_foreach (current_settings.toolbar, toolbar_add_item, pad);
	
	gtk_widget_show_all (box);
	
	gtk_toolbar_set_icon_size (GTK_TOOLBAR (box), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_set_style (GTK_TOOLBAR (box), GTK_TOOLBAR_ICONS);
	
	gtk_widget_realize (pad->toolbar);
	
	gtk_widget_size_request (pad->toolbar, &req);
	
	req.height = MAX (req.height, 18);	/* must make grip at least 18 pixels */
	
	gtk_widget_set_size_request (gtk_bin_get_child (GTK_BIN (pad->grip)), req.height, req.height);
	
	pad->toolbar_height = req.height;
	pad->toolbar_timeout = 0;
}

void toolbar_add_to_pad (pad_node *pad)
{
	GtkWidget *toolbar = gtk_toolbar_new ();
	GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
	GtkWidget *grip = gtk_drawing_area_new ();
	GtkWidget *align = gtk_alignment_new (1, 1, 1, 1);
	
	if (verbosity >= 2) printf ("Adding toolbar to pad.\n");
	
	gtk_box_pack_end (GTK_BOX (pad->box), hbox, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), align, FALSE, FALSE, 0);
	
	gtk_widget_add_events (grip, GDK_BUTTON_PRESS_MASK);
	g_signal_connect (G_OBJECT (grip), "expose-event", 
		G_CALLBACK (grip_expose_handler), pad);
	g_signal_connect (G_OBJECT (grip), "button-press-event", 
		G_CALLBACK (grip_press_handler), pad);
	
	gtk_container_add (GTK_CONTAINER (align), grip);
	
	pad->toolbar = hbox;
	pad->grip = align;
}
