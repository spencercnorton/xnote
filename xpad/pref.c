/*

Copyright (c) 2001-2002 Michael Terry

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
#include <string.h>


gboolean change_background_color (GtkWidget *colorsel, GtkWidget *window)
{
	pad_node *temp = first_pad;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.back);
	
	while (temp)
	{
		gtk_widget_modify_base (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, &current_settings.style.back);
		gtk_widget_modify_bg (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, &current_settings.style.back);

		temp = temp->next;
	}
	
	return FALSE;
}

gboolean change_text_color (GtkWidget *colorsel, GtkWidget *window)
{
	pad_node *temp = first_pad;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.text);
	
	while (temp)
	{
		gtk_widget_modify_text (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, &current_settings.style.text);
		
		temp = temp->next;
	}
	
	return FALSE;
}

gboolean change_border_color (GtkWidget *colorsel, GtkWidget *window)
{
	pad_node *temp = first_pad;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.border);
	
	while (temp)
	{
		gtk_widget_modify_bg (GTK_WIDGET (temp->eventbox_outer),
			GTK_STATE_NORMAL, &current_settings.style.border);
		
		temp = temp->next;
	}
	
	return FALSE;
}

gboolean change_padding (GtkWidget *spinner, GtkWidget *window)
{
	pad_node *temp = first_pad;
	
	current_settings.style.padding = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	
	while (temp)
	{
		gtk_container_set_border_width (GTK_CONTAINER (get_text (temp->window)),
			current_settings.style.padding);
		
		temp = temp->next;
	}
	
	return FALSE;
}

gboolean change_border_width (GtkWidget *spinner, GtkWidget *window)
{
	pad_node *temp = first_pad;
	
	current_settings.style.border_width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	
	while (temp)
	{
		gtk_container_set_border_width (GTK_CONTAINER (temp->eventbox),
			current_settings.style.border_width);
		
		temp = temp->next;
	}
	
	return FALSE;
}

gboolean change_font (GtkWidget *fontsel, GtkWidget *window)
{
	pad_node *temp = first_pad;
	
	strncpy (current_settings.style.fontname, 
		gtk_font_selection_get_font_name (GTK_FONT_SELECTION (fontsel)),
		MAX_FILENAME_SIZE);
	current_settings.style.filename[MAX_FILENAME_SIZE] = '\0';
	
	while (temp)
	{
		gtk_widget_modify_font (GTK_WIDGET (get_text (temp->window)),
			pango_font_description_from_string (current_settings.style.fontname));
		
		temp = temp->next;
	}
	
	return FALSE;
}

gboolean change_decorations (GtkWidget *checkbutton, GtkWidget *window)
{
	current_settings.decorations = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	pads_set_decorations (current_settings.decorations, window);
	
	return FALSE;
}

gboolean change_confirm_destroy (GtkWidget *checkbutton, GtkWidget *window)
{
	current_settings.confirm_destroy = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	return FALSE;
}

gboolean change_edit_lock (GtkWidget *checkbutton, GtkWidget *window)
{
	current_settings.edit_lock = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	pads_set_editable (!current_settings.edit_lock);
	
	return FALSE;
}

void preferences_open (pad_node *pad)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *notebook = gtk_notebook_new ();
	GtkWidget *buttonbox = gtk_hbutton_box_new ();
	GtkWidget *button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
//	GtkWidget *button_help = gtk_button_new_from_stock (GTK_STOCK_HELP);
	GtkWidget *label_back = gtk_label_new ("Background Color");
	GtkWidget *label_text = gtk_label_new ("Text Color");
	GtkWidget *label_border = gtk_label_new ("Border");
	GtkWidget *label_border_width = gtk_label_new ("Border width:");
	GtkWidget *label_border_width_unit = gtk_label_new ("pixels");
	GtkWidget *label_font = gtk_label_new ("Font Face");
	GtkWidget *label_misc = gtk_label_new ("Options");
	GtkWidget *label_padding = gtk_label_new ("Padding:");
	GtkWidget *label_padding_unit = gtk_label_new ("pixels");
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 0);
	GtkWidget *color_back = gtk_color_selection_new ();
	GtkWidget *color_text = gtk_color_selection_new ();
	GtkWidget *color_border = gtk_color_selection_new ();
	GtkWidget *font_selection = gtk_font_selection_new ();
	GtkWidget *checkbutton_decorations = gtk_check_button_new_with_label ("Allow window manager decorations");
	GtkWidget *checkbutton_confirm_destroy = gtk_check_button_new_with_label ("Confirm pad destructions");
	GtkWidget *checkbutton_edit_lock = gtk_check_button_new_with_label ("Edit lock");
	GtkWidget *hbox_padding = gtk_hbox_new (FALSE, 0);
	GtkWidget *hbox_border_width = gtk_hbox_new (FALSE, 0);
	GtkWidget *hbox_border_entries = gtk_hbox_new (FALSE, 0);
	GtkWidget *separator_border = gtk_hseparator_new ();
	GtkObject *adjust_padding;
	GtkWidget *spinner_padding;
	GtkObject *adjust_border_width;
	GtkWidget *spinner_border_width;
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
	GtkTooltips *tooltips_border = gtk_tooltips_new ();
	GtkTooltips *tooltips_options = gtk_tooltips_new ();
		
	gtk_window_set_title (GTK_WINDOW(window), "xpad Preferences");
	gtk_container_add (GTK_CONTAINER(window), vbox_global);

	// buttonbox setup
	g_signal_connect_swapped (GTK_OBJECT (button_close), "clicked", 
		G_CALLBACK (gtk_widget_destroy), (gpointer) window);
	g_signal_connect_swapped (GTK_OBJECT (window), "destroy", 
		G_CALLBACK (fio_save_as_defaults), (gpointer) &current_settings);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(buttonbox), 10);
//	gtk_box_pack_start_defaults (GTK_BOX(buttonbox), button_help);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_close);
//	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (buttonbox), button_help, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 10);

	// vbox_global setup
	gtk_box_pack_start (GTK_BOX(vbox_global), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox_global), buttonbox, TRUE, TRUE, 0);

	// text setup
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_text, label_text);
	gtk_box_pack_start (GTK_BOX (hbox_text), vbox_text, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (vbox_text), color_text, FALSE, FALSE, 10);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_text), &current_settings.style.text);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_text), FALSE);
	g_signal_connect (GTK_OBJECT (color_text), "color-changed", G_CALLBACK (change_text_color), (gpointer) window);

	// background setup
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_background, label_back);
	gtk_box_pack_start (GTK_BOX (hbox_background), vbox_background, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (vbox_background), color_back, FALSE, FALSE, 10);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_back), &current_settings.style.back);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_back), FALSE);
	g_signal_connect (GTK_OBJECT (color_back), "color-changed", G_CALLBACK (change_background_color), (gpointer) window);

	// border setup
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_border, label_border);
	gtk_box_pack_start (GTK_BOX (hbox_border), vbox_border, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (vbox_border), color_border, FALSE, FALSE, 10);
	
	gtk_misc_set_alignment (GTK_MISC (label_border_width), 0, 1);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_border), &current_settings.style.border);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_border), FALSE);
	g_signal_connect (GTK_OBJECT (color_border), "color-changed", G_CALLBACK (change_border_color), (gpointer) window);

	gtk_box_pack_start (GTK_BOX (vbox_border), separator_border, FALSE, FALSE, 5);
	
	adjust_padding = gtk_adjustment_new (current_settings.style.padding, 0.0, 100.0, 1.0, 5.0, 5.0);
	spinner_padding = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_padding), 1.0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_padding), 0, 0.5);
	gtk_entry_set_width_chars (GTK_ENTRY (spinner_padding), 3);

	gtk_box_pack_start (GTK_BOX (hbox_padding), label_padding, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox_padding), spinner_padding, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox_padding), label_padding_unit, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (spinner_padding), "value-changed", G_CALLBACK (change_padding), (gpointer) window);
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_border), spinner_padding, 
"The amount of space you want between the border and text.",
"Choose the number of pixels around the text region.  This space is colored "
"the same and surrounds it on all sides.");

	adjust_border_width = gtk_adjustment_new (current_settings.style.border_width, 0.0, 100.0, 1.0, 5.0, 5.0);
	spinner_border_width = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_border_width), 1.0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_border_width), 0, 0.5);
	gtk_entry_set_width_chars (GTK_ENTRY (spinner_border_width), 3);

	gtk_box_pack_start (GTK_BOX (hbox_border_width), label_border_width, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox_border_width), spinner_border_width, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox_border_width), label_border_width_unit, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (spinner_border_width), "value-changed", G_CALLBACK (change_border_width), (gpointer) window);
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_border), spinner_border_width, 
"The amount of space you want surrounding the pad.", 
"Choose the number of pixels around the pad.  This space is colored"
"independently and surrounds it on all sides.");

	gtk_box_pack_start_defaults (GTK_BOX (hbox_border_entries), hbox_border_width);
	gtk_box_pack_start_defaults (GTK_BOX (hbox_border_entries), hbox_padding);
	gtk_box_pack_start (GTK_BOX (vbox_border), hbox_border_entries, FALSE, FALSE, 10);

	// font setup
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_font, label_font);
	gtk_box_pack_start (GTK_BOX (hbox_font), vbox_font, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (vbox_font), font_selection, FALSE, FALSE, 10);
	gtk_font_selection_set_font_name (GTK_FONT_SELECTION (font_selection), current_settings.style.fontname);
	
	// this is a bit hacky, but there is no font-changed signal!
	g_signal_connect (GTK_OBJECT (font_selection), "button-release-event", G_CALLBACK (change_font), (gpointer) window);
	// key release event does not seem to be sent when I think it should
	g_signal_connect (GTK_OBJECT (font_selection), "key-release-event", G_CALLBACK (change_font), (gpointer) window);

	// misc. setup
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_decorations), current_settings.decorations);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_confirm_destroy), current_settings.confirm_destroy);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_edit_lock), current_settings.edit_lock);
	gtk_box_pack_start (GTK_BOX (hbox_misc), vbox_misc, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_confirm_destroy, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_decorations, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_edit_lock, FALSE, FALSE, 10);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_misc, label_misc);

	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_decorations, 
"If on, your window manager will add its own decorations to each pad.  For example, "
"a titlebar and close button.",
"If on, your window manager will add its own decorations to each pad.  For example, "
"a titlebar and close button.");

	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_confirm_destroy, 
"If on, choosing to destroy a pad will prompt for conformation.",
"If on, choosing to destroy a pad will prompt for conformation.");

	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_edit_lock, 
"If on, when a pad loses focus, it will become uneditable.  "
"This allows you to move it by left dragging.  To make it editable again, "
"double-click the pad.  If off, pads are always editable.",
"If on, when a pad loses focus, it will become uneditable.  "
"This allows you to move it by left dragging.  To make it editable again, "
"double-click the pad.  If off, pads are always editable.");

	g_signal_connect (GTK_OBJECT (checkbutton_confirm_destroy), "toggled", G_CALLBACK (change_confirm_destroy), (gpointer) window);
	g_signal_connect (GTK_OBJECT (checkbutton_decorations), "toggled", G_CALLBACK (change_decorations), (gpointer) window);
	g_signal_connect (GTK_OBJECT (checkbutton_edit_lock), "toggled", G_CALLBACK (change_edit_lock), (gpointer) window);

	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_widget_show_all (window);
}
