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


#ifndef _MAIN_H_
#define _MAIN_H_

#include <gtk/gtk.h>

enum {
	MAX_FILENAME_SIZE=1024,
	MAX_FILE_SIZE=2048
};

extern gchar working_dir[MAX_FILENAME_SIZE];
extern const gchar *VERSION;

#endif /* _MAIN_H_ */

























