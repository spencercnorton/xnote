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

#include "main.h"
#include "pref.h"
#include "fio.h"
#include "pad.h"
#include "help.h"
#include "toolbar.h"
#include <string.h>

/* we keep a pointer around so that only one window will be open at a time */
GtkWidget *pref_window = NULL;
GSList *toolbar_widgets;
GtkWidget *pref_help_window = NULL;

static void
pref_help_close (void)
{
	if (pref_help_window)
		pref_help_window = NULL;
}

/**
 * Open a new help window corresponding to page |page| of the preference window.
 */
static GtkWidget *
help_window_new (GtkWidget *parent, gint page)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *notebook = gtk_notebook_new ();
	GtkWidget *buttonbox = gtk_hbutton_box_new ();
	GtkWidget *button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GtkWidget *label_back = gtk_label_new ("Background Color");
	GtkWidget *label_text = gtk_label_new ("Text Color");
	GtkWidget *label_border = gtk_label_new ("Border");
	GtkWidget *label_font = gtk_label_new ("Font Face");
	GtkWidget *label_misc = gtk_label_new ("Options");
	GtkWidget *label_toolbar = gtk_label_new ("Toolbar");
	GtkWidget *label_back_help = gtk_label_new ("");
	GtkWidget *label_text_help = gtk_label_new ("");
	GtkWidget *label_border_help = gtk_label_new ("");
	GtkWidget *label_font_help = gtk_label_new ("");
	GtkWidget *label_misc_help = gtk_label_new ("");
	GtkWidget *label_toolbar_help = gtk_label_new ("");
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 0);
	GtkWidget *vbox_background = gtk_vbox_new (FALSE, 3);
	GtkWidget *hbox_background = gtk_hbox_new (FALSE, 3);
	GtkWidget *vbox_text = gtk_vbox_new (FALSE, 3);
	GtkWidget *hbox_text = gtk_hbox_new (FALSE, 3);
	GtkWidget *vbox_border = gtk_vbox_new (FALSE, 3);
	GtkWidget *hbox_border = gtk_hbox_new (FALSE, 3);
	GtkWidget *vbox_font = gtk_vbox_new (FALSE, 3);
	GtkWidget *hbox_font = gtk_hbox_new (FALSE, 3);
	GtkWidget *vbox_misc = gtk_vbox_new (FALSE, 3);
	GtkWidget *hbox_misc = gtk_hbox_new (FALSE, 3);
	gchar bordertext[800];
	gchar fonttext[800];
	gchar toolbartext[700];
	gchar misctext[1600];
	
	gtk_container_set_border_width (GTK_CONTAINER (vbox_background), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_background), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_text), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_text), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_border), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_border), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_font), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_font), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_misc), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_misc), 6);
	
	gtk_window_set_title (GTK_WINDOW(window), "xpad Preferences Help");
	gtk_container_add (GTK_CONTAINER(window), vbox_global);
	
	/* buttonbox setup */
	g_signal_connect_swapped (GTK_OBJECT (button_close), "clicked", 
		G_CALLBACK (gtk_widget_destroy), (gpointer) window);
	g_signal_connect (GTK_OBJECT (window), "destroy", 
		G_CALLBACK (pref_help_close), NULL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(buttonbox), 0);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_close);
	gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 6);
	
	/* vbox_global setup */
	gtk_box_pack_start (GTK_BOX(vbox_global), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox_global), buttonbox, TRUE, TRUE, 0);
	
	/* text setup */
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), hbox_text, label_text);
	gtk_box_pack_start (GTK_BOX (hbox_text), vbox_text, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_text), label_text_help, FALSE, FALSE, 0);
	gtk_label_set_markup (GTK_LABEL (label_text_help),
"Select the <b>color</b> you would like the text of pads to be.  Either click on the "
"color wheel or enter values in the Red, Green, Blue text boxes to change the "
"color.\n\n"
"Changes will take effect immediately.  If you do not see a change, make sure that "
"there is text visible to be changed and that the pad does not have its style "
"locked (right click on pad, make sure that \"Lock Style\" is disabled).");
	gtk_label_set_line_wrap (GTK_LABEL (label_text_help), TRUE);

	/* background setup */
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), hbox_background, label_back);
	gtk_box_pack_start (GTK_BOX (hbox_background), vbox_background, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_background), label_back_help, FALSE, FALSE, 0);
	gtk_label_set_markup (GTK_LABEL (label_back_help),
"Select the <b>color</b> you would like the background of pads to be.  Either click on the "
"color wheel or enter values in the Red, Green, Blue text boxes to change the "
"color.\n\n"
"Changes will take effect immediately.  If you do not see a change, make sure that "
"the pad does not have its style locked (right click on pad, make sure that \"Lock Style\" is disabled).");
	gtk_label_set_line_wrap (GTK_LABEL (label_back_help), TRUE);

	/* border setup */
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), hbox_border, label_border);
	gtk_box_pack_start (GTK_BOX (hbox_border), vbox_border, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_border), label_border_help, FALSE, FALSE, 0);
	strcpy (bordertext,
"Select the <b>color</b> you would like the border of pads to be.  Either click on the "
"color wheel or enter values in the Red, Green, Blue text boxes to change the "
"color.  You cannot change the border color if the border width is zero.\n\n"
"You can also change the <b>size of the border</b> and the <b>amount of padding</b>.  The border ");
	strcat (bordertext,
"width controls how many pixels are drawn in an outline around the pad.  The padding "
"controls how many pixels are drawn between the border and the text of the pad.\n\n"
"Changes will take effect immediately.  If you do not see a change, make sure that "
"the pad does not have its style locked (right click on pad, make sure that \"Lock Style\" is disabled).");

	gtk_label_set_markup (GTK_LABEL (label_border_help), bordertext);
	gtk_label_set_line_wrap (GTK_LABEL (label_border_help), TRUE);

	/* font setup */
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), hbox_font, label_font);
	gtk_box_pack_start (GTK_BOX (hbox_font), vbox_font, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_font), label_font_help, FALSE, FALSE, 0);
	strcpy (fonttext,
"Select the <b>font face</b> you would like the text of pads to use.\n\n"
"The font <b>family</b> determines the general look and feel of text.  When some of the "
"available families are chosen, text will not be visible or will be garbled.  This means "
"the font family is not installed correctly.  Please choose another family.\n\n");
	strcat (fonttext,
"The font <b>style</b> controls whether the text is bold or italicized.\n\n"
"The font <b>size</b> controls how large the text is.\n\n"
"Changes will take effect immediately.  If you do not see a change, make sure that "
"there is text visible to be changed and that the pad does not have its style locked "
"(right click on pad, make sure that \"Lock Style\" is disabled).");
	gtk_label_set_markup (GTK_LABEL (label_font_help), fonttext);
	gtk_label_set_line_wrap (GTK_LABEL (label_font_help), TRUE);

	/* toolbar  setup */
	{
		GtkWidget *vbox_toolbar = gtk_vbox_new (FALSE, 3);
		GtkWidget *hbox_toolbar = gtk_hbox_new (FALSE, 3);
		
		gtk_container_set_border_width (GTK_CONTAINER (vbox_toolbar), 6);
		gtk_container_set_border_width (GTK_CONTAINER (hbox_toolbar), 6);
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_toolbar, label_toolbar);
		gtk_box_pack_start (GTK_BOX (hbox_toolbar), vbox_toolbar, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), label_toolbar_help, FALSE, FALSE, 0);
		strcpy (toolbartext,
"You can control whether pads have a <b>toolbar</b> or not by clicking on the "
"\"Enable toolbars\" option.  This affects all pads immediately.\n\n");
		strcat (toolbartext,
"If <b>auto-hide</b> is enabled, a pad's toolbar will disappear (after a small delay) when "
"your mouse pointer leaves the pad.  It will return when your mouse does.\n\n");
		strcat (toolbartext,
"If the toolbar is enabled, you can customize which <b>buttons</b> appear by dragging "
"the button you want from one box to another.  The upper box looks like a toolbar and "
"contains the buttons that are currently enabled.  The lower box holds unused buttons.  ");
		strcat (toolbartext,
"Dropping a button moves it to the end of the box you dropped it in.\n\n"
"If you don't know "
"what a button does, try hovering your mouse over it for a bit until a box appears, "
"describing the button.");
		gtk_label_set_markup (GTK_LABEL (label_toolbar_help), toolbartext);
		gtk_label_set_line_wrap (GTK_LABEL (label_toolbar_help), TRUE);
	}
	
	/* misc. setup */
	gtk_box_pack_start (GTK_BOX (hbox_misc), vbox_misc, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_misc), label_misc_help, FALSE, FALSE, 3);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_misc, label_misc);
	gtk_label_set_line_wrap (GTK_LABEL (label_misc_help), TRUE);
	strcpy (misctext,
"If <b>edit lock</b> is enabled, pads that are not in focus lose the ability to "
"be edited.  Rather, clicking and dragging on the surface of the pad will move it.  "
"To edit a pad that is locked, double click on its surface.  This will allow you to "
"change its text until the pad loses focus again.  If edit lock is not enabled, every "
"pad is always editable -- to move it, either drag on the border or hold down CTRL and "
"left click.  Edit lock is disabled by default.\n\n");
	strcat (misctext,
"If <b>confirm pad deletion</b> is enabled, a confirmation dialog will appear whenever "
"you delete a non-empty pad.  Deleting a pad loses the pad contents irrevocably.  Delete "
"confirmation is enabled by default.\n\n");
	strcat (misctext,
"If <b>allow scrollbars</b> is enabled, when a pad is smaller than the text area it "
"contains, scrollbars appear so you can view all the text.  If they are disabled, the "
"pad resizes to fit the text.\n\n");
	strcat (misctext,
"If <b>window decorations</b> are enabled, your window manager will draw a border and title "
"bar for each pad.  Window decorations are disabled by default.\n\n"
"If window decorations are enabled, you can choose what happens when you click on your <b>window "
"manager's close button</b>.  You can close and save all pads (quitting xpad), close and save "
"the one pad you clicked on, or delete the pad (no confirmation is offered).  By default, "
"only the one pad you clicked on is closed and saved.");

	gtk_label_set_markup (GTK_LABEL (label_misc_help), misctext);
	
	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);
	
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);
	
	gtk_widget_show_all (notebook);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page);
	gtk_widget_show_all (window);
	
	return window;
	
}

