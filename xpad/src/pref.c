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
#include "pref.h"
#include "fio.h"
#include "pad.h"
#include "help.h"
#include "xpad-settings.h"

/* we keep a pointer around so that only one window will be open at a time */
GtkWidget *pref_window = NULL;
GSList *toolbar_widgets;


static gboolean change_back_color (GtkWidget *colorbutton, GtkWidget *checkbutton)
{
	gboolean use_back = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton));
	GdkColor c;
	
	gtk_color_button_get_color (GTK_COLOR_BUTTON (colorbutton), &c);
	
	if (use_back)
		xpad_settings_set_back_color (xpad_settings (), &c);
	else
		xpad_settings_set_back_color (xpad_settings (), NULL);
	
	gtk_widget_set_sensitive (colorbutton, use_back);
	
	return FALSE;
}

static gboolean change_text_color (GtkWidget *colorbutton, GtkWidget *checkbutton)
{
	gboolean use_text = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton));
	GdkColor c;
	
	gtk_color_button_get_color (GTK_COLOR_BUTTON (colorbutton), &c);
	
	if (use_text)
		xpad_settings_set_text_color (xpad_settings (), &c);
	else
		xpad_settings_set_text_color (xpad_settings (), NULL);
	
	gtk_widget_set_sensitive (colorbutton, use_text);
	
	return FALSE;
}

static gboolean change_font_face (GtkWidget *fontbutton, GtkWidget *checkbutton)
{
	G_CONST_RETURN gchar *fontname;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)))
		fontname = gtk_font_button_get_font_name (GTK_FONT_BUTTON (fontbutton));
	else
		fontname = NULL;
	
	xpad_settings_set_fontname (xpad_settings (), fontname);
	
	gtk_widget_set_sensitive (fontbutton, fontname ? TRUE : FALSE);
	
	return FALSE;
}

static gboolean change_decorations (GtkWidget *checkbutton, gpointer data)
{
	gboolean decor = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton));
	
	xpad_settings_set_has_decorations (xpad_settings (), decor);
	
	return FALSE;
}

static gboolean change_scrollbars (GtkWidget *checkbutton, GtkWidget *window)
{
	xpad_settings_set_has_scrollbar (xpad_settings (), gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton)));
	
	return FALSE;
}

static gboolean change_sticky_on_start (GtkWidget *checkbutton, GtkWidget *window)
{
	xpad_settings_set_sticky (xpad_settings (), gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton)));
	
	return FALSE;
}

static gboolean change_confirm_destroy (GtkWidget *checkbutton, GtkWidget *window)
{
	xpad_settings_set_confirm_destroy (xpad_settings (), gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton)));
	
	return FALSE;
}

static gboolean change_edit_lock (GtkWidget *checkbutton, GtkWidget *window)
{
	xpad_settings_set_edit_lock (xpad_settings (), gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkbutton)));
	
	return FALSE;
}

#if 0
static void
change_toolbar (GtkToggleButton *togglebutton, gpointer user_data)
{
	gint has = gtk_toggle_button_get_active (togglebutton);
	
	g_slist_foreach ((GSList *) user_data, (GFunc) gtk_widget_set_sensitive, GINT_TO_POINTER (has));
	
	xpad_settings_set_has_toolbar (xpad_settings (), (gboolean) has);
}

static void
change_auto_hide_toolbar (GtkToggleButton *togglebutton, gpointer user_data)
{
	xpad_settings_set_autohide_toolbar (xpad_settings (), 
		gtk_toggle_button_get_active (togglebutton));
}
#endif

void pref_close (void)
{
	if (pref_window)
	{
		gtk_widget_destroy (pref_window);
		pref_window = NULL;
	}
}

#if 0
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
		
		xpad_settings_remove_toolbar_button (xpad_settings (), tb->name);
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
		xpad_settings_remove_toolbar_button (xpad_settings (), tb->name);
	
	xpad_settings_add_toolbar_button (xpad_settings (), tb->name);	
}
#endif
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
	GtkWidget *label_appearance = gtk_label_new (_("Appearance"));
	GtkWidget *label_misc = gtk_label_new (_("Options"));
