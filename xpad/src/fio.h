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

#ifndef _FIO_H_
#define _FIO_H_

#include <gtk/gtk.h>
#include "main.h"
#include "pad.h"
#include "pref.h"

int fio_load_pads (void);
gchar *fio_get_file (const gchar *name);
gboolean fio_set_file (const gchar *name, const gchar *value);

gint fio_load_default_settings (void);
void fio_save_default_settings (void);

void fio_open_pad_files (pad_node *pad, gboolean create);
void fio_remove_pad_files (pad_node *pad);

void fio_remove_file (const gchar *filename);

void fio_save_pad_info (pad_node *pad);
void fio_save_pad_content (pad_node *pad);
void fio_save_pad (pad_node *pad);
void fio_save_pads (void);


#endif /* _FIO_H_ */










