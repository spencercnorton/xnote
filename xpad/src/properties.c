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

#include "properties.h"
#include "settings.h"
#include "pad.h"


static gboolean change_background_color (GtkWidget *colorsel, GtkWidget *checkbutton)
{
	gboolean use_back = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)) ? 0 : 1;
	GdkColor c;
	pad_node *pad;
	
	pad = (pad_node *) g_object_get_data (
		G_OBJECT (gtk_widget_get_toplevel (colorsel)), "pad");
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&c);
	
	if (use_back)
		pad_set_back_color (pad, &c);
	else
		pad_set_back_color (pad, NULL);
	
	gtk_widget_set_sensitive (colorsel, use_back);
	
	return FALSE;
}

static gboolean change_text_color (GtkWidget *colorsel, GtkWidget *checkbutton)
{
	gboolean use_text = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)) ? 0 : 1;
	GdkColor c;
	pad_node *pad;
	
	pad = (pad_node *) g_object_get_data (
		G_OBJECT (gtk_widget_get_toplevel (colorsel)), "pad");
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&c);
	
	if (use_text)
		pad_set_text_color (pad, &c);
	else
		pad_set_text_color (pad, NULL);
	
	gtk_widget_set_sensitive (colorsel, use_text);
	
	return FALSE;
}

static gboolean change_border_color (GtkWidget *colorsel, GtkWidget *window)
{
	GdkColor c;
	pad_node *pad;
	
	pad = (pad_node *) g_object_get_data (
		G_OBJECT (gtk_widget_get_toplevel (colorsel)), "pad");
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&c);
	
	pad_set_border_color (pad, &c);
	
	return FALSE;
}

static gboolean change_padding (GtkWidget *spinner, GtkWidget *window)
{
	pad_node *pad;
	
	pad = (pad_node *) g_object_get_data (
		G_OBJECT (gtk_widget_get_toplevel (spinner)), "pad");
	pad_set_padding (pad, 
		gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner)));
	
	return FALSE;
}

static gboolean change_border_width (GtkWidget *spinner, GtkWidget *colorsel)
{
	gint width;
	pad_node *pad;
	
	width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	pad = (pad_node *) g_object_get_data (
		G_OBJECT (gtk_widget_get_toplevel (spinner)), "pad");
	pad_set_border_width (pad, width);
	
	gtk_widget_set_sensitive (colorsel, (width != 0));
	
	return FALSE;
}

static gboolean change_font (GtkWidget *fontsel, GtkWidget *checkbutton)
{
	gchar *fontname;
	pad_node *pad;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)))
		fontname = NULL;
	else
		fontname = gtk_font_selection_get_font_name (GTK_FONT_SELECTION (fontsel));
	
	pad = (pad_node *) g_object_get_data (
		G_OBJECT (gtk_widget_get_toplevel (fontsel)), "pad");
	pad_set_fontname (pad, fontname);
	
	gtk_widget_set_sensitive (fontsel, fontname ? TRUE : FALSE);
	
	g_free (fontname);
	
	return FALSE;
}

static gboolean change_font_3_args (GtkWidget *fontsel, gpointer middle, GtkWidget *checkbutton)
{
	return change_font (fontsel, checkbutton);
}

static gboolean change_use_global (GtkToggleButton *togglebutton, GtkWidget *widget)
{
	gboolean use_global = gtk_toggle_button_get_active (togglebutton);
	pad_node *pad;
	
	pad = (pad_node *) g_object_get_data (
		G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (togglebutton))), "pad");
	
	if (use_global)
	{
		gtk_widget_set_sensitive (widget, FALSE);
		
		pad_unlock_style (pad);
	}
	else
	{
		gtk_widget_set_sensitive (widget, TRUE);
		
		pad_lock_style (pad);
	}
	
	return FALSE;
}


/**
 * Note to the uncautious:  The following function is ugly as hell.  There are a lot of 
 * widgets define at the top that are not used until several pages down, there are random
 * statement blocks in the middle because it was easier to code it that way.  No consistent
 * naming scheme is used, and there is no order to most of the gtk function calls.  Proceed
 * at your own risk.  I am not responsible for any mental illness as a result.
 */
