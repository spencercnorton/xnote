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

#ifndef _PAD_H_
#define _PAD_H_

#include <stdio.h>
#include <gtk/gtk.h>
#include "defines.h"
#include "help.h"
#include "pref.h"

typedef struct pad_node_def pad_node;
typedef struct pad_info_def pad_info;
typedef struct pad_style_def pad_style;
typedef struct toolbar_button_def toolbar_button;


/* holds all the internal data we need to manipulate pads */
struct pad_node_def
{
	pad_node *next;
	gint x, y, width, height;
	gchar *infoname;
	gchar *contentname;
	
	GtkWindow *window;
	GtkWidget *eventbox;
	GtkWidget *eventbox_outer;
	GtkWidget *scrollbar;
	GtkWidget *box;	/* holds textbox stuff and toolbar */
	
	/* toolbar related variables */
	GtkWidget *toolbar;
	GtkWidget *grip;
	guint toolbar_timeout;
	gint toolbar_height;
};

/* describes the custom styles of a pad */
struct pad_style_def
{
	GdkColor back;
	GdkColor text;
	GdkColor border;
	gint border_width;
	gint padding;
	gchar *fontname;
} current_style;

/* used to keep all the persistant data we need for one pad */
struct pad_info_def
{
	gint x;
	gint y;
	gint width;
	gint height;
	gchar *infoname;
	gchar *contentname;
};

extern pad_node *first_pad;
extern pad_node *last_pad;

pad_node *pad_new_with_info (pad_info *info);
pad_node *pad_new (void);
GtkTextView *get_text (GtkWindow *window);
void cleanup (void);

void pads_set_decorations (gboolean decor, GtkWidget *caller);
void pads_set_editable (gboolean editable);

void pad_close (pad_node *pad);
void pad_destroy (pad_node *pad);
void pad_open_file (pad_node *pad);
void pad_save_as_file (pad_node *pad);
void pad_close_all (void);
gboolean pad_confirm_destroy (pad_node *pad);
void pad_clear (pad_node *pad);

struct toolbar_button_def
{
	const gchar *name;
	const gchar *stock;
	GCallback func;
	const gchar *desc;
};

static const toolbar_button buttons[] =
{
	{"Close", "gtk-close", G_CALLBACK (pad_close), "Close and Save Pad"},
	{"Delete", "gtk-delete", G_CALLBACK (pad_confirm_destroy), "Delete Pad"},
	{"Help", "gtk-help", G_CALLBACK (show_help), "Open Help"},
	{"Preferences", "gtk-preferences", G_CALLBACK (preferences_open), "Open Preferences"},
	{"New", "gtk-new", G_CALLBACK (pad_new), "Open New Pad"},
	{"Open", "gtk-open", G_CALLBACK (pad_open_file), "Open File"},
	{"Save As", "gtk-save-as", G_CALLBACK (pad_save_as_file), "Save Pad As File"},
	{"Quit", "gtk-quit", G_CALLBACK (pad_close_all), "Close and Save All Pads"},
	{"Clear", "gtk-clear", G_CALLBACK (pad_clear), "Clear Pad Contents"}
};


#define NUM_BUTTONS (sizeof (buttons) / sizeof (toolbar_button))

#endif /* _PAD_H_ */















