#include "constants.h"

/* A pleasant light yellow background color, similar to commercial sticky notes, with black text. */
const guint default_pad_width = 300;
const guint default_pad_height = 300;
const gchar *default_font_name = "Sans 9";
const GdkRGBA default_text_color = {0, 0, 0, 1};
const GdkRGBA default_back_color = {1, 0.933334350586, 0.6, 1};

const XpadStickyColor xpad_sticky_colors[] = {
	{ "Canary Yellow", "#FFF9B1" },
	{ "Pink",          "#FFC8DC" },
	{ "Mint Green",    "#C3F0BE" },
	{ "Sky Blue",      "#B5E0F7" },
	{ "Peach",         "#FFD6A0" },
	{ "Lilac",         "#E0CCF5" },
};
const guint xpad_sticky_colors_n = G_N_ELEMENTS (xpad_sticky_colors);

const GdkRGBA xpad_sticky_text_color = { 0.16, 0.15, 0.11, 1.0 };