static void
open_help_callback (gpointer data)
{
	GtkNotebook *notebook = (GtkNotebook *) data;
	
	if (!pref_help_window)
	{
		pref_help_window = help_window_new (gtk_widget_get_toplevel (GTK_WIDGET (notebook)),
			gtk_notebook_get_current_page (notebook));
	}
}




static gboolean change_background_color (GtkWidget *colorsel, GtkWidget *checkbutton)
{
	pad_node *temp;
	
	current_settings.style.use_back = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)) ? 0 : 1;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.back);
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		GtkStyle *style;
		
		if (temp->locked) continue;
		
		gtk_widget_modify_base (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, current_settings.style.use_back ? 
			&current_settings.style.back : NULL);
		
		style = gtk_widget_get_style (GTK_WIDGET (get_text (temp->window)));
		gtk_widget_modify_bg (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, &style->base[GTK_STATE_NORMAL]);
	}
	
	gtk_widget_set_sensitive (colorsel, current_settings.style.use_back);
	
	return FALSE;
}

static gboolean change_text_color (GtkWidget *colorsel, GtkWidget *checkbutton)
{
	pad_node *temp;
	
	current_settings.style.use_text = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)) ? 0 : 1;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.text);
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (temp->locked) continue;
		
		gtk_widget_modify_text (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, current_settings.style.use_text ?
			&current_settings.style.text : NULL);
	}
		
	gtk_widget_set_sensitive (colorsel, current_settings.style.use_text);
	
	return FALSE;
}

