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
#include "settings.h"
#include <string.h>

/* we keep a pointer around so that only one window will be open at a time */
GtkWidget *pref_window = NULL;
GSList *toolbar_widgets;


static gboolean change_background_color (GtkWidget *colorsel, GtkWidget *checkbutton)
{
	gboolean use_back = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)) ? 0 : 1;
	GdkColor c;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&c);
	
	if (use_back)
		xpad_settings_style_set_back_color (&c);
	else
		xpad_settings_style_set_back_color (NULL);
	
	gtk_widget_set_sensitive (colorsel, use_back);
	
	return FALSE;
}

static gboolean change_text_color (GtkWidget *colorsel, GtkWidget *checkbutton)
{
	gboolean use_text = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)) ? 0 : 1;
	GdkColor c;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&c);
	
	if (use_text)
		xpad_settings_style_set_text_color (&c);
	else
		xpad_settings_style_set_text_color (NULL);
	
	gtk_widget_set_sensitive (colorsel, use_text);
	
	return FALSE;
}

static gboolean change_border_color (GtkWidget *colorsel, GtkWidget *window)
{
	GdkColor c;
	
	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (colorsel), 
		&c);
	
	xpad_settings_style_set_border_color (&c);
	
	return FALSE;
}

static gboolean change_padding (GtkWidget *spinner, GtkWidget *window)
{
	xpad_settings_style_set_padding (
		gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner)));
	
	return FALSE;
}

static gboolean change_border_width (GtkWidget *spinner, GtkWidget *colorsel)
{
	gint width;
	
	width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	xpad_settings_style_set_border_width (width);
	
	gtk_widget_set_sensitive (colorsel, (width != 0));
	
	return FALSE;
}

static gboolean change_font (GtkWidget *fontsel, GtkWidget *checkbutton)
{
	gchar *fontname;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)))
		fontname = NULL;
	else
		fontname = gtk_font_selection_get_font_name (GTK_FONT_SELECTION (fontsel));
	
	xpad_settings_style_set_fontname (fontname);
	
	gtk_widget_set_sensitive (fontsel, fontname ? TRUE : FALSE);
	
	g_free (fontname);
	
	return FALSE;
}

static gboolean change_font_3_args (GtkWidget *fontsel, gpointer middle, GtkWidget *checkbutton)
{
	return change_font (fontsel, checkbutton);
}

static gboolean change_decorations (GtkWidget *checkbutton, GtkWidget *frame)
{
	gboolean decor = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	gtk_widget_set_sensitive (frame, decor);
	
	xpad_settings_set_has_decorations (decor, gtk_widget_get_toplevel (frame));
	
	return FALSE;
}

static gboolean change_scrollbars (GtkWidget *checkbutton, GtkWidget *window)
{
	xpad_settings_set_has_scrollbar (gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton)));
	
	return FALSE;
}

static gboolean change_sticky_on_start (GtkWidget *checkbutton, GtkWidget *window)
{
	xpad_settings_set_sticky_on_start (gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton)));
	
	return FALSE;
}

static gboolean change_confirm_destroy (GtkWidget *checkbutton, GtkWidget *window)
{
	xpad_settings_set_confirm_destroy (gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton)));
	
	return FALSE;
}

static gboolean change_edit_lock (GtkWidget *checkbutton, GtkWidget *window)
{
	xpad_settings_set_edit_lock (gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton)));
	
	return FALSE;
}

static gboolean change_wm_close (GtkWidget *radiobutton, gint num)
{
	enum XPAD_WM_CLOSE_ACTION action;
	
	switch (num)
	{
	case 0:
		action = XPAD_WM_CLOSE_ALL;
		break;
	default:
	case 1:
		action = XPAD_WM_CLOSE_PAD;
		break;
	case 2:
		action = XPAD_WM_DESTROY_PAD;
		break;
	}
	
	xpad_settings_set_wm_close (action);
	
	return FALSE;
}


static void
change_toolbar (GtkToggleButton *togglebutton, gpointer user_data)
{
	gint has = gtk_toggle_button_get_active (togglebutton);
	
	g_slist_foreach ((GSList *) user_data, (GFunc) gtk_widget_set_sensitive, GINT_TO_POINTER (has));
	
	xpad_settings_set_has_toolbar ((gboolean) has);
}

static void
change_auto_hide_toolbar (GtkToggleButton *togglebutton, gpointer user_data)
{
	xpad_settings_set_auto_hide_toolbar (
		gtk_toggle_button_get_active (togglebutton));
}

