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


#ifndef _MAIN_H_
#define _MAIN_H_

#include <gtk/gtk.h>
#include "defines.h"
#include "pad.h"

struct settings {
	int width;
	int height;
	int sync_time;
	int decorations;
	int confirm_destroy;
	int edit_lock;
	int wm_close;
	pad_style style;
};

void xpad_exit (void);
int sync_pads (gpointer data);
void reset_sync (void);

extern gchar working_dir[MAX_FILENAME_SIZE];
extern gint verbosity;
extern guint autosave_timeout_id;
extern struct settings current_settings;

#endif /* _MAIN_H_ */