static gboolean change_border_color (GtkWidget *colorsel, GtkWidget *window)
{
	pad_node *temp;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.border);
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (temp->locked) continue;
		
		gtk_widget_modify_bg (GTK_WIDGET (temp->eventbox_outer),
			GTK_STATE_NORMAL, &current_settings.style.border);
	}
	
	return FALSE;
}

static gboolean change_padding (GtkWidget *spinner, GtkWidget *window)
{
	pad_node *temp;
	
	current_settings.style.padding = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (temp->locked) continue;
		
		gtk_container_set_border_width (GTK_CONTAINER (get_text (temp->window)),
			current_settings.style.padding);
	}
	
	return FALSE;
}

static gboolean change_border_width (GtkWidget *spinner, GtkWidget *colorsel)
{
	pad_node *temp;
	
	current_settings.style.border_width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	
	gtk_widget_set_sensitive(colorsel, (current_settings.style.border_width != 0));
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (temp->locked) continue;
		
		gtk_container_set_border_width (GTK_CONTAINER (temp->eventbox),
			current_settings.style.border_width);
	}
	
	return FALSE;
}

static gboolean change_font (GtkWidget *fontsel, GtkWidget *checkbutton)
{
	pad_node *temp;
	PangoFontDescription *fontdesc;
	
	/* free current memory used by fontname */
	g_free (current_settings.style.fontname);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)))
	{
		current_settings.style.fontname = NULL;
		fontdesc = NULL;
	}
	else
	{
		current_settings.style.fontname = gtk_font_selection_get_font_name (GTK_FONT_SELECTION (fontsel));
		fontdesc = pango_font_description_from_string (current_settings.style.fontname);
	}
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (temp->locked) continue;
		
		gtk_widget_modify_font (GTK_WIDGET (get_text (temp->window)), fontdesc);
	}
	
	g_free (fontdesc);
	
	gtk_widget_set_sensitive (fontsel, current_settings.style.fontname ? TRUE : FALSE);
	
	return FALSE;
}

static gboolean change_font_3_args (GtkWidget *fontsel, gpointer middle, GtkWidget *checkbutton)
{
	return change_font (fontsel, checkbutton);
}

static gboolean change_decorations (GtkWidget *checkbutton, GtkWidget *frame)
{
	current_settings.decorations = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	pads_set_decorations (current_settings.decorations, gtk_widget_get_toplevel (frame));
	
	gtk_widget_set_sensitive (frame, current_settings.decorations);
	
	return FALSE;
}

static gboolean change_scrollbars (GtkWidget *checkbutton, GtkWidget *window)
{
	pad_node *temp;
	
	current_settings.scrollbar = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		pad_set_scrollbars (temp, current_settings.scrollbar);
	}
	
	return FALSE;
}

static gboolean change_confirm_destroy (GtkWidget *checkbutton, GtkWidget *window)
{
	current_settings.confirm_destroy = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	return FALSE;
}

static gboolean change_edit_lock (GtkWidget *checkbutton, GtkWidget *window)
{
	current_settings.edit_lock = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	pads_set_editable (!current_settings.edit_lock);
	
	return FALSE;
}

static gboolean change_wm_close (GtkWidget *radiobutton, gint num)
{
	current_settings.wm_close = num;
	
	return FALSE;
}


static void
change_toolbar (GtkToggleButton *togglebutton, gpointer user_data)
{
	pad_node *temp;
	
	current_settings.toolbar = gtk_toggle_button_get_active (togglebutton);
	
	g_slist_foreach ((GSList *) user_data, (GFunc) gtk_widget_set_sensitive, GINT_TO_POINTER (current_settings.toolbar));
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (current_settings.toolbar)
		{
			pad_add_toolbar (temp);
			
			if (!current_settings.auto_hide_toolbar)
				toolbar_show (temp);
		}
		else
			pad_remove_toolbar (temp);
	}
}

