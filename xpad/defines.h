#ifndef DEFINES_H
#define DEFINES_H

/**
 * This file is just used to hold all the common hard defines we need.
 */

#define DEFAULTS_FILENAME	"default-style"

/* files we use are not large */
#define MAX_FILE_SIZE		1024

#define VERSION				"1.4"

/* now define some structures here so that i don't have odd header dependencies */
typedef struct xpad_toolbar_def xpad_toolbar;
struct xpad_toolbar_def
{
	GtkWidget *bar;
	GtkWidget *grip;
	guint timeout;
	gint height;
};


#endif

