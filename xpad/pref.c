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
#include "help.h"
#include <string.h>

// we keep a pointer around so that only one window will be open at a time
GtkWidget *pref_window = NULL;

static gboolean change_background_color (GtkWidget *colorsel, GtkWidget *window)
{
	pad_node *temp;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.back);
	
	for (temp = first_pad; temp; temp = temp->next)
	{
		gtk_widget_modify_base (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, &current_settings.style.back);
		gtk_widget_modify_bg (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, &current_settings.style.back);
	}
	
	return FALSE;
}

static gboolean change_text_color (GtkWidget *colorsel, GtkWidget *window)
{
	pad_node *temp;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.text);
	
	for (temp = first_pad; temp; temp = temp->next)
		gtk_widget_modify_text (GTK_WIDGET (get_text (temp->window)),
			GTK_STATE_NORMAL, &current_settings.style.text);
	
	return FALSE;
}

static gboolean change_border_color (GtkWidget *colorsel, GtkWidget *window)
{
	pad_node *temp;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&current_settings.style.border);
	
	for (temp = first_pad; temp; temp = temp->next)
		gtk_widget_modify_bg (GTK_WIDGET (temp->eventbox_outer),
			GTK_STATE_NORMAL, &current_settings.style.border);
	
	return FALSE;
}

static gboolean change_padding (GtkWidget *spinner, GtkWidget *window)
{
	pad_node *temp;
	
	current_settings.style.padding = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	
	for (temp = first_pad; temp; temp = temp->next)
		gtk_container_set_border_width (GTK_CONTAINER (get_text (temp->window)),
			current_settings.style.padding);
	
	return FALSE;
}

static gboolean change_border_width (GtkWidget *spinner, GtkWidget *colorsel)
{
	pad_node *temp;
	
	current_settings.style.border_width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	
	gtk_widget_set_sensitive(colorsel, (current_settings.style.border_width != 0));
	
	for (temp = first_pad; temp; temp = temp->next)
		gtk_container_set_border_width (GTK_CONTAINER (temp->eventbox),
			current_settings.style.border_width);
	
	return FALSE;
}

static gboolean change_font (GtkWidget *fontsel, GtkWidget *window)
{
	pad_node *temp;
	
	strncpy (current_settings.style.fontname, 
		gtk_font_selection_get_font_name (GTK_FONT_SELECTION (fontsel)),
		MAX_FILENAME_SIZE);
	current_settings.style.fontname[MAX_FILENAME_SIZE] = '\0';
	
	for (temp = first_pad; temp; temp = temp->next)
		gtk_widget_modify_font (GTK_WIDGET (get_text (temp->window)),
			pango_font_description_from_string (current_settings.style.fontname));
	
	return FALSE;
}