static void
change_auto_hide_toolbar (GtkToggleButton *togglebutton, gpointer user_data)
{
	pad_node *temp;
	
	current_settings.auto_hide_toolbar = gtk_toggle_button_get_active (togglebutton);
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		if (current_settings.auto_hide_toolbar)
		{
			toolbar_start_timeout (temp);	/* safe, since the cursor is unlikely to be on the pad? */
		}
		else
		{
			if (temp->toolbar->timeout)
				toolbar_end_timeout (temp);
			
			toolbar_show (temp);
		}
	}
}

void pref_close (void)
{
	if (pref_window)
	{
		gtk_widget_destroy (pref_window);
		pref_window = NULL;
		fio_save_default_settings ();
	}
}

static void
data_get (GtkWidget *widget, GdkDragContext *drag_context, 
	GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	gtk_selection_data_set (data, 
		gdk_atom_intern ("_XPAD_TOOLBAR_PREF", FALSE),
		8,
		(const guchar *) "",
		0);
}

static void
unused_data_receive (GtkWidget *widget, GdkDragContext *drag_context, gint x,
	gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	GtkWidget *source = gtk_drag_get_source_widget (drag_context);
	GtkWidget *parent = gtk_widget_get_parent (source);
	const toolbar_button *tb;
	pad_node *temp;
	
	gtk_widget_ref (source);
	gtk_container_remove (GTK_CONTAINER (parent), source);
	gtk_box_pack_start (GTK_BOX (user_data), source, FALSE, FALSE, 0);
	gtk_widget_unref (source);
	
	if (parent != GTK_WIDGET (user_data))
	{
		tb = (const toolbar_button *) g_object_get_data (G_OBJECT (source), "tb");
		
		current_settings.toolbar_buttons = 
			g_slist_remove (current_settings.toolbar_buttons,
			tb);
		
		for (temp = first_pad; temp; temp = temp->next)
			pad_toolbar_update (temp);
	}
}

static void
toolbar_data_receive (GtkWidget *widget, GdkDragContext *drag_context, gint x,
	gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	GtkWidget *source = gtk_drag_get_source_widget (drag_context);
	GtkWidget *parent = gtk_widget_get_parent (source);
	const toolbar_button *tb;
	pad_node *temp;
	
	gtk_widget_ref (source);
	gtk_container_remove (GTK_CONTAINER (parent), source);
	gtk_container_add (GTK_CONTAINER (user_data), source);
	gtk_widget_unref (source);
	
	tb = (const toolbar_button *) g_object_get_data (G_OBJECT (source), "tb");
	
	if (parent == GTK_WIDGET (user_data))
		current_settings.toolbar_buttons = 
			g_slist_remove (current_settings.toolbar_buttons,
			tb);
	
	current_settings.toolbar_buttons = 
		g_slist_append (current_settings.toolbar_buttons,
		(gpointer) tb);
	
	for (temp = first_pad; temp; temp = temp->next)
		pad_toolbar_update (temp);
}


/**
 * Note to the uncautious:  The following function is ugly as hell.  There are a lot of 
 * widgets define at the top that are not used until several pages down, there are random
 * statement blocks in the middle because it was easier to code it that way.  No consistent
 * naming scheme is used, and there is no order to most of the gtk function calls.  Proceed
 * at your own risk.  I am not responsible for any mental illness as a result.
 */
