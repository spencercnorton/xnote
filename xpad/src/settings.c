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
#include "settings.h"
#include "fio.h"
#include "pad.h"

struct settings {
	gint width;
	gint height;
	gint decorations;
	gint confirm_destroy;
	gint edit_lock;
	gint wm_close;
	gint sticky_on_start;
	pad_style style;
	gint toolbar;
	gint auto_hide_toolbar;
	gint scrollbar;
	GSList *toolbar_buttons;
} current_settings;

static void xpad_settings_load_defaults (void);
static void xpad_settings_load_from_file (void);
static void xpad_settings_save_to_file (void);


void xpad_settings_init (void)
{
	xpad_settings_load_defaults ();
	xpad_settings_load_from_file ();
}

void xpad_settings_shutdown (void)
{
	g_slist_free (current_settings.toolbar_buttons);
	pad_style_free (&current_settings.style);
}

void xpad_settings_set_default_width (gint width)
{
	current_settings.width = width;
	
	xpad_settings_save_to_file ();
}

gint xpad_settings_get_default_width (void)
{
	return current_settings.width;
}

void xpad_settings_set_default_height (gint height)
{
	current_settings.height = height;
	
	xpad_settings_save_to_file ();
}

gint xpad_settings_get_default_height (void)
{
	return current_settings.height;
}

void xpad_settings_set_has_decorations (gboolean decorations, GtkWidget *caller)
{
	current_settings.decorations = decorations;
	
	PAD_ITERATE_START
	pad_set_decorations (PAD, decorations);
	PAD_ITERATE_END
	
	gtk_window_present (GTK_WINDOW (caller));
	
	xpad_settings_save_to_file ();
}

gboolean xpad_settings_get_has_decorations (void)
{
	return current_settings.decorations;
}

void xpad_settings_set_confirm_destroy (gboolean confirm)
{
	current_settings.confirm_destroy = confirm;
	
	xpad_settings_save_to_file ();
}

gboolean xpad_settings_get_confirm_destroy (void)
{
	return current_settings.confirm_destroy;
}

void xpad_settings_set_edit_lock (gboolean lock)
{
	current_settings.edit_lock = lock;
	
	pads_set_editable (lock ? FALSE : TRUE);
	
	xpad_settings_save_to_file ();
}

gboolean xpad_settings_get_edit_lock (void)
{
	return current_settings.edit_lock;
}

void xpad_settings_set_wm_close (enum XPAD_WM_CLOSE_ACTION action)
{
	current_settings.wm_close = action;
	
	xpad_settings_save_to_file ();
}

enum XPAD_WM_CLOSE_ACTION xpad_settings_get_wm_close (void)
{
	return current_settings.wm_close;
}

void xpad_settings_set_has_toolbar (gboolean toolbar)
{
	current_settings.toolbar = toolbar;
	
	pads_set_toolbar (toolbar);
	
	xpad_settings_save_to_file ();
}

gboolean xpad_settings_get_has_toolbar (void)
{
	return current_settings.toolbar;
}

void xpad_settings_set_sticky_on_start (gboolean sticky)
{
	current_settings.sticky_on_start = sticky;
	
	xpad_settings_save_to_file ();
}

gboolean xpad_settings_get_sticky_on_start (void)
{
	return current_settings.sticky_on_start;
}

void xpad_settings_set_auto_hide_toolbar (gboolean hide)
{
	current_settings.auto_hide_toolbar = hide;
	
	pads_set_auto_hide_toolbar (hide);
	
	xpad_settings_save_to_file ();
}

gboolean xpad_settings_get_auto_hide_toolbar (void)
{
	return current_settings.auto_hide_toolbar;
}