static GtkWidget *properties_create (pad_node *pad)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *notebook = gtk_notebook_new ();
	GtkWidget *buttonbox = gtk_hbutton_box_new ();
	GtkWidget *button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GtkWidget *label_back = gtk_label_new (_("Background Color"));
	GtkWidget *label_text = gtk_label_new (_("Text Color"));
	GtkWidget *label_border = gtk_label_new (_("Border"));
	GtkWidget *label_border_width = gtk_label_new_with_mnemonic (_("Border _width:"));
	GtkWidget *label_border_width_unit = gtk_label_new (_("pixels"));
	GtkWidget *label_font = gtk_label_new (_("Font Face"));
	GtkWidget *label_padding = gtk_label_new_with_mnemonic (_("_Padding:"));
	GtkWidget *label_padding_unit = gtk_label_new (_("pixels"));
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 0);
	GtkWidget *color_back = gtk_color_selection_new ();
	GtkWidget *color_text = gtk_color_selection_new ();
	GtkWidget *color_border = gtk_color_selection_new ();
	GtkWidget *font_selection = gtk_font_selection_new ();
	GtkWidget *separator_border = gtk_hseparator_new ();
	GtkWidget *separator_locked = gtk_hseparator_new ();
	GtkObject *adjust_padding;
	GtkWidget *spinner_padding;
	GtkObject *adjust_border_width;
	GtkWidget *spinner_border_width;
	GtkWidget *checkbutton_locked = gtk_check_button_new_with_mnemonic ("Use _global settings");
	GtkWidget *vbox_background = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_background = gtk_hbox_new (FALSE, 0);
	GtkWidget *vbox_text = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_text = gtk_hbox_new (FALSE, 0);
	GtkWidget *vbox_border = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_border = gtk_hbox_new (FALSE, 0);
	GtkWidget *vbox_font = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox_font = gtk_hbox_new (FALSE, 0);
	GtkWidget *hbox_padding = gtk_hbox_new (FALSE, 3);
	GtkWidget *hbox_border_width = gtk_hbox_new (FALSE, 3);
	GtkWidget *hbox_border_entries = gtk_hbox_new (FALSE, 3);
	GtkTooltips *tooltips_border = gtk_tooltips_new ();
	
	gtk_container_set_border_width (GTK_CONTAINER (vbox_background), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_background), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_text), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_text), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_border), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_border), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_font), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_font), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_padding), 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_border_width), 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_border_entries), 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_global), 6);
	gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 6);
	gtk_container_set_border_width (GTK_CONTAINER (window), 6);
	
	gtk_window_set_title (GTK_WINDOW (window), _("Pad Properties"));
	gtk_container_add (GTK_CONTAINER (window), vbox_global);
	
	/* buttonbox setup */
	g_signal_connect_swapped (GTK_OBJECT (button_close), "clicked", 
		G_CALLBACK (gtk_widget_destroy), (gpointer) window);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX (buttonbox), 0);
	gtk_box_pack_end_defaults (GTK_BOX (buttonbox), button_close);
	
	/* vbox_global setup */
	gtk_box_pack_start (GTK_BOX (vbox_global), checkbutton_locked, TRUE, TRUE, 6);
	gtk_box_pack_start (GTK_BOX (vbox_global), separator_locked, TRUE, TRUE, 6);
	gtk_box_pack_start (GTK_BOX (vbox_global), notebook, TRUE, TRUE, 6);
	gtk_box_pack_start (GTK_BOX (vbox_global), buttonbox, TRUE, TRUE, 0);
	
	if (!pad->locked)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_locked), TRUE);
		gtk_widget_set_sensitive (notebook, FALSE);
	}
	
	g_signal_connect (G_OBJECT (checkbutton_locked), "toggled", G_CALLBACK (change_use_global), notebook);
	
	/* text setup */
	{
		GtkWidget *separator = gtk_hseparator_new ();
		GtkWidget *checkbutton_use = gtk_check_button_new_with_mnemonic (_("_Use system text color"));
		GdkColor c = xpad_settings_style_get_text_color ();
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_text, label_text);
		gtk_box_pack_start (GTK_BOX (hbox_text), vbox_text, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (vbox_text), checkbutton_use, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_use),  xpad_settings_style_get_system_text ());
		g_signal_connect_swapped (GTK_OBJECT (checkbutton_use), "toggled", G_CALLBACK (change_text_color), (gpointer) color_text);
		
		gtk_box_pack_start (GTK_BOX (vbox_text), separator, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_text), color_text, FALSE, FALSE, 9);
		gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_text), &c);
		gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_text), FALSE);
		gtk_widget_set_sensitive (color_text, !xpad_settings_style_get_system_text ());
		g_signal_connect (GTK_OBJECT (color_text), "color-changed", G_CALLBACK (change_text_color), (gpointer) checkbutton_use);
		
	}
	
	/* background setup */
	{
		GtkWidget *separator = gtk_hseparator_new ();
		GtkWidget *checkbutton_use = gtk_check_button_new_with_mnemonic (_("_Use system background color"));
		GdkColor c = xpad_settings_style_get_back_color ();
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_background, label_back);
		gtk_box_pack_start (GTK_BOX (hbox_background), vbox_background, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (vbox_background), checkbutton_use, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_use), xpad_settings_style_get_system_back ());
		g_signal_connect_swapped (GTK_OBJECT (checkbutton_use), "toggled", G_CALLBACK (change_background_color), (gpointer) color_back);
		
		gtk_box_pack_start (GTK_BOX (vbox_background), separator, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_background), color_back, FALSE, FALSE, 9);
		gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_back), &c);
		gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_back), FALSE);
		gtk_widget_set_sensitive (color_back, !xpad_settings_style_get_system_back ());
		g_signal_connect (GTK_OBJECT (color_back), "color-changed", G_CALLBACK (change_background_color), (gpointer) checkbutton_use);
	}
	
	/* border setup */
	{
		GdkColor c = xpad_settings_style_get_border_color ();
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_border, label_border);
		gtk_box_pack_start (GTK_BOX (hbox_border), vbox_border, FALSE, FALSE, 0);
		
		adjust_padding = gtk_adjustment_new (xpad_settings_style_get_padding (), 0.0, 100.0, 1.0, 5.0, 5.0);
		spinner_padding = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_padding), 1.0, 0);
		gtk_misc_set_alignment (GTK_MISC (label_padding), 0, 0.5);
		gtk_entry_set_width_chars (GTK_ENTRY (spinner_padding), 3);
		
		gtk_box_pack_start (GTK_BOX (hbox_padding), label_padding, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_padding), spinner_padding, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_padding), label_padding_unit, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (spinner_padding), "value-changed", G_CALLBACK (change_padding), (gpointer) color_border);
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_border), spinner_padding, 
	_("The amount of space you want between the border and text."),
	_("Choose the number of pixels around the text region.  This space is colored "
	"the same and surrounds it on all sides."));
		
		adjust_border_width = gtk_adjustment_new (xpad_settings_style_get_border_width (), 0.0, 100.0, 1.0, 5.0, 5.0);
		spinner_border_width = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_border_width), 1.0, 0);
		gtk_misc_set_alignment (GTK_MISC (label_border_width), 0, 0.5);
		gtk_entry_set_width_chars (GTK_ENTRY (spinner_border_width), 3);
		
		gtk_label_set_mnemonic_widget (GTK_LABEL (label_border_width), spinner_border_width);
		gtk_label_set_mnemonic_widget (GTK_LABEL (label_padding), spinner_padding);
		
		gtk_box_pack_start (GTK_BOX (hbox_border_width), label_border_width, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_border_width), spinner_border_width, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox_border_width), label_border_width_unit, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (spinner_border_width), "value-changed", G_CALLBACK (change_border_width), (gpointer) color_border);
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_border), spinner_border_width, 
	_("The amount of space you want surrounding the pad."), 
	_("Choose the number of pixels around the pad.  This space is colored "
	"independently and surrounds it on all sides."));
		
		gtk_box_pack_start_defaults (GTK_BOX (hbox_border_entries), hbox_border_width);
		gtk_box_pack_start_defaults (GTK_BOX (hbox_border_entries), hbox_padding);
		gtk_box_pack_start (GTK_BOX (vbox_border), hbox_border_entries, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_border), separator_border, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_border), color_border, FALSE, FALSE, 9);
		
		gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_border), &c);
		gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_border), FALSE);
		g_signal_connect (GTK_OBJECT (color_border), "color-changed", G_CALLBACK (change_border_color), (gpointer) window);
		
		if (xpad_settings_style_get_border_width () == 0)
			gtk_widget_set_sensitive (color_border, FALSE);
	}
	
	/* font setup */
	{
		GtkWidget *separator = gtk_hseparator_new ();
		GtkWidget *checkbutton_use = gtk_check_button_new_with_mnemonic (_("_Use system font face"));
		
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_font, label_font);
		gtk_box_pack_start (GTK_BOX (hbox_font), vbox_font, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (vbox_font), checkbutton_use, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_use), xpad_settings_style_get_fontname () ? 0 : 1);
		g_signal_connect_swapped (GTK_OBJECT (checkbutton_use), "toggled", G_CALLBACK (change_font), (gpointer) font_selection);
		
		gtk_box_pack_start (GTK_BOX (vbox_font), separator, FALSE, FALSE, 9);
		
		gtk_box_pack_start (GTK_BOX (vbox_font), font_selection, FALSE, FALSE, 9);
		gtk_widget_set_sensitive (font_selection, xpad_settings_style_get_fontname () ? 1 : 0);
		
		if (xpad_settings_style_get_fontname ())
			gtk_font_selection_set_font_name (GTK_FONT_SELECTION (font_selection), 
				xpad_settings_style_get_fontname ());
		
		/* this is a bit hacky, but there is no font-changed signal! */
		g_signal_connect (GTK_OBJECT (font_selection), "button-release-event", G_CALLBACK (change_font_3_args), (gpointer) checkbutton_use);
		/* key release event does not seem to be sent when I think it should */
		gtk_widget_add_events(font_selection, GDK_KEY_RELEASE_MASK);
		g_signal_connect (GTK_OBJECT (font_selection), "key-release-event", G_CALLBACK (change_font_3_args), (gpointer) checkbutton_use);
	}
	
	g_object_set_data (G_OBJECT (window), "pad", pad);
	
	g_signal_connect_swapped (G_OBJECT (window), "destroy", G_CALLBACK (g_nullify_pointer), &pad->properties);
	gtk_window_set_transient_for (GTK_WINDOW (window), pad->window);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_show_all (window);
	
	return window;
}

void properties_open (pad_node *pad)
{
	if (pad->properties == NULL)
		pad->properties = properties_create (pad);
	else
		gtk_window_present (GTK_WINDOW (pad->properties));
}

void properties_close (pad_node *pad)
{
	if (pad->properties)
	{
		gtk_widget_destroy (pad->properties);
		pad->properties = NULL;
	}
}
