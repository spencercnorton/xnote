#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <gdk/gdk.h>

extern const guint default_pad_width;
extern const guint default_pad_height;
extern const gchar *default_font_name;
extern const GdkRGBA default_text_color;
extern const GdkRGBA default_back_color;

/* The classic sticky-note palette, shared by the random-color feature and
   the toolbar color picker. */
typedef struct
{
	const gchar *name;	/* untranslated; pass through _() for display */
	const gchar *hex;
} XpadStickyColor;

extern const XpadStickyColor xpad_sticky_colors[];
extern const guint xpad_sticky_colors_n;

/* Ink color used on all sticky-note backgrounds. */
extern const GdkRGBA xpad_sticky_text_color;

#endif
