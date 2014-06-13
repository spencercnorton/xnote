/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2013-2014 Arthur Borsboom

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef XPAD_PERIODIC_H
#define XPAD_PERIODIC_H

#include <glib.h>

typedef void (*XpadPeriodicFunc)(void *);

/* Callback function codes: save-content, save-info */

/************************
	Xpad_periodic_init():	initializes this module
	Xpad_periodic_close():	frees resources of this module
************************/
gboolean	Xpad_periodic_init(void);
void		Xpad_periodic_close(void);

/************************
	Xpad_periodic_set_callback():
	Sets up a callback function for a signal name.
	The signal names are "save-info" and "save-content"
************************/
gboolean	Xpad_periodic_set_callback (const char *, XpadPeriodicFunc);

/************************
	Xpad_periodic_save_content_delayed:
	Xpad_periodic_save_info_delayed:
	These functions prepare either the pad's content
	or info to be saved later.
************************/
void	Xpad_periodic_save_content_delayed (void * xpad_pad);
void	Xpad_periodic_save_info_delayed (void * xpad_pad);
void	Xpad_periodic_signal (const char * cbname, void * xpad_pad);

void	Xpad_periodic_test (); /* aborts program */
void	Xpad_periodic_error_exit (const char *, ...); /* ditto */

#endif /* XPAD_PERIODIC_H */

/************************
************************/
/* vim: set ts=4 sw=4 :vim */
