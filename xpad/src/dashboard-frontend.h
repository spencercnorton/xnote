/*

Copyright (c) 2004 Michael Terry

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

#ifndef __DASHBOARD_FRONTEND_H__
#define __DASHBOARD_FRONTEND_H__

#include <glib.h>

void dashboard_send_raw_cluepacket (const char *rawcluepacket);
void dashboard_send_raw_cluepacket_sync (const char *rawcluepacket);
char *dashboard_build_clue (const char *text, const char *type, int relevance);
char *dashboard_build_cluepacket_from_cluelist (const char *frontend, gboolean focused, const char *context, gboolean additive, GList *clues);
char *dashboard_build_cluepacket_v (const char *frontend, gboolean focused, const char *context, gboolean additive, va_list args);
char *dashboard_build_cluepacket (const char *frontend, gboolean focused, const char *context, gboolean additive, ...);
char *dashboard_build_cluepacket_then_free_clues (const char *frontend, gboolean focused, const char *context, gboolean additive, ...);

#endif /* ! __DASHBOARD_FRONTEND_H__ */
