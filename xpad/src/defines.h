#ifndef DEFINES_H
#define DEFINES_H

#include <gtk/gtk.h>
#include "../config.h"
#include <glib/gi18n.h>

/**
 * This file is just used to hold all the common hard defines we need.
 */

#define DEFAULTS_FILENAME	"default-style"

/* files we use are not large */
#define MAX_FILE_SIZE		1024

/* this is for testing the drawing support */
#define DRAWING_ON			0

/* now define some structures here so that i don't have odd header dependencies */
typedef struct xpad_toolbar_def xpad_toolbar;
struct xpad_toolbar_def
{
	GtkWidget *bar;
	GtkWidget *grip;
	guint timeout;
	gint height;
	gboolean visible;
	GtkTooltips *tooltips;
};


#endif