void xpad_settings_set_has_scrollbar (gboolean scrollbar)
{
	current_settings.scrollbar = scrollbar;
	
	PAD_ITERATE_START
	pad_set_scrollbars (PAD, scrollbar);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

gboolean xpad_settings_get_has_scrollbar (void)
{
	return current_settings.scrollbar;
}

void xpad_settings_add_toolbar_button (const gchar *button)
{
	g_slist_append (current_settings.toolbar_buttons,
		(toolbar_button *) get_toolbar_button_by_name (button));
	
	PAD_ITERATE_START
	pad_toolbar_update (PAD);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

void xpad_settings_remove_toolbar_button (const gchar *button)
{
	g_slist_remove (current_settings.toolbar_buttons,
		get_toolbar_button_by_name (button));
	
	PAD_ITERATE_START
	pad_toolbar_update (PAD);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

GSList *xpad_settings_get_toolbar_buttons (void)
{
	return g_slist_copy (current_settings.toolbar_buttons);
}

pad_style xpad_settings_get_style (void)
{
	pad_style p;
	
	pad_style_copy (&p, &current_settings.style);
	
	return p;
}


void xpad_settings_style_set_back_color (GdkColor *back)
{
	if (back)
	{
		current_settings.style.use_back = TRUE;
		current_settings.style.back = *back;
	}
	else
	{
		current_settings.style.use_back = FALSE;
	}
	
	PAD_ITERATE_START
	if (!PAD->locked) pad_set_back_color (PAD, back);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

GdkColor xpad_settings_style_get_back_color (void)
{
	return current_settings.style.back;
}

void xpad_settings_style_set_text_color (GdkColor *text)
{
	if (text)
	{
		current_settings.style.use_text = TRUE;
		current_settings.style.text = *text;
	}
	else
	{
		current_settings.style.use_text = FALSE;
	}
	
	PAD_ITERATE_START
	if (!PAD->locked) pad_set_text_color (PAD, text);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

GdkColor xpad_settings_style_get_text_color (void)
{
	return current_settings.style.text;
}

void xpad_settings_style_set_border_color (GdkColor *border)
{
	current_settings.style.border = *border;
	
	PAD_ITERATE_START
	if (!PAD->locked) pad_set_border_color (PAD, border);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

GdkColor xpad_settings_style_get_border_color (void)
{
	return current_settings.style.border;
}

gboolean xpad_settings_style_get_system_back (void)
{
	return current_settings.style.use_back ? FALSE : TRUE;
}

gboolean xpad_settings_style_get_system_text (void)
{
	return current_settings.style.use_text ? FALSE : TRUE;
}


void xpad_settings_style_set_border_width (gint width)
{
	current_settings.style.border_width = width;
	
	PAD_ITERATE_START
	if (!PAD->locked) pad_set_border_width (PAD, width);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

gint xpad_settings_style_get_border_width (void)
{
	return current_settings.style.border_width;
}

void xpad_settings_style_set_padding (gint padding)
{
	current_settings.style.padding = padding;
	
	PAD_ITERATE_START
	if (!PAD->locked) pad_set_padding (PAD, padding);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

gint xpad_settings_style_get_padding (void)
{
	return current_settings.style.padding;
}

void xpad_settings_style_set_fontname (const gchar *fontname)
{
	g_free (current_settings.style.fontname);
	current_settings.style.fontname = g_strdup (fontname);
	
	PAD_ITERATE_START
	if (!PAD->locked) pad_set_fontname (PAD, fontname);
	PAD_ITERATE_END
	
	xpad_settings_save_to_file ();
}

const gchar *xpad_settings_style_get_fontname (void)
{
	return current_settings.style.fontname;
}

static void xpad_settings_load_defaults (void)
{
	current_settings.width = 200;
	current_settings.height = 200;
	current_settings.decorations = 0;
	current_settings.confirm_destroy = 1;
	current_settings.sticky_on_start = 0;
	current_settings.edit_lock = 0;
	current_settings.wm_close = XPAD_WM_CLOSE_PAD;
	current_settings.style.back.pixel = 0;
	current_settings.style.back.red = 0xe000;
	current_settings.style.back.green = 0xe000;
	current_settings.style.back.blue = 0x5600;
	current_settings.style.text.pixel = 0;
	current_settings.style.text.red = 0;
	current_settings.style.text.green = 0;
	current_settings.style.text.blue = 0;
	current_settings.style.border.pixel = 0;
	current_settings.style.border.red = 0;
	current_settings.style.border.green = 0;
	current_settings.style.border.blue = 0;
	current_settings.style.use_back = 1;
	current_settings.style.use_text = 1;
	current_settings.style.border_width = 0;
	current_settings.style.padding = 5;
	current_settings.style.fontname = NULL;
	current_settings.toolbar = 1;
	current_settings.auto_hide_toolbar = 1;
	current_settings.scrollbar = 1;
	
	current_settings.toolbar_buttons = NULL;
	current_settings.toolbar_buttons = 
		g_slist_append (current_settings.toolbar_buttons,
		(toolbar_button *) get_toolbar_button_by_name ("New"));
	current_settings.toolbar_buttons = 
		g_slist_append (current_settings.toolbar_buttons,
		(toolbar_button *) get_toolbar_button_by_name ("Delete"));
	current_settings.toolbar_buttons = 
		g_slist_append (current_settings.toolbar_buttons,
		(toolbar_button *) get_toolbar_button_by_name ("Quit"));
}

static void xpad_settings_load_from_file (void)
{
	/**
	 * We need to set up int values for all these to take the value from the file.
	 * These will be assigned back to the appropriate values after load.
	 */
	gint	back_R = current_settings.style.back.red,
		back_G = current_settings.style.back.green,
		back_B = current_settings.style.back.blue,
		
		text_R = current_settings.style.text.red,
		text_G = current_settings.style.text.green,
		text_B = current_settings.style.text.blue,
		
		bord_R = current_settings.style.border.red,
		bord_G = current_settings.style.border.green,
		bord_B = current_settings.style.border.blue;
		
		gchar *buttons = NULL;
	
	if (fio_get_values_from_file (DEFAULTS_FILENAME, 
		"decorations", &current_settings.decorations,
		"height", &current_settings.height,
		"width", &current_settings.width,
		"confirm_destroy", &current_settings.confirm_destroy,
		"edit_lock", &current_settings.edit_lock,
		"wm_close", &current_settings.wm_close,
		"sticky_on_start", &current_settings.sticky_on_start,
		"back_red", &back_R,
		"back_green", &back_G,
		"back_blue", &back_B,
		"use_back", &current_settings.style.use_back,
		"text_red", &text_R,
		"text_green", &text_G,
		"text_blue", &text_B,
		"use_text", &current_settings.style.use_text,
		"border_red", &bord_R,
		"border_green", &bord_G,
		"border_blue", &bord_B,
		"border_width", &current_settings.style.border_width,
		"padding", &current_settings.style.padding,
		"fontname", &current_settings.style.fontname,
		"toolbar", &current_settings.toolbar,
		"auto_hide_toolbar", &current_settings.auto_hide_toolbar,
		"scrollbar", &current_settings.scrollbar,
		"buttons", &buttons,
		NULL ))
		return;
	
	if (current_settings.style.fontname &&
	    strcmp (current_settings.style.fontname, "NULL") == 0)
	{
		g_free (current_settings.style.fontname);
		current_settings.style.fontname = NULL;
	}
	
	current_settings.style.back.red = back_R;
	current_settings.style.back.green = back_G;
	current_settings.style.back.blue = back_B;
	
	current_settings.style.text.red = text_R;
	current_settings.style.text.green = text_G;
	current_settings.style.text.blue = text_B;
	
	current_settings.style.border.red = bord_R;
	current_settings.style.border.green = bord_G;
	current_settings.style.border.blue = bord_B;
	
	if (buttons) /* get rid of defaults, use our own */
	{
		gint i;
		gchar **button_names;
		gchar *dup;
		
		button_names = g_strsplit (buttons, ",", 50);
		
		while (current_settings.toolbar_buttons)
		{
			current_settings.toolbar_buttons = 
				g_slist_delete_link (current_settings.toolbar_buttons,
				current_settings.toolbar_buttons);
		}
		
		for (i = 0; button_names[i]; ++i)
		{
			toolbar_button *tb;
			
			dup = g_strstrip (button_names[i]);
			
			tb = (toolbar_button *) get_toolbar_button_by_name (dup);
			
			if (tb)
				current_settings.toolbar_buttons = 
					g_slist_append (current_settings.toolbar_buttons,
					tb);
		}
		
		g_strfreev  (button_names);
		g_free (buttons);
	}
}


static void xpad_settings_save_to_file (void)
{
	gchar *buf, *oldbuf;
	GSList *tmp;
	
	buf = g_strdup_printf ("wm_close %i\nedit_lock %i\nconfirm_destroy %i\n"
		"decorations %i\nsticky_on_start %i\nauto_hide_toolbar %i\n"
		"width %i\nheight %i\nback_red %d\nback_green %d\nback_blue %d\nuse_back %d\n"
		"text_red %d\ntext_green %d\ntext_blue %d\nuse_text %d\n"
		"border_red %d\nborder_green %d\n"
		"border_blue %d\nborder_width %d\npadding %d\nfontname %s\ntoolbar %d\n"
		"scrollbar %d\nbuttons ",
		current_settings.wm_close, current_settings.edit_lock, current_settings.confirm_destroy,
		current_settings.decorations, current_settings.sticky_on_start,
		current_settings.auto_hide_toolbar, current_settings.width, current_settings.height,
		current_settings.style.back.red, current_settings.style.back.green, current_settings.style.back.blue,
		current_settings.style.use_back,
		current_settings.style.text.red, current_settings.style.text.green, current_settings.style.text.blue,
		current_settings.style.use_text,
		current_settings.style.border.red, current_settings.style.border.green, current_settings.style.border.blue,
		current_settings.style.border_width, current_settings.style.padding,
		current_settings.style.fontname ? current_settings.style.fontname : "NULL", 
		current_settings.toolbar, current_settings.scrollbar);
	
	tmp = current_settings.toolbar_buttons;
	
	while (tmp)
	{
		oldbuf = buf;
		
		if (tmp)
			buf = g_strconcat (buf, ((const toolbar_button *) tmp->data)->name, ", ", NULL);
		else
			buf = g_strconcat (buf, ((const toolbar_button *) tmp->data)->name, "\n", NULL);
		
		g_free (oldbuf);
		tmp = tmp->next;
	}
	
	fio_set_file (DEFAULTS_FILENAME, buf);
	g_free (buf);
}
