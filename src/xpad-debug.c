/*
 * xpad-debug.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include "xpad-debug.h"

#define ENABLE_PROFILING

#ifdef ENABLE_PROFILING
static GTimer *timer = NULL;
static gdouble last = 0.0;
#endif

static XpadDebugSection debug = XPAD_NO_DEBUG;

#define DEBUG_IS_ENABLED(section_rval) (debug & (section_rval))

/**
 * xpad_debug_init:
 *
 * Initializes the debugging subsystem of Xpad.
 *
 * The function checks for the existence of certain environment variables to
 * determine whether to enable output for a debug section. To enable output
 * for a specific debug section, set an environment variable of the same name;
 * e.g. to enable output for the %XPAD_DEBUG_APP section, set a
 * <code>XPAD_DEBUG_APP</code> environment variable. To enable output
 * for all debug sections, set the <code>XPAD_DEBUG</code> environment
 * variable.
 *
 * This function must be called before any of the other debug functions are
 * called. It must only be called once.
 */
void
xpad_debug_init (void)
{
	if (g_getenv ("XPAD_DEBUG") != NULL)
	{
		/* enable all debugging */
		debug = ~XPAD_NO_DEBUG;
		goto out;
	}

	if (g_getenv ("XPAD_DEBUG_VIEW") != NULL)
		debug = debug | XPAD_DEBUG_VIEW;
	if (g_getenv ("XPAD_DEBUG_SEARCH") != NULL)
		debug = debug | XPAD_DEBUG_SEARCH;
	if (g_getenv ("XPAD_DEBUG_PREFS") != NULL)
		debug = debug | XPAD_DEBUG_PREFS;
	if (g_getenv ("XPAD_DEBUG_PAD") != NULL)
		debug = debug | XPAD_DEBUG_PAD;
	if (g_getenv ("XPAD_DEBUG_APP") != NULL)
		debug = debug | XPAD_DEBUG_APP;
	if (g_getenv ("XPAD_DEBUG_WINDOW") != NULL)
		debug = debug | XPAD_DEBUG_WINDOW;
out:

#ifdef ENABLE_PROFILING
	if (debug != XPAD_NO_DEBUG)
		timer = g_timer_new ();
#endif
	return;
}

/**
 * xpad_debug:
 * @section: Debug section.
 * @file: Name of the source file containing the call to xpad_debug().
 * @line: Line number within the file named by @file of the call to xpad_debug().
 * @function: Name of the function that is calling xpad_debug().
 *
 * If output for debug section @section is enabled, then logs the trace
 * information @file, @line, and @function.
 */
void xpad_debug (XpadDebugSection  section,
		  const gchar       *file,
		  gint               line,
		  const gchar       *function)
{
	if (G_UNLIKELY (DEBUG_IS_ENABLED (section)))
	{
#ifdef ENABLE_PROFILING
		gdouble seconds;

		g_return_if_fail (timer != NULL);

		seconds = g_timer_elapsed (timer, NULL);
		g_print ("[%f (%f)] %s:%d (%s)\n",
			 seconds, seconds - last, file, line, function);
		last = seconds;
#else
		g_print ("%s:%d (%s)\n", file, line, function);
#endif

		fflush (stdout);
	}
}

/**
 * xpad_debug_message:
 * @section: Debug section.
 * @file: Name of the source file containing the call to xpad_debug_message().
 * @line: Line number within the file named by @file of the call to xpad_debug_message().
 * @function: Name of the function that is calling xpad_debug_message().
 * @format: A g_vprintf() format string.
 * @...: The format string arguments.
 *
 * If output for debug section @section is enabled, then logs the trace
 * information @file, @line, and @function along with the message obtained by
 * formatting @format with the given format string arguments.
 */
void
xpad_debug_message (XpadDebugSection  section,
		     const gchar       *file,
		     gint               line,
		     const gchar       *function,
		     const gchar       *format, ...)
{
	if (G_UNLIKELY (DEBUG_IS_ENABLED (section)))
	{
		va_list args;
		gchar *msg;

#ifdef ENABLE_PROFILING
		gdouble seconds;

		g_return_if_fail (timer != NULL);

		seconds = g_timer_elapsed (timer, NULL);
#endif

		g_return_if_fail (format != NULL);

		va_start (args, format);
		msg = g_strdup_vprintf (format, args);
		va_end (args);

#ifdef ENABLE_PROFILING
		g_print ("[%f (%f)] %s:%d (%s) %s\n",
			 seconds, seconds - last, file, line, function, msg);
		last = seconds;
#else
		g_print ("%s:%d (%s) %s\n", file, line, function, msg);
#endif

		fflush (stdout);

		g_free (msg);
	}
}