/*	GtkWidget *label_toolbar = gtk_label_new (_("Toolbar"));*/
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 12);
	GtkWidget *checkbutton_decorations = gtk_check_button_new_with_mnemonic (_("Allow _window manager decorations"));
	GtkWidget *checkbutton_confirm_destroy = gtk_check_button_new_with_mnemonic (_("Con_firm pad destruction"));
	GtkWidget *checkbutton_edit_lock = gtk_check_button_new_with_mnemonic (_("_Edit lock"));
	GtkWidget *vbox_appearance = gtk_vbox_new (FALSE, 6);
	GtkWidget *hbox_appearance = gtk_hbox_new (FALSE, 0);
	GtkWidget *vbox_misc = gtk_vbox_new (FALSE, 6);
	GtkWidget *hbox_misc = gtk_hbox_new (FALSE, 0);
	
	GtkTooltips *tooltips_options = gtk_tooltips_new ();
	
	gtk_container_set_border_width (GTK_CONTAINER (vbox_appearance), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_appearance), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_misc), 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_misc), 6);
	gtk_container_set_border_width (GTK_CONTAINER (window), 12);
	
	gtk_window_set_title (GTK_WINDOW(window), _("Xpad Preferences"));
	gtk_container_add (GTK_CONTAINER(window), vbox_global);

	/* buttonbox setup */
	g_signal_connect (GTK_OBJECT (button_close), "clicked", G_CALLBACK (pref_close), NULL);
	g_signal_connect (GTK_OBJECT (window), "delete-event", G_CALLBACK (pref_close), NULL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(buttonbox), 0);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_close);
	
	/* vbox_global setup */
	gtk_box_pack_start (GTK_BOX(vbox_global), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox_global), buttonbox, TRUE, TRUE, 0);

	/* appearance setup */
	{
		GtkSizeGroup *size_group_labels = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		GtkSizeGroup *size_group_buttons = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		
		GtkWidget *check_button_text = gtk_check_button_new_with_mnemonic (_("Use custom _text color:"));
		GtkWidget *check_button_back = gtk_check_button_new_with_mnemonic (_("Use custom _background color:"));
		GtkWidget *check_button_face = gtk_check_button_new_with_mnemonic (_("Use custom _font:"));
		
		GtkWidget *color_button_text = xpad_settings_get_text_color (xpad_settings ()) ? gtk_color_button_new_with_color (xpad_settings_get_text_color (xpad_settings ())) : gtk_color_button_new ();
		GtkWidget *color_button_back = xpad_settings_get_back_color (xpad_settings ()) ? gtk_color_button_new_with_color (xpad_settings_get_back_color (xpad_settings ())) : gtk_color_button_new ();
		GtkWidget *font_button_face = xpad_settings_get_fontname (xpad_settings ()) ? gtk_font_button_new_with_font (xpad_settings_get_fontname (xpad_settings ())) : gtk_font_button_new ();
		
		GtkWidget *hbox_text = gtk_hbox_new (FALSE, 12);
		GtkWidget *hbox_back = gtk_hbox_new (FALSE, 12);
		GtkWidget *hbox_face = gtk_hbox_new (FALSE, 12);
		
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
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button_text), xpad_settings_get_text_color (xpad_settings ()) ? TRUE : FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button_back), xpad_settings_get_back_color (xpad_settings ()) ? TRUE : FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button_face), xpad_settings_get_fontname (xpad_settings ()) ? TRUE : FALSE);
		
		gtk_widget_set_sensitive (color_button_text, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_button_text)));
		gtk_widget_set_sensitive (color_button_back, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_button_back)));
		gtk_widget_set_sensitive (font_button_face, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_button_face)));
		
		g_signal_connect_swapped (GTK_OBJECT (check_button_text), "toggled", G_CALLBACK (change_text_color), (gpointer) color_button_text);
		g_signal_connect_swapped (GTK_OBJECT (check_button_back), "toggled", G_CALLBACK (change_back_color), (gpointer) color_button_back);
		g_signal_connect_swapped (GTK_OBJECT (check_button_face), "toggled", G_CALLBACK (change_font_face), (gpointer) font_button_face);
		
		g_signal_connect (GTK_OBJECT (color_button_text), "color-set", G_CALLBACK (change_text_color), (gpointer) check_button_text);
		g_signal_connect (GTK_OBJECT (color_button_back), "color-set", G_CALLBACK (change_back_color), (gpointer) check_button_back);
		g_signal_connect (GTK_OBJECT (font_button_face), "font-set", G_CALLBACK (change_font_face), (gpointer) check_button_face);
		
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), hbox_appearance, label_appearance);
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
	}
	
	#if 0
	/* toolbar  setup */
	{
		GtkWidget *vbox_toolbar = gtk_vbox_new (FALSE, 6);
		GtkWidget *hbox_toolbar = gtk_hbox_new (FALSE, 0);
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
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), toolbar_on, FALSE, FALSE, 0);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_on), xpad_settings_get_has_toolbar (xpad_settings ()));
		g_signal_connect (toolbar_on, "toggled", G_CALLBACK (change_toolbar), toolbar_widgets);
		
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), toolbar_auto_hide, FALSE, FALSE, 0);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_auto_hide), xpad_settings_get_autohide_toolbar (xpad_settings ()));
		g_signal_connect (toolbar_auto_hide, "toggled", G_CALLBACK (change_auto_hide_toolbar), NULL);
		gtk_widget_set_sensitive (toolbar_auto_hide, xpad_settings_get_has_toolbar (xpad_settings ()));
		
		gtk_box_pack_start (GTK_BOX (vbox_toolbar), frame, FALSE, FALSE, 12);
		gtk_widget_set_sensitive (frame, xpad_settings_get_has_toolbar (xpad_settings ()));
		
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
			
			gtk_tool_item_set_use_drag_window (GTK_TOOL_ITEM (widget), TRUE);
			gtk_drag_source_set (widget,
				GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_MOVE);
			
			if (toolbar_is_button (widget))
				gtk_drag_source_set_icon_stock (widget, gtk_tool_button_get_stock_id (GTK_TOOL_BUTTON (widget)));
			
			g_signal_connect (widget, "drag-data-get", 
				G_CALLBACK (data_get), NULL);
		}
		
		g_list_free (inxt);
		
		/* build list of all toolbar buttons not in xt */
		for (i = 0; i < num_buttons; i++)
		{
			const GSList *tmp, *start;
			const toolbar_button *tb;
			GtkToolItem *b;
			
			tmp = start = xpad_settings_get_toolbar_buttons (xpad_settings ());
			for (; tmp; tmp = tmp->next)
			{
				tb = (const toolbar_button *) tmp->data;
				
				if (!g_ascii_strcasecmp (tb->name, buttons[i].name))
					break;
			}
			
			if (tmp)	/* we found it, so we don't add it to our list of unused buttons */
				continue;
			
			tb = &buttons[i];
			
			b = toolbar_button_new (tb, tt);
			
			gtk_box_pack_start (GTK_BOX (buttonbox), GTK_WIDGET (b), FALSE,
				FALSE, 0);
			
			gtk_tool_item_set_use_drag_window (b, TRUE);
			gtk_drag_source_set (GTK_WIDGET (b),
				GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_MOVE);
			
			gtk_drag_source_set_icon_stock (GTK_WIDGET (b), tb->stock);
			g_signal_connect (GTK_WIDGET (b), "drag-data-get", 
				G_CALLBACK (data_get), NULL);
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
	#endif
	
	/* misc. setup */
	{
		GtkWidget *checkbutton_scrollbars = gtk_check_button_new_with_mnemonic (_("Allow _scrollbars"));
		GtkWidget *checkbutton_sticky = gtk_check_button_new_with_mnemonic (_("Pads start s_ticky"));
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_decorations), xpad_settings_get_has_decorations (xpad_settings ()));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_confirm_destroy), xpad_settings_get_confirm_destroy (xpad_settings ()));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_edit_lock), xpad_settings_get_edit_lock (xpad_settings ()));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_scrollbars), xpad_settings_get_has_scrollbar (xpad_settings ()));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_sticky), xpad_settings_get_sticky (xpad_settings ()));
		gtk_box_pack_start (GTK_BOX (hbox_misc), vbox_misc, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_edit_lock, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_sticky, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_confirm_destroy, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_scrollbars, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox_misc), checkbutton_decorations, FALSE, FALSE, 0);
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
	
		g_signal_connect (GTK_OBJECT (checkbutton_confirm_destroy), "toggled", G_CALLBACK (change_confirm_destroy), (gpointer) window);
		g_signal_connect (GTK_OBJECT (checkbutton_decorations), "toggled", G_CALLBACK (change_decorations), NULL);
		g_signal_connect (GTK_OBJECT (checkbutton_edit_lock), "toggled", G_CALLBACK (change_edit_lock), (gpointer) window);
		g_signal_connect (GTK_OBJECT (checkbutton_scrollbars), "toggled", G_CALLBACK (change_scrollbars), (gpointer) window);
		g_signal_connect (GTK_OBJECT (checkbutton_sticky), "toggled", G_CALLBACK (change_sticky_on_start), (gpointer) window);
	}
	
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
/*	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);*/
	gtk_widget_show_all (window);
	
	return window;
}

void preferences_open (pad_node *pad)
{
	if (pref_window == NULL)
	{
		pref_window = preferences_create ();
	}
	else
	{
		gtk_window_present (GTK_WINDOW (pref_window));
	}
	
	if (pad) {
		gtk_window_set_transient_for (GTK_WINDOW (pref_window), GTK_WINDOW (pad->window));
		gtk_window_set_destroy_with_parent (GTK_WINDOW (pref_window), FALSE);
	}
}
