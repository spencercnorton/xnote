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


#ifndef _MAIN_H_
#define _MAIN_H_

#include <gtk/gtk.h>
#include "defines.h"
#include "pad.h"

struct settings {
	gint width;
	gint height;
	gint sync_time;
	gint decorations;
	gint confirm_destroy;
	gint edit_lock;
	gint wm_close;
	pad_style style;
	gint toolbar;
	gint auto_hide_toolbar;
	gint scrollbar;
	GSList *toolbar_buttons;
};

GtkWidget *xpad_alert_new (GtkWindow *parent, const gchar *stock, const gchar *primary, const gchar *secondary);
void xpad_show_error (GtkWindow *parent, const gchar *primary, const gchar *secondary);

extern gchar *working_dir;
extern gint verbosity;
extern guint autosave_timeout_id;
extern struct settings current_settings;

#endif /* _MAIN_H_ */
