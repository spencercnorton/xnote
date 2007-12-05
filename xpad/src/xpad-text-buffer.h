/**
 * Copyright (c) 2004-2007 Michael Terry
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __XPAD_TEXT_BUFFER_H__
#define __XPAD_TEXT_BUFFER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPAD_TYPE_TEXT_BUFFER          (xpad_text_buffer_get_type ())
#define XPAD_TEXT_BUFFER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_TEXT_BUFFER, XpadTextBuffer))
#define XPAD_TEXT_BUFFER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_TEXT_BUFFER, XpadTextBufferClass))
#define XPAD_IS_TEXT_BUFFER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_TEXT_BUFFER))
#define XPAD_IS_TEXT_BUFFER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_TEXT_BUFFER))
#define XPAD_TEXT_BUFFER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_TEXT_BUFFER, XpadTextBufferClass))

typedef struct XpadTextBufferClass XpadTextBufferClass;
typedef struct XpadTextBuffer XpadTextBuffer;

struct XpadTextBuffer
{
	GtkTextBuffer parent;
};

struct XpadTextBufferClass
{
	GtkTextBufferClass parent_class;
};

GType xpad_text_buffer_get_type (void);

GtkTextBuffer *xpad_text_buffer_new (void);

void xpad_text_buffer_set_text_with_tags (XpadTextBuffer *buffer, const gchar *text);
gchar *xpad_text_buffer_get_text_with_tags (XpadTextBuffer *buffer);

G_END_DECLS

#endif /* __XPAD_TEXT_BUFFER_H__ */
