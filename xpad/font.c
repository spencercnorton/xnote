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


#include "font.h"
#include "fio.h"
#include "pad.h"

void font_diag_close (GtkWidget *widget, pad_node *node)
{
	gtk_widget_destroy (widget);
}

void font_diag_cancel (GtkWidget *button, pad_node *node)
{
	gdk_window_destroy (gtk_widget_get_parent_window (button));
}


void font_diag_ok (GtkWidget *button, pad_node *node)
{
	GtkTextView *textbox = get_text (node->window);
	GtkStyle *style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET(textbox)));
	pad_style pstyle;	

	strcpy(node->fontname, gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG (gtk_widget_get_toplevel (button))));
	
	style->font_desc = pango_font_description_from_string (node->fontname);

	gtk_widget_set_style (GTK_WIDGET(textbox), style);

	gdk_window_destroy (gtk_widget_get_parent_window (button));

	/* set this as new default style */
	pstyle.text = style->text[GTK_STATE_NORMAL];
	pstyle.back = style->base[GTK_STATE_NORMAL];
	strcpy (pstyle.fontname, node->fontname);

	set_default_style (&pstyle);
}


void font_select (pad_node *node)
{
	GtkFontSelectionDialog *font_diag;

	font_diag = GTK_FONT_SELECTION_DIALOG (gtk_font_selection_dialog_new ("choose a font"));

	gtk_signal_connect (GTK_OBJECT (font_diag), "destroy", GTK_SIGNAL_FUNC (font_diag_close), (gpointer) node);
	gtk_signal_connect (GTK_OBJECT (font_diag->cancel_button), "clicked", GTK_SIGNAL_FUNC (font_diag_cancel), (gpointer) node);
	gtk_signal_connect (GTK_OBJECT (font_diag->ok_button), "clicked", GTK_SIGNAL_FUNC (font_diag_ok), (gpointer) node);

	gtk_font_selection_dialog_set_font_name (font_diag, node->fontname);

	gtk_window_set_transient_for (GTK_WINDOW(node->window), GTK_WINDOW(font_diag));

	gtk_widget_show (GTK_WIDGET (font_diag));
}




