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


#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <gtk/gtk.h>
#include "pad.h"

void xpad_settings_init (void);
void xpad_settings_shutdown (void);

void xpad_settings_set_default_width (gint width);
gint xpad_settings_get_default_width (void);

void xpad_settings_set_default_height (gint height);
gint xpad_settings_get_default_height (void);

void xpad_settings_set_has_decorations (gboolean decorations, GtkWidget *caller);
gboolean xpad_settings_get_has_decorations (void);

void xpad_settings_set_confirm_destroy (gboolean confirm);
gboolean xpad_settings_get_confirm_destroy (void);

void xpad_settings_set_edit_lock (gboolean lock);
gboolean xpad_settings_get_edit_lock (void);

enum XPAD_WM_CLOSE_ACTION
{
	XPAD_WM_CLOSE_ALL,
	XPAD_WM_CLOSE_PAD,
	XPAD_WM_DESTROY_PAD
};

void xpad_settings_set_wm_close (enum XPAD_WM_CLOSE_ACTION action);
enum XPAD_WM_CLOSE_ACTION xpad_settings_get_wm_close (void);

void xpad_settings_set_has_toolbar (gboolean toolbar);
gboolean xpad_settings_get_has_toolbar (void);

void xpad_settings_set_auto_hide_toolbar (gboolean hide);
gboolean xpad_settings_get_auto_hide_toolbar (void);

void xpad_settings_set_has_scrollbar (gboolean scrollbar);
gboolean xpad_settings_get_has_scrollbar (void);

void xpad_settings_add_toolbar_button (const gchar *button);
void xpad_settings_remove_toolbar_button (const gchar *button);
GSList *xpad_settings_get_toolbar_buttons (void);

pad_style xpad_settings_get_style (void);

void xpad_settings_style_set_back_color (GdkColor *back);
GdkColor xpad_settings_style_get_back_color (void);

void xpad_settings_style_set_text_color (GdkColor *text);
GdkColor xpad_settings_style_get_text_color (void);

gboolean xpad_settings_style_get_system_back (void);
gboolean xpad_settings_style_get_system_text (void);

void xpad_settings_style_set_border_color (GdkColor *border);
GdkColor xpad_settings_style_get_border_color (void);

void xpad_settings_style_set_border_width (gint width);
gint xpad_settings_style_get_border_width (void);

void xpad_settings_style_set_padding (gint padding);
gint xpad_settings_style_get_padding (void);

void xpad_settings_style_set_fontname (const gchar *fontname);
const gchar *xpad_settings_style_get_fontname (void);


#endif /* _SETTINGS_H_ */