void pref_close (void)
{
	if (pref_window)
	{
		gtk_widget_destroy (pref_window);
		pref_window = NULL;
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
	
	gtk_widget_ref (source);
	gtk_container_remove (GTK_CONTAINER (parent), source);
	gtk_box_pack_start (GTK_BOX (user_data), source, FALSE, FALSE, 0);
	gtk_widget_unref (source);
	
	if (parent != GTK_WIDGET (user_data))
	{
		tb = (const toolbar_button *) g_object_get_data (G_OBJECT (source), "tb");
		
		xpad_settings_remove_toolbar_button (tb->name);
	}
}

static void
toolbar_data_receive (GtkWidget *widget, GdkDragContext *drag_context, gint x,
	gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	GtkWidget *source = gtk_drag_get_source_widget (drag_context);
	GtkWidget *parent = gtk_widget_get_parent (source);
	const toolbar_button *tb;
	
	gtk_widget_ref (source);
	gtk_container_remove (GTK_CONTAINER (parent), source);
	gtk_container_add (GTK_CONTAINER (user_data), source);
	gtk_widget_unref (source);
	
	tb = (const toolbar_button *) g_object_get_data (G_OBJECT (source), "tb");
	
	if (parent == GTK_WIDGET (user_data))
		xpad_settings_remove_toolbar_button (tb->name);
	
	xpad_settings_add_toolbar_button (tb->name);	
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
	GtkWidget *label_back = gtk_label_new (_("Background Color"));
	GtkWidget *label_text = gtk_label_new (_("Text Color"));
	GtkWidget *label_border = gtk_label_new (_("Border"));
	GtkWidget *label_border_width = gtk_label_new_with_mnemonic (_("Border _width:"));
	GtkWidget *label_border_width_unit = gtk_label_new (_("pixels"));
	GtkWidget *label_font = gtk_label_new (_("Font Face"));
	GtkWidget *label_misc = gtk_label_new (_("Options"));
	GtkWidget *label_toolbar = gtk_label_new (_("Toolbar"));
	GtkWidget *label_padding = gtk_label_new_with_mnemonic (_("_Padding:"));
	GtkWidget *label_padding_unit = gtk_label_new (_("pixels"));
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 0);
	GtkWidget *color_back = gtk_color_selection_new ();
	GtkWidget *color_text = gtk_color_selection_new ();
	GtkWidget *color_border = gtk_color_selection_new ();
	GtkWidget *font_selection = gtk_font_selection_new ();
	GtkWidget *checkbutton_decorations = gtk_check_button_new_with_mnemonic (_("Allow _window manager decorations"));
	GtkWidget *checkbutton_confirm_destroy = gtk_check_button_new_with_mnemonic (_("Con_firm pad destruction"));
	GtkWidget *checkbutton_edit_lock = gtk_check_button_new_with_mnemonic (_("_Edit lock"));
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
	gtk_container_set_border_width (GTK_CONTAINER (vbox_global), 6);
	gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 6);
	gtk_container_set_border_width (GTK_CONTAINER (window), 6);
	
	gtk_window_set_title (GTK_WINDOW(window), _("Global Preferences"));
	gtk_container_add (GTK_CONTAINER(window), vbox_global);

	/* buttonbox setup */
	g_signal_connect_swapped (GTK_OBJECT (button_close), "clicked", 
		G_CALLBACK (gtk_widget_destroy), (gpointer) window);
	g_signal_connect (GTK_OBJECT (window), "destroy", 
		G_CALLBACK (pref_close), NULL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(buttonbox), 0);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_close);
	
	/* vbox_global setup */
	gtk_box_pack_start (GTK_BOX(vbox_global), notebook, TRUE, TRUE, 6);
	gtk_box_pack_start (GTK_BOX(vbox_global), buttonbox, TRUE, TRUE, 0);

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
		
		gtk_label_set_mnemonic_widget (GTK_LABEL (label_border_width), spinner_border_width);
		gtk_label_set_mnemonic_widget (GTK_LABEL (label_padding), spinner_padding);
		
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
		GtkWidget *label_buttons_desc = gtk_label_new (_("Click and drag a button to move it."));
		GtkWidget *unused_frame = gtk_frame_new (NULL);
		GtkWidget *toolbar_frame = gtk_frame_new (NULL);
		GtkWidget *vbox_frame = gtk_vbox_new (FALSE, 0);
		GtkWidget *vbox_unused_frame = gtk_vbox_new (FALSE, 0);
		GtkWidget *vbox_toolbar_frame = gtk_vbox_new (FALSE, 0);
		GtkWidget *toolbar_on = gtk_check_button_new_with_mnemonic (_("_Enable toolbar"));
		GtkWidget *toolbar_auto_hide = gtk_check_button_new_with_mnemonic (_("_Auto-hide toolbar"));
		GtkWidget *align = gtk_alignment_new (0, 0, 0, 0);
		GtkWidget *hbox_buttons = gtk_hbox_new (FALSE, 0);
		GtkWidget *label_indent = gtk_label_new ("    ");
		
		{
			GtkWidget *label_frame = gtk_label_new (NULL);
			GtkWidget *label_unused = gtk_label_new (NULL);
			GtkWidget *label_used = gtk_label_new (NULL);
			
			gtk_label_set_markup (GTK_LABEL (label_frame), _("<b>Toolbar Buttons:</b>"));
			gtk_label_set_markup (GTK_LABEL (label_used), _("<b>Used Buttons:</b>"));
			gtk_label_set_markup (GTK_LABEL (label_unused), _("<b>Unused Buttons:</b>"));
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
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_on), xpad_settings_get_has_toolbar ());
		g_signal_connect (toolbar_on, "toggled", G_CALLBACK (change_toolbar), toolbar_widgets);
		
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), toolbar_auto_hide, FALSE, FALSE, 9);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_auto_hide), xpad_settings_get_auto_hide_toolbar ());
		g_signal_connect (toolbar_auto_hide, "toggled", G_CALLBACK (change_auto_hide_toolbar), NULL);
		
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), frame, FALSE, FALSE, 9);
		gtk_widget_set_sensitive (frame, xpad_settings_get_has_toolbar ());
		
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
			GSList *tmp, *start;
			const toolbar_button *tb;
			GtkWidget *b;
			
			tmp = start = xpad_settings_get_toolbar_buttons ();
			for (; tmp; tmp = tmp->next)
			{
				tb = (const toolbar_button *) tmp->data;
				
				if (!g_ascii_strcasecmp (tb->name, buttons[i].name))
					break;
			}
			
			if (tmp)	/* we found it, so we don't add it to our list of unused buttons */
				continue;
			
			g_slist_free (start);
			
			tb = &buttons[i];
			
			b = toolbar_button_new (tb);
			
			gtk_box_pack_start (GTK_BOX (buttonbox), b, FALSE,
				FALSE, 0);
			
			gtk_drag_source_set (b,
				GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_MOVE);
			
			g_signal_connect (b, "drag-data-get", 
				G_CALLBACK (data_get), NULL);
			
			g_object_set_data (G_OBJECT (b), "tb", (void *) tb);
			
			gtk_tooltips_set_tip (tt, b, _(buttons[i].desc), _(buttons[i].desc));
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
_("If on, a toolbar will appear below each pad when the mouse hovers over it."),
_("If on, a toolbar will appear below each pad when the mouse hovers over it."));
		
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tt), toolbar_auto_hide, 
_("If on, the toolbar will disappear when you are not using the pad."),
_("If on, the toolbar will disappear when you are not using the pad."));
		
		gtk_container_set_border_width (GTK_CONTAINER (vbox_frame), 6);
		gtk_container_set_border_width (GTK_CONTAINER (vbox_unused_frame), 6);
		gtk_container_set_border_width (GTK_CONTAINER (vbox_toolbar_frame), 6);
	}
	
	/* misc. setup */
	{
		GtkWidget *label_frame_wm = gtk_label_new (NULL);
		GtkWidget *label_frame_wm_indent = gtk_label_new ("    ");
		GtkWidget *label_frame_wm_hbox = gtk_hbox_new (FALSE, 0);
		GtkWidget *checkbutton_scrollbars = gtk_check_button_new_with_mnemonic (_("Allow _scrollbars"));
		GtkWidget *checkbutton_sticky = gtk_check_button_new_with_mnemonic (_("Pads start s_ticky"));
		
		radio_close_all = gtk_radio_button_new_with_mnemonic (NULL,
			_("Close and save _all pads"));
		radio_close_this = gtk_radio_button_new_with_mnemonic_from_widget (
			GTK_RADIO_BUTTON (radio_close_all), _("Close and save _pad"));
		radio_delete_this = gtk_radio_button_new_with_mnemonic_from_widget (
			GTK_RADIO_BUTTON (radio_close_all), _("_Destroy pad (no confirmation)"));
		frame_wm_close = gtk_frame_new (NULL);
		
		vbox_wm_close = gtk_vbox_new (FALSE, 3);
		
		gtk_label_set_markup (GTK_LABEL (label_frame_wm), _("<b>Window Manager Close Action:</b>"));
		gtk_frame_set_shadow_type (GTK_FRAME (frame_wm_close), GTK_SHADOW_NONE);
		gtk_frame_set_label_widget (GTK_FRAME (frame_wm_close), label_frame_wm);
		
		switch (xpad_settings_get_wm_close ())
		{
		case XPAD_WM_CLOSE_ALL: /* close all */
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_close_all), TRUE);
			break;
		default:
		case XPAD_WM_CLOSE_PAD: /* close pad */
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_close_this), TRUE);
			break;
		case XPAD_WM_DESTROY_PAD: /* delete pad */
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_delete_this), TRUE);
			break;
		}
		
		gtk_widget_set_sensitive (frame_wm_close, xpad_settings_get_has_decorations ());
		
		gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_close_all, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_close_this, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_wm_close), radio_delete_this, FALSE, FALSE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (vbox_wm_close), 6);
		gtk_box_pack_start (GTK_BOX (label_frame_wm_hbox), label_frame_wm_indent,
			FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (label_frame_wm_hbox), vbox_wm_close,
			FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (frame_wm_close), label_frame_wm_hbox);
	
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_decorations), xpad_settings_get_has_decorations ());
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_confirm_destroy), xpad_settings_get_confirm_destroy ());
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_edit_lock), xpad_settings_get_edit_lock ());
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_scrollbars), xpad_settings_get_has_scrollbar ());
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_sticky), xpad_settings_get_sticky_on_start ());
		gtk_box_pack_start (GTK_BOX (hbox_misc), vbox_misc, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_edit_lock, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_sticky, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_confirm_destroy, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_scrollbars, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_decorations, FALSE, FALSE, 9);
		gtk_box_pack_start (GTK_BOX (vbox_misc), frame_wm_close, FALSE, FALSE, 3);
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), hbox_misc, label_misc);
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_decorations, 
	_("If on, your window manager will add its own decorations to each pad.  For example, "
	"a titlebar and close button."),
	_("If on, your window manager will add its own decorations to each pad.  For example, "
	"a titlebar and close button."));
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_confirm_destroy, 
	_("If on, choosing to destroy a pad will prompt for conformation."),
	_("If on, choosing to destroy a pad will prompt for conformation."));
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_sticky, 
	_("If on, new pads are sticky by default."),
	_("If on, new pads are sticky by default."));
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_scrollbars, 
	_("If on, scrollbars appear when text is larger than the pad.  If off, the pad resizes "
	"to fit the text."),
	_("If on, scrollbars appear when text is larger than the pad.  If off, the pad resizes "
	"to fit the text."));
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), checkbutton_edit_lock, 
	_("If on, when a pad loses focus, it will become uneditable.  "
	"This allows you to move it by left dragging.  To make it editable again, "
	"double-click the pad.  If off, pads are always editable."),
	_("If on, when a pad loses focus, it will become uneditable.  "
	"This allows you to move it by left dragging.  To make it editable again, "
	"double-click the pad.  If off, pads are always editable."));
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), radio_close_all, 
	_("All pads will close, saving contents first."),
	_("All pads will close, saving contents first."));
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), radio_close_this, 
	_("The pad you clicked the close button on will close, saving contents."),
	_("The pad you clicked the close button on will close, saving contents."));
	
		gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips_options), radio_delete_this, 
	_("The pad you clicked the close button on will be destroyed, losing contents.  There "
	"will be no confirmation."),
	_("The pad you clicked the close button on will be destroyed, losing contents.  There "
	"will be no confirmation."));
	
		g_signal_connect (GTK_OBJECT (checkbutton_confirm_destroy), "toggled", G_CALLBACK (change_confirm_destroy), (gpointer) window);
		g_signal_connect (GTK_OBJECT (checkbutton_decorations), "toggled", G_CALLBACK (change_decorations), (gpointer) frame_wm_close);
		g_signal_connect (GTK_OBJECT (checkbutton_edit_lock), "toggled", G_CALLBACK (change_edit_lock), (gpointer) window);
		g_signal_connect (GTK_OBJECT (checkbutton_scrollbars), "toggled", G_CALLBACK (change_scrollbars), (gpointer) window);
		g_signal_connect (GTK_OBJECT (checkbutton_sticky), "toggled", G_CALLBACK (change_sticky_on_start), (gpointer) window);
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
