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

#ifndef _FIO_H_
#define _FIO_H_

#include <gtk/gtk.h>
#include "pad.h"

/* helper func to get textbox from window */
GtkTextView *get_text (GtkWindow *window);

void save_pad (pad_node *pad);
void save_pads ();
void commit_pads ();
void load_pads ();
void remove_pad_files (pad_node *pad);

pad_style get_default_style ();
void set_default_style (pad_style *style);

#endif /* _FIO_H_ */







