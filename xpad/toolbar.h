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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <gtk/gtk.h>
#include "pad.h"
#include "defines.h"


void toolbar_show (pad_node *pad);
void toolbar_hide (pad_node *pad);

void toolbar_start_timeout (pad_node *pad);
void toolbar_end_timeout (pad_node *pad);

xpad_toolbar *toolbar_new (void);
void toolbar_update (xpad_toolbar *xt);

GList *toolbar_get_buttons (xpad_toolbar *xt);
GList *toolbar_get_children (xpad_toolbar *xt);
GtkWidget *toolbar_get_container (xpad_toolbar *xt);

gboolean toolbar_is_button (GtkWidget *widget);
GtkWidget *toolbar_button_new (const toolbar_button *tb);
GtkWidget *toolbar_separator_new (void);
gboolean toolbar_is_visible (xpad_toolbar *xt);

#endif /* TOOLBAR_H */
