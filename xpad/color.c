/*

Copyright (c) 2001 Michael Terry

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

#include "color.h"
#include "pad.h"

/* coupling is nothing more than a convenience struct, bundling a widget with 
   its original style. */

typedef struct coupling_def coupling;

struct coupling_def
{
	GtkWidget *widget;
	gboolean is_base;
};

void color_diag_close (GtkColorSelectionDialog *colordiag, coupling *c)
{
	GtkStyle *style = gtk_style_copy (gtk_widget_get_style (c->widget));
	GdkColor color;
	
	gtk_color_selection_get_previous_color (GTK_COLOR_SELECTION (colordiag->colorsel), &color);

	if (c->is_base == TRUE) 
		style->base[GTK_STATE_NORMAL] = color;
	else
		style->text[GTK_STATE_NORMAL] = color;
	
	gtk_widget_set_style (c->widget, style);
	gtk_widget_destroy (GTK_WIDGET(colordiag));
	g_free (c);
}

void color_diag_cancel (GtkWidget *button, coupling *c)
{
	GtkStyle *style = gtk_style_copy (gtk_widget_get_style (c->widget));
	GdkColor color;

	gtk_color_selection_get_previous_color (GTK_COLOR_SELECTION 
		(GTK_COLOR_SELECTION_DIALOG(gtk_widget_get_toplevel(button))->colorsel), &color);

	if (c->is_base == TRUE) 
		style->base[GTK_STATE_NORMAL] = color;
	else
		style->text[GTK_STATE_NORMAL] = color;
	
	gtk_widget_set_style (c->widget, style);
	gdk_window_destroy (gtk_widget_get_parent_window (button));
	g_free (c);
}


void color_diag_ok (GtkWidget *button, coupling *c)
{	
	GtkStyle *style = gtk_style_copy (gtk_widget_get_style (c->widget));
	GdkColor color;
	pad_style pstyle;

	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION 
		(GTK_COLOR_SELECTION_DIALOG(gtk_widget_get_toplevel(button))->colorsel), &color);

	if (c->is_base == TRUE) 
		style->base[GTK_STATE_NORMAL] = color;
	else
		style->text[GTK_STATE_NORMAL] = color;
	
	gtk_widget_set_style (c->widget, style);
	gdk_window_destroy (gtk_widget_get_parent_window (button));
	g_free (c);

	/* set this as new default style */
	pstyle.text = style->text[GTK_STATE_NORMAL];
	pstyle.back = style->base[GTK_STATE_NORMAL];
	strcpy (pstyle.fontname, pango_font_description_to_string (style->font_desc));

	set_default_style (&pstyle);
}

void color_changed (GtkColorSelection *colorsel, coupling *c)
{
	GdkColor color;
	GtkStyle *style;

	style = gtk_style_copy(gtk_widget_get_style (c->widget));

	gtk_color_selection_get_current_color (colorsel, &color);

	if (c->is_base == TRUE) 
		style->base[GTK_STATE_NORMAL] = color;
	else
		style->text[GTK_STATE_NORMAL] = color;
	
	gtk_widget_set_style (c->widget, style);

	g_free (style);
}

void color_select (GtkWidget *widget, gboolean is_base)
{
	GtkColorSelectionDialog *color_diag;
	GdkColor color;
	gchar title[30];
	coupling *c = (coupling *) g_malloc (sizeof (coupling));

	c->widget = GTK_WIDGET (get_text (widget));
	c->is_base = is_base;
	
	if (is_base == TRUE)
		color = (gtk_widget_get_style (c->widget))->base[GTK_STATE_NORMAL];
	else
		color = (gtk_widget_get_style (c->widget))->text[GTK_STATE_NORMAL];
	
	sprintf (title, "choose the %s color", is_base == TRUE ? "background" : "text");
	color_diag = GTK_COLOR_SELECTION_DIALOG(gtk_color_selection_dialog_new (title));

	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_diag->colorsel), &color);

	gtk_signal_connect (GTK_OBJECT (color_diag), "destroy", GTK_SIGNAL_FUNC (color_diag_close), (gpointer) c);
	gtk_signal_connect (GTK_OBJECT (color_diag->cancel_button), "clicked", GTK_SIGNAL_FUNC (color_diag_cancel), (gpointer) c);
	gtk_signal_connect (GTK_OBJECT (color_diag->ok_button), "clicked", GTK_SIGNAL_FUNC (color_diag_ok), (gpointer) c);
	
	/* IF YOU ARE LOOKING FOR SOMETHING TO DO...  FIGURE OUT WHY THIS NEXT STATEMENT HAS NO EFFECT.  THANKS */
	gtk_signal_connect (GTK_OBJECT (color_diag->colorsel), "color-changed", GTK_SIGNAL_FUNC (color_changed), (gpointer) c);
	
	gtk_widget_hide (color_diag->help_button);

	gtk_window_set_transient_for (GTK_WINDOW(widget), GTK_WINDOW(color_diag));

	gtk_widget_show (GTK_WIDGET (color_diag));
}

void back_color_select (GtkWidget *widget)
{
	color_select (widget, TRUE);
}

void text_color_select (GtkWidget *widget)
{
	color_select (widget, FALSE);
}






