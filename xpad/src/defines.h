#ifndef DEFINES_H
#define DEFINES_H

#include "../config.h"

#include "gettext.h"

/**
 * This file is just used to hold all the common hard defines we need.
 */
 
#define _(String) gettext (String)
#define N_(String) gettext_noop (String)


#define DEFAULTS_FILENAME	"default-style"

/* files we use are not large */
#define MAX_FILE_SIZE		1024

#define TITLE_CHARS			10

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
};


#endif

