#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <gtk/gtk.h>
#include "pad.h"
#include "defines.h"

void toolbar_show (pad_node *pad);

int toolbar_hide (gpointer data);

void toolbar_start_timeout (pad_node *pad);

void toolbar_end_timeout (pad_node *pad);

void toolbar_add_to_pad (pad_node *pad);

void toolbar_update (pad_node *pad);

#endif /* TOOLBAR_H */
