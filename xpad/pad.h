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

typedef struct pad_node_def pad_node;
typedef struct pad_info_def pad_info;
typedef struct pad_style_def pad_style;

/* holds all the internal data we need to manipulate pads */
struct pad_node_def
{
	pad_node *next;
	//FILE *file;
	gint x, y, width, height;
	gchar *infoname;
	gchar *contentname;
	
	GtkWindow *window;
	GtkWidget *eventbox;
	GtkWidget *eventbox_outer;
	GtkWidget *scrollbar;
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
};

/* used to keep all the persistant data we need for one pad */
struct pad_info_def
{
	gint x;
	gint y;
	gint width;
	gint height;
	pad_style style;
	gchar *infoname;
	gchar *contentname;
};

extern pad_node *first_pad;
extern pad_node *last_pad;

void help_dialog (void);
pad_node *pad_new_with_info (pad_info *info);
pad_node *pad_new (void);
GtkTextView *get_text (GtkWindow *window);
void cleanup (void);
pad_style *pad_get_style (pad_node *pad);
void pads_set_decorations (gboolean decor, GtkWidget *caller);
void pads_set_editable (gboolean editable);

#endif /* _PAD_H_ */















