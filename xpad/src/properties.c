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
#include "xpad-settings.h"
#include "pad.h"
#include "defines.h"
#include "xpad-text-view.h"


static gboolean change_back_color (GtkWidget *colorbutton, GtkWidget *checkbutton)
{
	gboolean use_back = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton));
	GdkColor c;
	pad_node *pad;
	
	pad = (pad_node *) g_object_get_data (G_OBJECT (gtk_widget_get_toplevel (colorbutton)), "pad");
	gtk_color_button_get_color (GTK_COLOR_BUTTON (colorbutton), &c);
	
	if (use_back)
		pad_set_back_color (pad, &c);
	else
		pad_set_back_color (pad, NULL);
	
	gtk_widget_set_sensitive (colorbutton, use_back);
	
	return FALSE;
}

static gboolean change_text_color (GtkWidget *colorbutton, GtkWidget *checkbutton)
{
	gboolean use_text = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton));
	GdkColor c;
	pad_node *pad;
	
	pad = (pad_node *) g_object_get_data (G_OBJECT (gtk_widget_get_toplevel (colorbutton)), "pad");
	gtk_color_button_get_color (GTK_COLOR_BUTTON (colorbutton), &c);
	
	if (use_text)
		pad_set_text_color (pad, &c);
	else
		pad_set_text_color (pad, NULL);
	
	gtk_widget_set_sensitive (colorbutton, use_text);
	
	return FALSE;
}

static gboolean change_font_face (GtkWidget *fontbutton, GtkWidget *checkbutton)
{
	G_CONST_RETURN gchar *fontname;
	pad_node *pad;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)))
		fontname = gtk_font_button_get_font_name (GTK_FONT_BUTTON (fontbutton));
	else
		fontname = NULL;
	
	pad = (pad_node *) g_object_get_data (
		G_OBJECT (gtk_widget_get_toplevel (fontbutton)), "pad");
	pad_set_fontname (pad, fontname);
	
	gtk_widget_set_sensitive (fontbutton, fontname ? TRUE : FALSE);
	
	return FALSE;
}

static void set_values (GObject *window)
{
	gpointer *p;
	GtkRcStyle *style;
	gchar *fontname;
	
	p = g_object_get_data (window, "pad");
	style = gtk_widget_get_modifier_style (((pad_node *) p)->textview);
	fontname = style->font_desc ? pango_font_description_to_string (style->font_desc) : NULL;
	
	p = g_object_get_data (window, "use_text");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (p), (style->color_flags[GTK_STATE_NORMAL] & GTK_RC_TEXT) ? TRUE : FALSE);
	
	p = g_object_get_data (window, "text");
	gtk_color_button_set_color (GTK_COLOR_BUTTON (p), &style->text[GTK_STATE_NORMAL]);
	gtk_widget_set_sensitive (GTK_WIDGET (p), (style->color_flags[GTK_STATE_NORMAL] & GTK_RC_TEXT) ? TRUE : FALSE);
	
	p = g_object_get_data (window, "use_back");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (p), (style->color_flags[GTK_STATE_NORMAL] & GTK_RC_BASE) ? TRUE : FALSE);
	
	p = g_object_get_data (window, "back");
	gtk_color_button_set_color (GTK_COLOR_BUTTON (p), &style->base[GTK_STATE_NORMAL]);
	gtk_widget_set_sensitive (GTK_WIDGET (p), (style->color_flags[GTK_STATE_NORMAL] & GTK_RC_BASE) ? TRUE : FALSE);
	
	p = g_object_get_data (window, "use_font");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (p), fontname ? TRUE : FALSE);
	
	p = g_object_get_data (window, "font");
	if (fontname) gtk_font_button_set_font_name (GTK_FONT_BUTTON (p), fontname);
	gtk_widget_set_sensitive (GTK_WIDGET (p), fontname ? TRUE : FALSE);
	
	g_free (fontname);
}

static gboolean change_use_global (GtkToggleButton *togglebutton, GtkWidget *widget)
{
	gboolean use_global = !gtk_toggle_button_get_active (togglebutton);
	pad_node *pad;
	
	pad = (pad_node *) g_object_get_data (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (togglebutton))), "pad");
	
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
	
	set_values (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (togglebutton))));
	
	return FALSE;
}

