/*

Copyright (c) 2004 Michael Terry

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

#include "../config.h"
#include "xpad-dashboard-frontend.h"
#include "dashboard-frontend.h"
#include "pad.h"

static void
xpad_dashboard_frontend_send_text_clue_packet (pad_node *pad,
                                               gboolean additive,
                                               gboolean do_textblock,
                                               gboolean do_word_at_point,
                                               gboolean do_sentence_at_point,
                                               gboolean do_line_at_point)
{
	GtkTextBuffer *buffer;
	GtkTextMark *cursor_mark;
	GtkTextIter cursor_iter;
	gchar *context;
	gchar *cluepacket;
	GList *clues = NULL;
	
	context = g_strdup_printf ("pad-%i", pad->num);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	cursor_mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &cursor_iter, cursor_mark);
	
	if (do_textblock) {
		GtkTextIter start, end;
		gchar *clue_text;
		
		gtk_text_buffer_get_bounds (buffer, &start, &end);
		clue_text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
		clues = g_list_prepend (clues, dashboard_build_clue (clue_text, "textblock", 4));
		g_free (clue_text);
	}
	
	if (do_word_at_point && 
	    (gtk_text_iter_starts_word (&cursor_iter) ||
	     gtk_text_iter_ends_word (&cursor_iter) ||
	     gtk_text_iter_inside_word (&cursor_iter))) {
		GtkTextIter start = cursor_iter, end = cursor_iter;
		gchar *clue_text;
		
		if (!gtk_text_iter_starts_word (&start))
			gtk_text_iter_backward_word_start (&start);
		if (!gtk_text_iter_ends_word (&end))
			gtk_text_iter_forward_word_end (&end);
		clue_text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
		clues = g_list_prepend (clues, dashboard_build_clue (clue_text, "word_at_point", 10));
		g_free (clue_text);
	}
	
	if (do_sentence_at_point && 
	    (gtk_text_iter_starts_sentence (&cursor_iter) ||
	     gtk_text_iter_ends_sentence (&cursor_iter) ||
	     gtk_text_iter_inside_sentence (&cursor_iter))) {
		GtkTextIter start = cursor_iter, end = cursor_iter;
		gchar *clue_text;
		
		if (!gtk_text_iter_starts_sentence (&start))
			gtk_text_iter_backward_sentence_start (&start);
		if (!gtk_text_iter_ends_sentence (&end))
			gtk_text_iter_forward_sentence_end (&end);
		clue_text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
		clues = g_list_prepend (clues, dashboard_build_clue (clue_text, "sentence_at_point", 8));
		g_free (clue_text);
	}
	
	if (do_line_at_point) {
		GtkTextIter start, end = cursor_iter;
		gchar *clue_text;
		
		gtk_text_buffer_get_iter_at_line (buffer, &start, gtk_text_iter_get_line (&cursor_iter));
		if (!gtk_text_iter_ends_line (&end))
			gtk_text_iter_forward_to_line_end (&end);
		clue_text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
		clues = g_list_prepend (clues, dashboard_build_clue (clue_text, "line_at_point", 8));
		g_free (clue_text);
	}
	
	cluepacket = dashboard_build_cluepacket_from_cluelist (
		PACKAGE,
		TRUE,
		context,
		additive,
		clues);
	
	dashboard_send_raw_cluepacket (cluepacket);
	
	g_free (cluepacket);
	g_free (context);
	g_list_foreach (clues, (GFunc) g_free, NULL);
	g_list_free (clues);
}

static void
xpad_dashboard_frontend_focus_in_cb (GtkWidget *widget, GdkEventFocus *event, pad_node *pad)
{
	xpad_dashboard_frontend_send_text_clue_packet (pad, FALSE, TRUE, FALSE, FALSE, FALSE);
}

static void
xpad_dashboard_frontend_end_user_action_cb (GtkTextBuffer *buffer, pad_node *pad)
{
	xpad_dashboard_frontend_send_text_clue_packet (pad, TRUE, FALSE, TRUE, TRUE, TRUE);
}

void
xpad_dashboard_frontend_init_for_pad (pad_node *pad)
{
	g_return_if_fail (pad != NULL);
	g_return_if_fail (pad->window != NULL);
	
	g_signal_connect (
		pad->window,
		"focus-in-event",
		G_CALLBACK (xpad_dashboard_frontend_focus_in_cb),
		pad);
	
	g_signal_connect (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview)),
		"end-user-action",
		G_CALLBACK (xpad_dashboard_frontend_end_user_action_cb),
		pad);
}
