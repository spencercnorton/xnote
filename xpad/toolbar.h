#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <gtk/gtk.h>
#include "pad.h"
#include "defines.h"


void toolbar_show (pad_node *pad);

int toolbar_hide (gpointer data);

void toolbar_start_timeout (pad_node *pad);

void toolbar_end_timeout (pad_node *pad);

xpad_toolbar *toolbar_new (void);

GList *toolbar_get_buttons (xpad_toolbar *xt);
GList *toolbar_get_children (xpad_toolbar *xt);

gboolean toolbar_is_button (GtkWidget *widget);

void toolbar_update (xpad_toolbar *xt);

GtkWidget * toolbar_button_new (const toolbar_button *tb);
GtkWidget * toolbar_separator_new (void);

GtkWidget *toolbar_get_container (xpad_toolbar *xt);

#endif /* TOOLBAR_H */