static GtkWidget *properties_create (pad_node *pad)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *buttonbox = gtk_hbutton_box_new ();
	GtkWidget *button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 12);
	GtkWidget *checkbutton_locked = gtk_check_button_new_with_mnemonic (_("Use custom _appearance"));
	GtkWidget *vbox_appearance = gtk_vbox_new (FALSE, 6);
	GtkWidget *hbox_appearance = gtk_hbox_new (FALSE, 0);
	gchar *title = g_strdup_printf (_("%s Properties"), pad->title);
	
	GtkSizeGroup *size_group_labels = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	GtkSizeGroup *size_group_buttons = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	
	GtkWidget *check_button_text = gtk_check_button_new_with_mnemonic (_("Use custom _text color:"));
	GtkWidget *check_button_back = gtk_check_button_new_with_mnemonic (_("Use custom _background color:"));
	GtkWidget *check_button_face = gtk_check_button_new_with_mnemonic (_("Use custom _font:"));
	
	GtkWidget *color_button_text = gtk_color_button_new ();
	GtkWidget *color_button_back = gtk_color_button_new ();
	GtkWidget *font_button_face = gtk_font_button_new ();
	
	GtkWidget *hbox_text = gtk_hbox_new (FALSE, 12);
	GtkWidget *hbox_back = gtk_hbox_new (FALSE, 12);
	GtkWidget *hbox_face = gtk_hbox_new (FALSE, 12);
	
	gtk_window_set_title (GTK_WINDOW (window), title);
	g_free (title);
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_container_add (GTK_CONTAINER (window), vbox_global);
	
	gtk_container_set_border_width (GTK_CONTAINER (window), 12);
	
	/* buttonbox setup */
	g_signal_connect_swapped (GTK_OBJECT (button_close), "clicked", 
		G_CALLBACK (gtk_widget_destroy), (gpointer) window);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX (buttonbox), 0);
	gtk_box_pack_end_defaults (GTK_BOX (buttonbox), button_close);
	
	/* vbox_global setup */
	gtk_box_pack_start (GTK_BOX (vbox_global), checkbutton_locked, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_global), hbox_appearance, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_global), buttonbox, FALSE, FALSE, 0);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_locked), !xpad_text_view_get_follow_global_style (XPAD_TEXT_VIEW (pad->textview)));
	gtk_widget_set_sensitive (hbox_appearance, !xpad_text_view_get_follow_global_style (XPAD_TEXT_VIEW (pad->textview)));
	
	gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (color_button_text), FALSE);
	gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (color_button_back), FALSE);
	
	gtk_color_button_set_title (GTK_COLOR_BUTTON (color_button_text), _("Set Custom Text Color"));
	gtk_color_button_set_title (GTK_COLOR_BUTTON (color_button_back), _("Set Custom Background Color"));
	gtk_font_button_set_title (GTK_FONT_BUTTON (font_button_face), _("Set Custom Font"));
	
	gtk_size_group_add_widget (size_group_labels, check_button_text);
	gtk_size_group_add_widget (size_group_labels, check_button_back);
	gtk_size_group_add_widget (size_group_labels, check_button_face);
	
	gtk_size_group_add_widget (size_group_buttons, color_button_text);
	gtk_size_group_add_widget (size_group_buttons, color_button_back);
	gtk_size_group_add_widget (size_group_buttons, font_button_face);
	
	g_object_set_data (G_OBJECT (window), "use_text", check_button_text);
	g_object_set_data (G_OBJECT (window), "use_back", check_button_back);
	g_object_set_data (G_OBJECT (window), "use_font", check_button_face);
	
	g_object_set_data (G_OBJECT (window), "text", color_button_text);
	g_object_set_data (G_OBJECT (window), "back", color_button_back);
	g_object_set_data (G_OBJECT (window), "font", font_button_face);
	
	gtk_box_pack_start (GTK_BOX (hbox_appearance), vbox_appearance, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (hbox_text), check_button_text, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox_text), color_button_text, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_appearance), hbox_text, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (hbox_back), check_button_back, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox_back), color_button_back, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_appearance), hbox_back, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (hbox_face), check_button_face, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox_face), font_button_face, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_appearance), hbox_face, FALSE, FALSE, 0);
	
	g_object_set_data (G_OBJECT (window), "pad", pad);
	g_object_set_data (G_OBJECT (window), "use_custom", checkbutton_locked);
	
	set_values (G_OBJECT (window));
	
	g_signal_connect (G_OBJECT (checkbutton_locked), "toggled", G_CALLBACK (change_use_global), hbox_appearance);
	
	g_signal_connect_swapped (G_OBJECT (check_button_text), "toggled", G_CALLBACK (change_text_color), (gpointer) color_button_text);
	g_signal_connect_swapped (G_OBJECT (check_button_back), "toggled", G_CALLBACK (change_back_color), (gpointer) color_button_back);
	g_signal_connect_swapped (G_OBJECT (check_button_face), "toggled", G_CALLBACK (change_font_face), (gpointer) font_button_face);
	
	g_signal_connect (G_OBJECT (color_button_text), "color-set", G_CALLBACK (change_text_color), (gpointer) check_button_text);
	g_signal_connect (G_OBJECT (color_button_back), "color-set", G_CALLBACK (change_back_color), (gpointer) check_button_back);
	g_signal_connect (G_OBJECT (font_button_face), "font-set", G_CALLBACK (change_font_face), (gpointer) check_button_face);
	
	g_signal_connect_swapped (G_OBJECT (window), "destroy", G_CALLBACK (g_nullify_pointer), &pad->properties);
	gtk_window_set_transient_for (GTK_WINDOW (window), pad->window);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);
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
