#include "toolbar.h"
#include "pad.h"
#include "main.h"
#include <string.h>

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

void
toolbar_show (pad_node *pad)
{
	if (!GTK_WIDGET_VISIBLE (pad->toolbar->bar))
	{
		gtk_window_resize (pad->window, pad->width, pad->height + pad->toolbar->height);
		gtk_widget_show_all (pad->toolbar->bar);
	}
}

int
toolbar_hide (gpointer data)
{
	pad_node *pad = (pad_node *) data;
	
	if (GTK_WIDGET_VISIBLE (pad->toolbar->bar))
	{
		pad->toolbar->timeout = 0;
		
		gtk_widget_hide (pad->toolbar->bar);
		gtk_window_resize (pad->window, pad->width, pad->height - pad->toolbar->height);
	}
	
	return 0;
}

void
toolbar_start_timeout (pad_node *pad)
{
	pad->toolbar->timeout = gtk_timeout_add (1000, toolbar_hide, pad);
}

void
toolbar_end_timeout (pad_node *pad)
{
	gtk_timeout_remove (pad->toolbar->timeout);
	pad->toolbar->timeout = 0;
}

GtkWidget *
toolbar_button_new (const toolbar_button *tb)
{
	GtkWidget *rv;
	GtkWidget *image;
	
	switch (tb->type)
	{
	default:
	case 0:
		rv = gtk_button_new ();
		break;
	case 1:
		rv = gtk_toggle_button_new ();
		break;
	}
	
	image = gtk_image_new_from_stock (tb->stock, 
		GTK_ICON_SIZE_SMALL_TOOLBAR);
	
	gtk_container_add (GTK_CONTAINER (rv), image);
	
	g_object_set_data (G_OBJECT (rv), "func", (void *) tb->func);
	
	return rv;
}

static void
toolbar_add_separator (xpad_toolbar *xt)
{
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar_get_box (xt->bar)));
/*	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar_get_box (xt->bar)),
		gtk_vseparator_new (), NULL, NULL);*/
}

static void
toolbar_add_button (xpad_toolbar *xt, const toolbar_button *tb)
{
	GtkWidget *button;
	
	button = toolbar_button_new (tb);
	
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar_get_box (xt->bar)),
		button, tb->desc, tb->desc);
}

void
toolbar_add_item (gpointer gname, gpointer gxt)
{
	const gchar *name = (const gchar *) gname;
	xpad_toolbar *xt = (xpad_toolbar *) gxt;
	
	if (!g_ascii_strcasecmp (name, "sep"))
	{
		toolbar_add_separator (xt);
	}
	else
	{
		gint i;
		const toolbar_button *tb = NULL;
		
		for (i = 0; i < NUM_BUTTONS; i++)
		{
			if (!g_ascii_strcasecmp (buttons[i].name, name))
			{
				tb = &buttons[i];
				break;
			}
		}
		
		if (!tb)
			return;
		
		toolbar_add_button (xt, tb);
	}
}

static void
toolbar_show_item (GtkWidget *widget, gpointer p2)
{
	gtk_widget_show_all (widget);
}

GList *toolbar_get_buttons (xpad_toolbar *xt)
{
	GList *list = gtk_container_get_children (GTK_CONTAINER (
		toolbar_get_box (xt->bar))), *tmp = list;
	
	while (tmp)
	{
		GtkWidget *widget = GTK_WIDGET (tmp->data);
		const gchar *type;
		
		type = GTK_OBJECT_TYPE_NAME (GTK_OBJECT (widget));
		
		if (strcmp (type, "GtkButton") && strcmp (type, "GtkToggleButton"))
		{
			GList *backup = tmp->next;
			
			g_list_remove (list, tmp->data);
			
			tmp = backup;
		}
		else
			tmp = tmp->next;
	}
	
	return list;
}

void
toolbar_update (xpad_toolbar *xt)
{
	GtkRequisition req;
	GList *list, *temp;
	GtkWidget *box = toolbar_get_box (xt->bar);
	
	if (verbosity >= 2) printf ("Updating toolbar.\n");
	
	list = temp = gtk_container_get_children (GTK_CONTAINER (box));
	
	while (temp)
	{
		gtk_container_remove (GTK_CONTAINER (box), temp->data);
		
		temp = temp->next;
	}
	
	g_list_free (list);
	
	g_slist_foreach (current_settings.toolbar, toolbar_add_item, xt);
	
	gtk_widget_show_all (box);
	
	gtk_toolbar_set_icon_size (GTK_TOOLBAR (box), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_set_style (GTK_TOOLBAR (box), GTK_TOOLBAR_ICONS);
	
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
	
	if (verbosity >= 2) printf ("Adding toolbar to pad.\n");
	
	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), align, FALSE, FALSE, 0);
	
	gtk_widget_add_events (grip, GDK_BUTTON_PRESS_MASK);
	g_signal_connect (G_OBJECT (grip), "expose-event", 
		G_CALLBACK (grip_expose_handler), xt);

	gtk_container_add (GTK_CONTAINER (align), grip);
	
	xt->bar = hbox;
	xt->grip = align;
	xt->timeout = 0;
	
	return xt;
}
