#ifndef DEFINES_H
#define DEFINES_H

#include <gtk/gtk.h>
#include "../config.h"
#include "gettext.h"

/**
 * This file is just used to hold all the common hard defines we need.
 */

/* Seems that some systems (sun-sparc-solaris2.8 at least), need the following three #defines. 
   These were provided by Alan Mizrahi <alan@cesma.usb.ve>.
*/
#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

#ifndef SUN_LEN
#define SUN_LEN(sunp) ((size_t)((struct sockaddr_un *)0)->sun_path + strlen((sunp)->sun_path))
#endif


/* Some gettext conveniences. */
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

