/*

Copyright (c) 2001-2004 Michael Terry

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

#ifndef __XPAD_TOOLBAR_H__
#define __XPAD_TOOLBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPAD_TYPE_TOOLBAR          (xpad_toolbar_get_type ())
#define XPAD_TOOLBAR(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_TOOLBAR, XpadToolbar))
#define XPAD_TOOLBAR_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_TOOLBAR, XpadToolbarClass))
#define XPAD_IS_TOOLBAR(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_TOOLBAR))
#define XPAD_IS_TOOLBAR_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_TOOLBAR))
#define XPAD_TOOLBAR_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_TOOLBAR, XpadToolbarClass))

typedef struct XpadToolbarClass XpadToolbarClass;
typedef struct XpadToolbarPrivate XpadToolbarPrivate;
typedef struct XpadToolbar XpadToolbar;

struct XpadToolbar
{
	GtkToolbar parent;
	
	/* private */
	XpadToolbarPrivate *priv;
};

struct XpadToolbarClass
{
	GtkToolbarClass parent_class;
	
	void (*activate_clear) (XpadToolbar *toolbar);
	void (*activate_close) (XpadToolbar *toolbar);
	void (*activate_delete) (XpadToolbar *toolbar);
	void (*activate_new) (XpadToolbar *toolbar);
	void (*activate_preferences) (XpadToolbar *toolbar);
	void (*activate_properties) (XpadToolbar *toolbar);
	void (*activate_quit) (XpadToolbar *toolbar);
	void (*activate_sticky) (XpadToolbar *toolbar);
};

GType xpad_toolbar_get_type (void);

GtkWidget *xpad_toolbar_new (void);

gboolean xpad_toolbar_get_sticky_active (XpadToolbar *toolbar);
void xpad_toolbar_set_sticky_active (XpadToolbar *toolbar, gboolean active);

#if 0
void toolbar_show (pad_node *pad);
void toolbar_hide (pad_node *pad);

void toolbar_start_timeout (pad_node *pad);
void toolbar_end_timeout (pad_node *pad);

xpad_toolbar *toolbar_new (void);
void toolbar_update (xpad_toolbar *xt);

GList *toolbar_get_buttons (xpad_toolbar *xt);
GList *toolbar_get_children (xpad_toolbar *xt);
GtkWidget *toolbar_get_container (xpad_toolbar *xt);

gboolean toolbar_is_button (GtkWidget *widget);
GtkToolItem *toolbar_button_new (const toolbar_button *tb, GtkTooltips *tooltips);
GtkToolItem *toolbar_separator_new (void);
gboolean toolbar_is_visible (xpad_toolbar *xt);
#endif

G_END_DECLS

#endif /* __XPAD_TOOLBAR_H__ */
