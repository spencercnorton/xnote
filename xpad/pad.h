/*

Copyright (c) 2001 Michael Terry

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

#include <gtk/gtk.h>
#include "main.h"

typedef struct pad_node_def pad_node;
typedef struct pad_info_def pad_info;
typedef struct pad_style_def pad_style;

struct pad_node_def
{
        pad_node *next;
	gchar fontname[1024];
	gboolean hidden;
	unsigned char num;
        GtkWindow *window;
};

struct pad_style_def
{
	GdkColor back;
	GdkColor text;
	gchar fontname[1024];
};

struct pad_info_def
{
        gint x;
        gint y;
        gint width;
        gint height;
	pad_style style;
        gchar content[MAX_FILE_SIZE];
};

extern pad_info DEFAULT_INFO;
extern pad_node *first_pad;
extern pad_node *last_pad;
extern pad_info current_info;

pad_node *create_pad_with_info (pad_info *info);
pad_node *create_pad ();
void cleanup ();

#endif /* _PAD_H_ */











