/*
 * xpad-debug.h
 * This file is part of xpad
 *
 * Copyright (C) 2014 - 2015 Sagar Ghuge
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __XPAD_DEBUG_H__
#define __XPAD_DEBUG_H__

#include <glib.h>

/**
 * XpadDebugSection:
 *
 * Enumeration of debug sections.
 *
 * Debugging output for a section is enabled by setting an environment variable
 * of the same name. For example, setting the <code>XPAD_DEBUG_APP</code>
 * environment variable enables all debugging output for the %XPAD_DEBUG_APP
 * section. Setting the special environment variable <code>XPAD_DEBUG</code>
 * enables output for all sections.
 */
typedef enum {
	XPAD_NO_DEBUG       = 0,
	XPAD_DEBUG_VIEW     = 1 << 0,
	XPAD_DEBUG_SEARCH   = 1 << 1,
	XPAD_DEBUG_PREFS    = 1 << 2,
	XPAD_DEBUG_PAD      = 1 << 3,
	XPAD_DEBUG_APP      = 1 << 4,
	XPAD_DEBUG_WINDOW   = 1 << 5,
} XpadDebugSection;

#define	DEBUG_VIEW	XPAD_DEBUG_VIEW,    __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_SEARCH	XPAD_DEBUG_SEARCH,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PREFS	XPAD_DEBUG_PREFS,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PAD	XPAD_DEBUG_PAD,     __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_APP	XPAD_DEBUG_APP,     __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_WINDOW	XPAD_DEBUG_WINDOW,  __FILE__, __LINE__, G_STRFUNC

void xpad_debug_init (void);

void xpad_debug (XpadDebugSection  section,
		  const gchar       *file,
		  gint               line,
		  const gchar       *function);

void xpad_debug_message (XpadDebugSection  section,
			  const gchar       *file,
			  gint               line,
			  const gchar       *function,
			  const gchar       *format, ...) G_GNUC_PRINTF(5, 6);

#endif /* __XPAD_DEBUG_H__ */