static GtkWidget *preferences_create (void)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *notebook = gtk_notebook_new ();
	GtkWidget *buttonbox = gtk_hbutton_box_new ();
	GtkWidget *button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GtkWidget *button_help = gtk_button_new_from_stock (GTK_STOCK_HELP);
	GtkWidget *label_back = gtk_label_new ("Background Color");
	GtkWidget *label_text = gtk_label_new ("Text Color");
	GtkWidget *label_border = gtk_label_new ("Border");
	GtkWidget *label_border_width = gtk_label_new ("Border width:");
	GtkWidget *label_border_width_unit = gtk_label_new ("pixels");
	GtkWidget *label_font = gtk_label_new ("Font Face");
	GtkWidget *label_misc = gtk_label_new ("Options");
	GtkWidget *label_toolbar = gtk_label_new ("Toolbar");
	GtkWidget *label_padding = gtk_label_new ("Padding:");
	GtkWidget *label_padding_unit = gtk_label_new ("pixels");
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 0);
	GtkWidget *color_back = gtk_color_selection_new ();
	GtkWidget *color_text = gtk_color_selection_new ();
	GtkWidget *color_border = gtk_color_selection_new ();
	GtkWidget *font_selection = gtk_font_selection_new ();
	GtkWidget *checkbutton_decorations = gtk_check_button_new_with_label ("Allow window manager decorations");
	GtkWidget *checkbutton_confirm_destroy = gtk_check_button_new_with_label ("Confirm pad deletion");
	GtkWidget *checkbutton_edit_lock = gtk_check_button_new_with_label ("Edit lock");
	GtkWidget *separator_border = gtk_hseparator_new ();
	GtkObject *adjust_padding;
	GtkWidget *spinner_padding;
	GtkObject *adjust_border_width;
	GtkWidget *spinner_border_width;
	GtkWidget *radio_close_all;
	GtkWidget *radio_close_this;
	GtkWidget *radio_delete_this;
	GtkWidget *vbox_wm_close;
	GtkWidget *frame_wm_close;
	GtkWidget *vbox_background = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_background = gtk_hbox_new (FALSE, 0);
	GtkWidget *vbox_text = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_text = gtk_hbox_new (FALSE, 0);
	GtkWidget *vbox_border = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_border = gtk_hbox_new (FALSE, 0);
	GtkWidget *vbox_font = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_font = gtk_hbox_new (FALSE, 0);
	GtkWidget *vbox_misc = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_misc = gtk_hbox_new (FALSE, 0);
	GtkWidget *hbox_padding = gtk_hbox_new (FALSE, 3);
	GtkWidget *hbox_border_width = gtk_hbox_new (FALSE, 3);
	GtkWidget *hbox_border_entries = gtk_hbox_new (FALSE, 3);
	
	GtkTooltips *tooltips_border = gtk_tooltips_new ();
	GtkTooltips *tooltips_options = gtk_tooltips_new ();
	
	gtk_container_set_border_width (GTK_CONTAINER (vbox_background), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_background), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_text), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_text), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_border), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_border), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_font), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_font), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_misc), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_misc), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_padding), 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_border_width), 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_border_entries), 0);
	
	gtk_window_set_title (GTK_WINDOW(window), "xpad Preferences");
	gtk_container_add (GTK_CONTAINER(window), vbox_global);

	/* buttonbox setup */
	g_signal_connect_swapped (GTK_OBJECT (button_close), "clicked", 
		G_CALLBACK (gtk_widget_destroy), (gpointer) window);
	g_signal_connect_swapped (GTK_OBJECT (button_help), "clicked", 
		G_CALLBACK (open_help_callback), notebook);
	g_signal_connect (GTK_OBJECT (window), "destroy", 
		G_CALLBACK (pref_close), NULL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(buttonbox), 0);
	gtk_box_pack_start_defaults (GTK_BOX(buttonbox), button_help);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_close);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (buttonbox), button_help, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 6);
	
	/* vbox_global setup */
	gtk_box_pack_start (GTK_BOX(vbox_global), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox_global), buttonbox, TRUE, TRUE, 0);

	/* text setup */
	{
		GtkWidget *separator = gtk_hseparator_new ();
		GtkWidget *checkbutton_use = gtk_check_button_new_with_label ("Use system text color");
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_text, label_text);
		gtk_box_pack_start (GTK_BOX (hbox_text), vbox_text, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (vbox_text), checkbutton_use, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_use), !current_settings.style.use_text);
		g_signal_connect_swapped (GTK_OBJECT (checkbutton_use), "toggled", G_CALLBACK (change_text_color), (gpointer) color_text);
		
		gtk_box_pack_start (GTK_BOX (vbox_text), separator, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_text), color_text, FALSE, FALSE, 9);
		gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_text), &current_settings.style.text);
		gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_text), FALSE);
		gtk_widget_set_sensitive (color_text, current_settings.style.use_text);
		g_signal_connect (GTK_OBJECT (color_text), "color-changed", G_CALLBACK (change_text_color), (gpointer) checkbutton_use);
		
	}
	
	/* background setup */
	{
		GtkWidget *separator = gtk_hseparator_new ();
		GtkWidget *checkbutton_use = gtk_check_button_new_with_label ("Use system background color");
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_background, label_back);
		gtk_box_pack_start (GTK_BOX (hbox_background), vbox_background, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (vbox_background), checkbutton_use, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_use), !current_settings.style.use_back);
		g_signal_connect_swapped (GTK_OBJECT (checkbutton_use), "toggled", G_CALLBACK (change_background_color), (gpointer) color_back);
		
		gtk_box_pack_start (GTK_BOX (vbox_background), separator, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_background), color_back, FALSE, FALSE, 9);
		gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_back), &current_settings.style.back);
		gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_back), FALSE);
		gtk_widget_set_sensitive (color_back, current_settings.style.use_back);
		g_signal_connect (GTK_OBJECT (color_back), "color-changed", G_CALLBACK (change_background_color), (gpointer) checkbutton_use);
	}
	
	/* border setup */
	{
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_border, label_border);
		gtk_box_pack_start (GTK_BOX (hbox_border), vbox_border, FALSE, FALSE, 0);
		
		adjust_padding = gtk_adjustment_new (current_settings.style.padding, 0.0, 100.0, 1.0, 5.0, 5.0);
		spinner_padding = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_padding), 1.0, 0);
		gtk_misc_set_alignment (GTK_MISC (label_padding), 0, 0.5);
		gtk_entry_set_width_chars (GTK_ENTRY (spinner_padding), 3);
		
		gtk_box_pack_start (GTK_BOX (hbox_padding), label_padding, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_padding), spinner_padding, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_padding), label_padding_unit, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (spinner_padding), "value-changed", G_CALLBACK (change_padding), (gpointer) color_border);
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_border), spinner_padding, 
	"The amount of space you want between the border and text.",
	"Choose the number of pixels around the text region.  This space is colored "
	"the same and surrounds it on all sides.");
		
		adjust_border_width = gtk_adjustment_new (current_settings.style.border_width, 0.0, 100.0, 1.0, 5.0, 5.0);
		spinner_border_width = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_border_width), 1.0, 0);
		gtk_misc_set_alignment (GTK_MISC (label_border_width), 0, 0.5);
		gtk_entry_set_width_chars (GTK_ENTRY (spinner_border_width), 3);
		
		gtk_box_pack_start (GTK_BOX (hbox_border_width), label_border_width, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_border_width), spinner_border_width, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_border_width), label_border_width_unit, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (spinner_border_width), "value-changed", G_CALLBACK (change_border_width), (gpointer) color_border);
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_border), spinner_border_width, 
	"The amount of space you want surrounding the pad.", 
	"Choose the number of pixels around the pad.  This space is colored "
	"independently and surrounds it on all sides.");
		
		gtk_box_pack_start_defaults (GTK_BOX (hbox_border_entries), hbox_border_width);
		gtk_box_pack_start_defaults (GTK_BOX (hbox_border_entries), hbox_padding);
		gtk_box_pack_start (GTK_BOX (vbox_border), hbox_border_entries, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_border), separator_border, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_border), color_border, FALSE, FALSE, 9);
		
		gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_border), &current_settings.style.border);
		gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_border), FALSE);
		g_signal_connect (GTK_OBJECT (color_border), "color-changed", G_CALLBACK (change_border_color), (gpointer) window);
		
		if (current_settings.style.border_width == 0)
			gtk_widget_set_sensitive (color_border, FALSE);
		
	}

	/* font setup */
	{
		GtkWidget *separator = gtk_hseparator_new ();
		GtkWidget *checkbutton_use = gtk_check_button_new_with_label ("Use system font face");
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_font, label_font);
		gtk_box_pack_start (GTK_BOX (hbox_font), vbox_font, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (vbox_font), checkbutton_use, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_use), current_settings.style.fontname ? 0 : 1);
		g_signal_connect_swapped (GTK_OBJECT (checkbutton_use), "toggled", G_CALLBACK (change_font), (gpointer) font_selection);
		
		gtk_box_pack_start (GTK_BOX (vbox_font), separator, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_font), font_selection, FALSE, FALSE, 9);
		gtk_widget_set_sensitive (font_selection, current_settings.style.fontname ? 1 : 0);
		
		if (current_settings.style.fontname)
			gtk_font_selection_set_font_name (GTK_FONT_SELECTION (font_selection), 
				current_settings.style.fontname);
		
		/* this is a bit hacky, but there is no font-changed signal! */
		g_signal_connect (GTK_OBJECT (font_selection), "button-release-event", G_CALLBACK (change_font_3_args), (gpointer) checkbutton_use);
		/* key release event does not seem to be sent when I think it should */
		gtk_widget_add_events(font_selection, GDK_KEY_RELEASE_MASK);
		g_signal_connect (GTK_OBJECT (font_selection), "key-release-event", G_CALLBACK (change_font_3_args), (gpointer) checkbutton_use);
	}
	
	/* toolbar  setup */
	{
		GtkWidget *vbox_toolbar = gtk_vbox_new (FALSE, 3);
		GtkWidget *hbox_toolbar = gtk_hbox_new (FALSE, 3);
		GtkWidget *frame = gtk_frame_new (NULL);
		xpad_toolbar *xt = toolbar_new ();
		gint i;
		GList *inxt, *tmp;
		GtkWidget *buttonbox = gtk_hbox_new (FALSE, 3);
		GtkTooltips *tt = gtk_tooltips_new ();
		GtkTargetEntry entry;
		GtkWidget *label_buttons_desc = gtk_label_new ("Click and drag a button to move it.");
		GtkWidget *unused_frame = gtk_frame_new (NULL);
		GtkWidget *toolbar_frame = gtk_frame_new (NULL);
		GtkWidget *vbox_frame = gtk_vbox_new (FALSE, 0);
		GtkWidget *vbox_unused_frame = gtk_vbox_new (FALSE, 0);
		GtkWidget *vbox_toolbar_frame = gtk_vbox_new (FALSE, 0);
		GtkWidget *toolbar_on = gtk_check_button_new_with_label ("Enable toolbar");
		GtkWidget *toolbar_auto_hide = gtk_check_button_new_with_label ("Auto-hide toolbar");
		GtkWidget *align = gtk_alignment_new (0, 0, 0, 0);
		GtkWidget *hbox_buttons = gtk_hbox_new (FALSE, 0);
		GtkWidget *label_indent = gtk_label_new ("    ");
		
		{
			GtkWidget *label_frame = gtk_label_new (NULL);
			GtkWidget *label_unused = gtk_label_new (NULL);
			GtkWidget *label_used = gtk_label_new (NULL);
			
			gtk_label_set_markup (GTK_LABEL (label_frame), "<b>Toolbar Buttons:</b>");
			gtk_label_set_markup (GTK_LABEL (label_used), "<b>Used Buttons:</b>");
			gtk_label_set_markup (GTK_LABEL (label_unused), "<b>Unused Buttons:</b>");
			gtk_frame_set_label_widget (GTK_FRAME (frame), label_frame);
			gtk_frame_set_label_widget (GTK_FRAME (toolbar_frame), label_used);
			gtk_frame_set_label_widget (GTK_FRAME (unused_frame), label_unused);
			gtk_frame_set_shadow_type (GTK_FRAME (unused_frame), GTK_SHADOW_NONE);
			gtk_frame_set_shadow_type (GTK_FRAME (toolbar_frame), GTK_SHADOW_NONE);
			gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
		}
		
		entry.target = "_XPAD_TOOLBAR_PREF";
		entry.flags = GTK_TARGET_SAME_APP;
		entry.info = 2;
		
		gtk_container_set_border_width (GTK_CONTAINER (vbox_toolbar), 6);
		gtk_container_set_border_width (GTK_CONTAINER (hbox_toolbar), 6);
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_toolbar, label_toolbar);
		gtk_box_pack_start (GTK_BOX (hbox_toolbar), vbox_toolbar, TRUE, TRUE, 0);
		
		toolbar_widgets = g_slist_append (NULL, toolbar_auto_hide);
		toolbar_widgets = g_slist_append (toolbar_widgets, frame);
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), toolbar_on, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_on), current_settings.toolbar);
		g_signal_connect (toolbar_on, "toggled", G_CALLBACK (change_toolbar), toolbar_widgets);
		
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), toolbar_auto_hide, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_auto_hide), current_settings.auto_hide_toolbar);
		g_signal_connect (toolbar_auto_hide, "toggled", G_CALLBACK (change_auto_hide_toolbar), NULL);
		
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), frame, FALSE, FALSE, 9);
		gtk_widget_set_sensitive (frame, current_settings.toolbar);
		
		gtk_box_pack_start (GTK_BOX (hbox_buttons), label_indent, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_buttons), vbox_frame, TRUE, TRUE, 0);
		gtk_container_add (GTK_CONTAINER (frame), hbox_buttons);
		
		gtk_box_pack_start (GTK_BOX (vbox_frame), align, FALSE, FALSE, 6);
		gtk_container_add (GTK_CONTAINER (align), label_buttons_desc);
		
		gtk_box_pack_start (GTK_BOX (vbox_frame), toolbar_frame, FALSE, FALSE, 6);
		gtk_container_add (GTK_CONTAINER (toolbar_frame), vbox_toolbar_frame);
		gtk_container_add (GTK_CONTAINER (vbox_toolbar_frame), xt->bar);
		toolbar_update (xt);
		
		gtk_box_pack_start (GTK_BOX (vbox_frame), unused_frame, FALSE, FALSE, 6);
		gtk_container_add (GTK_CONTAINER (unused_frame), vbox_unused_frame);
		gtk_container_add (GTK_CONTAINER (vbox_unused_frame), buttonbox);
		
		inxt = toolbar_get_children (xt);
		
		for (tmp = inxt; tmp; tmp = tmp->next)
		{
			GtkWidget *widget = GTK_WIDGET (tmp->data);
			
			gtk_drag_source_set (widget,
				GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_MOVE);
			
			g_signal_connect (widget, "drag-data-get", 
				G_CALLBACK (data_get), NULL);
		}
		
		g_list_free (inxt);
		
		/* build list of all toolbar buttons not in xt */
		for (i = 0; i < num_buttons; i++)
		{
			GSList *tmp;
			const toolbar_button *tb;
			GtkWidget *b;
			
			for (tmp = current_settings.toolbar_buttons; tmp; tmp = tmp->next)
			{
				tb = (const toolbar_button *) tmp->data;
				
				if (!g_ascii_strcasecmp (tb->name, buttons[i].name))
					break;
			}
			
			if (tmp)	/* we found it, so we don't add it to our list of unused buttons */
				continue;
			
			tb = &buttons[i];
			
			b = toolbar_button_new (tb);
			
			gtk_box_pack_start (GTK_BOX (buttonbox), b, FALSE,
				FALSE, 0);
			
			gtk_drag_source_set (b,
				GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_MOVE);
			
			g_signal_connect (b, "drag-data-get", 
				G_CALLBACK (data_get), NULL);
			
			g_object_set_data (G_OBJECT (b), "tb", (void *) tb);
			
			gtk_tooltips_set_tip (tt, b, buttons[i].desc, buttons[i].desc);
		}
		
		{
			GtkWidget *cont = toolbar_get_container (xt);
			
			gtk_drag_dest_set (unused_frame, GTK_DEST_DEFAULT_ALL,
				&entry, 1, GDK_ACTION_MOVE);
			gtk_drag_dest_set (toolbar_frame, GTK_DEST_DEFAULT_ALL,
				&entry, 1, GDK_ACTION_MOVE);
			
			g_signal_connect (unused_frame, "drag-data-received",
				G_CALLBACK (unused_data_receive), buttonbox);
			g_signal_connect (toolbar_frame, "drag-data-received",
				G_CALLBACK (toolbar_data_receive), cont);
		}
		
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tt), toolbar_on, 
"If on, a toolbar will appear below each pad when the mouse hovers over it.",
"If on, a toolbar will appear below each pad when the mouse hovers over it.");
		
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tt), toolbar_auto_hide, 
"If on, the toolbar will disappear when you are not using the pad.",
"If on, the toolbar will disappear when you are not using the pad.");
		
		gtk_container_set_border_width (GTK_CONTAINER (vbox_frame), 6);
		gtk_container_set_border_width (GTK_CONTAINER (vbox_unused_frame), 6);
		gtk_container_set_border_width (GTK_CONTAINER (vbox_toolbar_frame), 6);
	}
	
	/* misc. setup */
	{
		GtkWidget *label_frame_wm = gtk_label_new (NULL);
		GtkWidget *label_frame_wm_indent = gtk_label_new ("    ");
		GtkWidget *label_frame_wm_hbox = gtk_hbox_new (FALSE, 0);
		GtkWidget *checkbutton_scrollbars = gtk_check_button_new_with_label ("Allow Scrollbars");
		
		radio_close_all = gtk_radio_button_new_with_label (NULL,
			"Close and save all pads");
		radio_close_this = gtk_radio_button_new_with_label_from_widget (
			GTK_RADIO_BUTTON (radio_close_all), "Close and save pad");
		radio_delete_this = gtk_radio_button_new_with_label_from_widget (
			GTK_RADIO_BUTTON (radio_close_all), "Delete pad (no confirmation)");
		frame_wm_close = gtk_frame_new (NULL);
		
		vbox_wm_close = gtk_vbox_new (FALSE, 3);
		
		gtk_label_set_markup (GTK_LABEL (label_frame_wm), "<b>Window Manager Close Action:</b>");
		gtk_frame_set_shadow_type (GTK_FRAME (frame_wm_close), GTK_SHADOW_NONE);
		gtk_frame_set_label_widget (GTK_FRAME (frame_wm_close), label_frame_wm);
		
		switch (current_settings.wm_close)
		{
		case 0: /* close all */
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_close_all), TRUE);
			break;
		default:
		case 1: /* close pad */
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_close_this), TRUE);
			break;
		case 2: /* delete pad */
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_delete_this), TRUE);
			break;
		}
		
		gtk_widget_set_sensitive (frame_wm_close, current_settings.decorations);
		
		gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_close_all, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_close_this, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_delete_this, FALSE, FALSE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (vbox_wm_close), 6);
		gtk_box_pack_start (GTK_BOX (label_frame_wm_hbox), label_frame_wm_indent,
			FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (label_frame_wm_hbox), vbox_wm_close,
			FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (frame_wm_close), label_frame_wm_hbox);
	
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_decorations), current_settings.decorations);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_confirm_destroy), current_settings.confirm_destroy);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_edit_lock), current_settings.edit_lock);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_scrollbars), current_settings.scrollbar);
		gtk_box_pack_start (GTK_BOX (hbox_misc), vbox_misc, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_edit_lock, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_confirm_destroy, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_scrollbars, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_decorations, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), frame_wm_close, FALSE, FALSE, 3);
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_misc, label_misc);
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_decorations, 
	"If on, your window manager will add its own decorations to each pad.  For example, "
	"a titlebar and close button.",
	"If on, your window manager will add its own decorations to each pad.  For example, "
	"a titlebar and close button.");
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_confirm_destroy, 
	"If on, choosing to delete a pad will prompt for conformation.",
	"If on, choosing to delete a pad will prompt for conformation.");
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_scrollbars, 
	"If on, scrollbars appear when text is larger than the pad.  If off, the pad resizes "
	"to fit the text.",
	"If on, scrollbars appear when text is larger than the pad.  If off, the pad resizes "
	"to fit the text.");
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_edit_lock, 
	"If on, when a pad loses focus, it will become uneditable.  "
	"This allows you to move it by left dragging.  To make it editable again, "
	"double-click the pad.  If off, pads are always editable.",
	"If on, when a pad loses focus, it will become uneditable.  "
	"This allows you to move it by left dragging.  To make it editable again, "
	"double-click the pad.  If off, pads are always editable.");
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), radio_close_all, 
	"All pads will close, saving contents first.",
	"All pads will close, saving contents first.");
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), radio_close_this, 
	"The pad you clicked the close button on will close, saving contents.",
	"The pad you clicked the close button on will close, saving contents.");
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), radio_delete_this, 
	"The pad you clicked the close button on will be deleted, losing contents.  There "
	"will be no confirmation.",
	"The pad you clicked the close button on will be deleted, losing contents.  There "
	"will be no confirmation.");
	
		g_signal_connect (GTK_OBJECT (checkbutton_confirm_destroy), "toggled", G_CALLBACK (change_confirm_destroy), (gpointer) window);
		g_signal_connect (GTK_OBJECT (checkbutton_decorations), "toggled", G_CALLBACK (change_decorations), (gpointer) frame_wm_close);
		g_signal_connect (GTK_OBJECT (checkbutton_edit_lock), "toggled", G_CALLBACK (change_edit_lock), (gpointer) window);
		g_signal_connect (GTK_OBJECT (checkbutton_scrollbars), "toggled", G_CALLBACK (change_scrollbars), (gpointer) window);
		g_signal_connect (GTK_OBJECT (radio_close_all), "toggled", G_CALLBACK (change_wm_close), (gpointer) 0);
		g_signal_connect (GTK_OBJECT (radio_close_this), "toggled", G_CALLBACK (change_wm_close), (gpointer) 1);
		g_signal_connect (GTK_OBJECT (radio_delete_this), "toggled", G_CALLBACK (change_wm_close), (gpointer) 2);
	}


	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_widget_show_all (window);
	
	return window;
}

void preferences_open (void)
{
	if (pref_window == NULL)
		pref_window = preferences_create ();
	else
		gtk_window_present (GTK_WINDOW (pref_window));
}