static gboolean change_decorations (GtkWidget *checkbutton, GtkWidget *frame)
{
	current_settings.decorations = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	pads_set_decorations (current_settings.decorations, gtk_widget_get_toplevel (frame));
	
	gtk_widget_set_sensitive (frame, current_settings.decorations);
	
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

static void pref_close (void)
{
	pref_window = NULL;
	fio_save_as_defaults (&current_settings);
}

static GtkWidget *preferences_create (pad_node *pad)
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

	// buttonbox setup
	g_signal_connect_swapped (GTK_OBJECT (button_close), "clicked", 
		G_CALLBACK (gtk_widget_destroy), (gpointer) window);
	g_signal_connect_swapped (GTK_OBJECT (button_help), "clicked", 
		G_CALLBACK (show_help), NULL);
	g_signal_connect (GTK_OBJECT (window), "destroy", 
		G_CALLBACK (pref_close), NULL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(buttonbox), 0);
	gtk_box_pack_start_defaults (GTK_BOX(buttonbox), button_help);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_close);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (buttonbox), button_help, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 6);

	// vbox_global setup
	gtk_box_pack_start (GTK_BOX(vbox_global), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox_global), buttonbox, TRUE, TRUE, 0);

	// text setup
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_text, label_text);
	gtk_box_pack_start (GTK_BOX (hbox_text), vbox_text, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_text), color_text, FALSE, FALSE, 0);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_text), &current_settings.style.text);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_text), FALSE);
	g_signal_connect (GTK_OBJECT (color_text), "color-changed", G_CALLBACK (change_text_color), (gpointer) window);

	// background setup
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_background, label_back);
	gtk_box_pack_start (GTK_BOX (hbox_background), vbox_background, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_background), color_back, FALSE, FALSE, 0);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_back), &current_settings.style.back);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_back), FALSE);
	g_signal_connect (GTK_OBJECT (color_back), "color-changed", G_CALLBACK (change_background_color), (gpointer) window);

	// border setup
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_border, label_border);
	gtk_box_pack_start (GTK_BOX (hbox_border), vbox_border, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_border), color_border, FALSE, FALSE, 0);
	
	gtk_misc_set_alignment (GTK_MISC (label_border_width), 0, 1);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_border), &current_settings.style.border);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_border), FALSE);
	g_signal_connect (GTK_OBJECT (color_border), "color-changed", G_CALLBACK (change_border_color), (gpointer) window);

	if (current_settings.style.border_width == 0)
		gtk_widget_set_sensitive (color_border, FALSE);

	gtk_box_pack_start (GTK_BOX (vbox_border), separator_border, FALSE, FALSE, 9);
	
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
"Choose the number of pixels around the pad.  This space is colored"
"independently and surrounds it on all sides.");

	gtk_box_pack_start_defaults (GTK_BOX (hbox_border_entries), hbox_border_width);
	gtk_box_pack_start_defaults (GTK_BOX (hbox_border_entries), hbox_padding);
	gtk_box_pack_start (GTK_BOX (vbox_border), hbox_border_entries, FALSE, FALSE, 0);

	// font setup
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_font, label_font);
	gtk_box_pack_start (GTK_BOX (hbox_font), vbox_font, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_font), font_selection, FALSE, FALSE, 0);
	gtk_font_selection_set_font_name (GTK_FONT_SELECTION (font_selection), current_settings.style.fontname);
	
	// this is a bit hacky, but there is no font-changed signal!
	g_signal_connect (GTK_OBJECT (font_selection), "button-release-event", G_CALLBACK (change_font), (gpointer) window);
	// key release event does not seem to be sent when I think it should
	g_signal_connect (GTK_OBJECT (font_selection), "key-release-event", G_CALLBACK (change_font), (gpointer) window);

	// misc. setup
	
	radio_close_all = gtk_radio_button_new_with_label (NULL,
		"Close and save all pads");
	radio_close_this = gtk_radio_button_new_with_label_from_widget (
		GTK_RADIO_BUTTON (radio_close_all), "Close and save pad");
	radio_delete_this = gtk_radio_button_new_with_label_from_widget (
		GTK_RADIO_BUTTON (radio_close_all), "Delete pad (no confirmation)");
	frame_wm_close = gtk_frame_new ("Window Manager Close Action");
	vbox_wm_close = gtk_vbox_new (FALSE, 3);
	
	switch (current_settings.wm_close)
	{
	case 0: // close all
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_close_all), TRUE);
		break;
	default:
	case 1: // close pad
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_close_this), TRUE);
		break;
	case 2: // delete pad
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_delete_this), TRUE);
		break;
	}
	
	gtk_widget_set_sensitive (frame_wm_close, current_settings.decorations);
	
	gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_close_all, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_close_this, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_delete_this, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_wm_close), 6);
	gtk_container_add (GTK_CONTAINER (frame_wm_close), vbox_wm_close);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_decorations), current_settings.decorations);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_confirm_destroy), current_settings.confirm_destroy);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_edit_lock), current_settings.edit_lock);
	gtk_box_pack_start (GTK_BOX (hbox_misc), vbox_misc, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_edit_lock, FALSE, FALSE, 3);
	gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_confirm_destroy, FALSE, FALSE, 3);
	gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_decorations, FALSE, FALSE, 3);
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
	g_signal_connect (GTK_OBJECT (radio_close_all), "toggled", G_CALLBACK (change_wm_close), (gpointer) 0);
	g_signal_connect (GTK_OBJECT (radio_close_this), "toggled", G_CALLBACK (change_wm_close), (gpointer) 1);
	g_signal_connect (GTK_OBJECT (radio_delete_this), "toggled", G_CALLBACK (change_wm_close), (gpointer) 2);

	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_widget_show_all (window);
	
	return window;
}

void preferences_open (pad_node *pad)
{
	if (pref_window == NULL)
		pref_window = preferences_create (pad);
	else
		gtk_window_present (GTK_WINDOW (pref_window));
}
