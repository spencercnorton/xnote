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

#ifndef _PAD_H_
#define _PAD_H_

#include <stdio.h>
#include <gtk/gtk.h>
#include "defines.h"
#include "help.h"

typedef struct pad_node_def pad_node;
typedef struct pad_info_def pad_info;
typedef struct pad_style_def pad_style;
typedef struct toolbar_button_def toolbar_button;


/* describes the custom styles of a pad */
struct pad_style_def
{
	GdkColor back;
	GdkColor text;
	gint use_back;
	gint use_text;
	gchar *fontname;
};

/* holds all the internal data we need to manipulate pads */
struct pad_node_def
{
	pad_node *next;
	
	/* saved values */
	gint x, y, width, height;
	gchar *infoname;
	gchar *contentname;
	gint locked;
	gint sticky;
	pad_style style;
	
	/* generated values */
	gint num;
	gboolean hidden;
	gchar *title;
	
	/* this was added to differentiate between a pad that has been explicitly closed,
	 * and a pad that has been merely hidden to the system tray */
	gboolean closed;

	GtkUIManager *ui_manager;
	GtkActionGroup *popup_notes_actions;
	gint popup_notes_merge_id;
	
	/* main textbox stuff */
	GtkWindow *window;
	GtkWidget *textview;
	GtkWidget *scrollbar;
	GtkWidget *box;	/* holds textbox stuff and toolbar */
	
	/* toolbar stuff */
	xpad_toolbar *toolbar;
	
	/* properties window */
	GtkWidget *properties;
	
#if DRAWING_ON
	/* background image stuff */
	GdkPixmap *background;
	GdkPixmap *visible_back;
	gint last_draw_x, last_draw_y;
#endif
};

/* used to keep all the persistant data we need for one pad */
struct pad_info_def
{
	gint x;
	gint y;
	gint width;
	gint height;
	gint locked;
	gint sticky;
	
	pad_style style;
	
	gchar *infoname;
	gchar *contentname;
};

#define PAD_ITERATE_START \
{\
	pad_node *PAD;\
	for (PAD = first_pad; PAD; PAD = PAD->next)\
	{

#define PAD_ITERATE_END \
	}\
}


extern pad_node *first_pad;

pad_node *pad_new_with_info (pad_info *info);
pad_node *pad_new (void);
void cleanup (void);

void pads_set_editable (gboolean editable);
void pads_set_toolbar (gboolean toolbar);
void pads_set_auto_hide_toolbar (gboolean auto_hide_toolbar);
void pads_show_all (void);
void pads_close_all (void);

void pad_set_back_color (pad_node *pad, GdkColor *c);
void pad_set_text_color (pad_node *pad, GdkColor *c);
void pad_set_border_color (pad_node *pad, GdkColor *c);
void pad_set_padding (pad_node *pad, gint p);
void pad_set_border_width (pad_node *pad, gint w);
void pad_set_fontname (pad_node *pad, const gchar *fontname);
void pad_close (pad_node *pad);
void pad_destroy (pad_node *pad);
void pad_open_file (pad_node *pad);
void pad_save_as_file (pad_node *pad);
void pad_clear (pad_node *pad);
void pad_hide (pad_node *pad);
void pad_toggle_lock (pad_node *pad);
void pad_style_copy (pad_style *dest, pad_style *source);
void pad_style_free (pad_style *dest);
void pad_background_clear (pad_node *pad);
void pad_toggle_sticky (pad_node *pad);
void pad_show_by_num (gint n);
void pad_show (pad_node *pad);
void pad_lock_style (pad_node *pad);
void pad_unlock_style (pad_node *pad);
void pad_set_scrollbars (pad_node *pad, gboolean on);
void pad_set_decorations (pad_node *pad, gboolean decor);
gboolean pad_confirm_destroy (pad_node *pad);


gchar *str_replace_tokens (gchar **string, gchar obj, gchar *replacement);


void pad_remove_toolbar (pad_node *pad);
void pad_add_toolbar (pad_node *pad);
void pad_toolbar_update (pad_node *pad);

struct toolbar_button_def
{
	const gchar *name;
	const gchar *stock;
	guint type;
	GCallback func;
	const gchar *desc;
};

void menuitem_cb (GtkAction *action, gpointer user_data);
extern GtkActionEntry pad_actions[];
extern const gint num_pad_actions;
extern const toolbar_button buttons[];
extern const char num_buttons;

const toolbar_button *get_toolbar_button_by_func (GCallback func);
const toolbar_button *get_toolbar_button_by_name (const gchar *name);

#endif /* _PAD_H_ */
