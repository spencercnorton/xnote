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

#include "pad.h"
#include "font.h"
#include "color.h"
#include "fio.h"
#include <stdio.h>

pad_info DEFAULT_INFO = {0, 0, {260, 260, {0, 0xe000, 0xe000, 0x5600}, {0, 0, 0, 0}, "annstone 12"}, ""};
pad_info current_info;
pad_node *first_pad = NULL;
pad_node *last_pad = NULL;
unsigned char pad_num = 0;

void pad_close (pad_node *node);

pad_node *get_pad (GtkWindow *window)
{
	pad_node *current = first_pad;

	while (current != NULL)
	{
		if (current->window == window)
			return current;

		current = current->next;
	}

	return NULL;
}

void free_all_pads ()
{
	pad_node *current = first_pad;

	while (first_pad != NULL)
	{
		current = first_pad;
		first_pad = first_pad->next;

		gtk_widget_destroy (GTK_WIDGET(current->window));

		close_pad_files (current);
		g_free (current);
	}
}

void cleanup ()
{
	free_all_pads();
}

void quit_if_no_pads ()
{
	gboolean pad_exists = FALSE;
	pad_node *current = first_pad;
	
	if (current == NULL)
		gtk_main_quit();

	while (current != NULL)
	{
		if (current->hidden == FALSE)
		{
			pad_exists = TRUE;
			break;
		}

		current = current->next;
	}

	if (pad_exists == FALSE)
	{
		cleanup ();
		gtk_main_quit();
	}
}

/* destroy one pad */
void pad_destroy (pad_node *node)
{
	GtkWindow *window = node->window;
	pad_node *current = first_pad;
	pad_node *old_node = first_pad;
	
	if (window == NULL || first_pad == NULL) 
		return;

	/* first, find window in linked list, and remove pad */
	if (first_pad == node) /* first in list */
		first_pad = first_pad->next;
	else
	{
		while (current->next != NULL)
		{
			if (current->next->window == window)
			{
				old_node = current->next;
				current->next = current->next->next;
				
				if (old_node == last_pad)
					last_pad = current;
				
				break;
			}
			current = current->next;
		}
	}
	if (old_node != node)
		return; /* must already be destroyed */

	/* then, destroy the widget */
	gtk_widget_destroy (GTK_WIDGET(window));
	node->window = NULL;
	close_pad_files (node);

	g_free(node);

	remove_pad_files (node);

	quit_if_no_pads ();
}


void pad_close (pad_node *node)
{
	GtkWindow *window = node->window;

	if (window == NULL || first_pad == NULL) 
		return;

	/* then, hide the widget */
	gtk_widget_hide(GTK_WIDGET(window));

	(get_pad (window))->hidden = TRUE;

	save_pad (node);
	close_pad_files (node);

	quit_if_no_pads ();
}


void pad_move (pad_node *node, GdkEvent *event)
{
	gint x, y;
	GdkEventButton *eb = (GdkEventButton *) event;

	gdk_window_get_root_origin (GTK_WIDGET(node->window)->window, &x, &y);

	gtk_window_begin_move_drag (node->window, eb->button, eb->x + x, eb->y + y, eb->time);
}

void pad_resize (pad_node *node, GdkEvent *event)
{
	gint x, y;
	GdkEventButton *eb = (GdkEventButton *) event;

	gdk_window_get_root_origin (GTK_WIDGET(node->window)->window, &x, &y);

	gtk_window_begin_resize_drag (node->window, GDK_WINDOW_EDGE_SOUTH_EAST, eb->button, eb->x + x, eb->y + y, eb->time);
}

static gboolean event_handler (GtkWidget *widget, GdkEvent *event, pad_node *pad)
{
	GdkEventButton *event_button;

	g_return_val_if_fail (event != NULL, FALSE);
	
	if (event->type == GDK_BUTTON_PRESS)
	{
		event_button = (GdkEventButton *) event;
		
		if (event_button->button == 1 && event_button->state == GDK_CONTROL_MASK )
		{
			pad_move (pad, event);
			return TRUE;
		}
		else if (event_button->button == 3 && event_button->state == GDK_CONTROL_MASK )
		{
			pad_resize (pad, event);
			return TRUE;
		}
	}

	return FALSE;
}

void about_dialog (pad_node *node)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (node->window,
        		GTK_DIALOG_DESTROY_WITH_PARENT,
        		GTK_MESSAGE_INFO,
        		GTK_BUTTONS_CLOSE,
        		VERSION);

	gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

void popup (GtkTextView *textview, GtkMenu *menu, pad_node *node)
{
	GtkWidget *menu_item_about;
	GtkWidget *menu_item_font;
	GtkWidget *menu_item_new_pad;
	GtkWidget *menu_item_destroy;
	GtkWidget *menu_item_close;
	GtkWidget *menu_item_back_color;
	GtkWidget *menu_item_text_color;
	GtkWidget *separator1, *separator2, *separator3, *separator4;

	separator1 = gtk_separator_menu_item_new ();
	separator2 = gtk_separator_menu_item_new ();
	separator3 = gtk_separator_menu_item_new ();
	separator4 = gtk_separator_menu_item_new ();
	menu_item_about = gtk_menu_item_new_with_mnemonic ("_About");
	menu_item_new_pad = gtk_menu_item_new_with_mnemonic ("_New Pad");
	menu_item_destroy = gtk_menu_item_new_with_mnemonic ("_Destroy");
	menu_item_close = gtk_menu_item_new_with_mnemonic ("_Close");
	menu_item_back_color = gtk_menu_item_new_with_mnemonic ("_Background Color");
	menu_item_text_color = gtk_menu_item_new_with_mnemonic ("_Text Color");
	menu_item_font = gtk_menu_item_new_with_mnemonic ("_Font");

	gtk_menu_shell_prepend (GTK_MENU_SHELL(menu), separator1);
	gtk_menu_shell_prepend (GTK_MENU_SHELL(menu), menu_item_new_pad);

	gtk_menu_shell_append (GTK_MENU_SHELL(menu), separator2);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item_back_color);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item_text_color);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item_font);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), separator4);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item_close);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item_destroy);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), separator3);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item_about);

	gtk_signal_connect_object (GTK_OBJECT (menu_item_destroy), "activate", GTK_SIGNAL_FUNC (pad_destroy), (gpointer) node);
	gtk_signal_connect_object (GTK_OBJECT (menu_item_close), "activate", GTK_SIGNAL_FUNC (pad_close), (gpointer) node);
	gtk_signal_connect (GTK_OBJECT (menu_item_new_pad), "activate", GTK_SIGNAL_FUNC (create_pad), NULL);
	gtk_signal_connect_object (GTK_OBJECT (menu_item_back_color), "activate", GTK_SIGNAL_FUNC (back_color_select), (gpointer) node->window);
	gtk_signal_connect_object (GTK_OBJECT (menu_item_text_color), "activate", GTK_SIGNAL_FUNC (text_color_select), (gpointer) node->window);
	gtk_signal_connect_object (GTK_OBJECT (menu_item_font), "activate", GTK_SIGNAL_FUNC (font_select), (gpointer) node);
	gtk_signal_connect_object (GTK_OBJECT (menu_item_about), "activate", GTK_SIGNAL_FUNC (about_dialog), (gpointer) node);

	gtk_widget_show (menu_item_new_pad);
	gtk_widget_show (separator1);
	gtk_widget_show (separator2);
	gtk_widget_show (separator3);
	gtk_widget_show (separator4);
	gtk_widget_show (menu_item_destroy);
	gtk_widget_show (menu_item_back_color);
	gtk_widget_show (menu_item_text_color);
	gtk_widget_show (menu_item_font);
	gtk_widget_show (menu_item_close);
	gtk_widget_show (menu_item_about);
}


/*
   creates and returns a pad with an *unshown* window -- to 
   be decorated 
*/
pad_node *start_pad ()
{
	GtkWidget *window;
	GtkWidget *textbox;
	pad_node *node = (pad_node *) g_malloc(sizeof(pad_node));	

	/* make widgets */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	textbox = gtk_text_view_new ();

	/* set textbox's properties */
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textbox), TRUE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textbox), GTK_WRAP_WORD);

	/* add textbox to window */
	gtk_container_add (GTK_CONTAINER (window), textbox);

	/* if a wm wants us to close, we close */
	gtk_signal_connect_object (GTK_OBJECT (window), "destroy", GTK_SIGNAL_FUNC(pad_destroy), (gpointer) node);

	gtk_signal_connect (GTK_OBJECT (textbox), "event", GTK_SIGNAL_FUNC(event_handler), (gpointer) node);
	gtk_signal_connect (GTK_OBJECT (textbox), "populate-popup", GTK_SIGNAL_FUNC(popup), (gpointer) node);

	/* display widgets */	
	gtk_widget_show (textbox);
	gtk_widget_realize (window); /* still have to set decorations */

	node->hidden = FALSE;
	node->next = NULL;
	node->window = GTK_WINDOW(window);

	/* check if this is first pad made */
	if (first_pad == NULL)
	{
		last_pad = node;
		first_pad = node;
	}
	else
	{
		last_pad->next = node;
		last_pad = node;
	}

	/* remove wm decorations */
	gtk_window_set_decorated (GTK_WINDOW(window), FALSE);
        
	return node;
}

pad_node *create_pad ()
{
	pad_node *pad = start_pad ();
	GtkStyle *style = gtk_style_new ();
	pad_style *pstyle = get_default_style ();

	gtk_window_set_default_size (pad->window, pstyle->width, pstyle->height);

	style->base[GTK_STATE_NORMAL] = pstyle->back;
	style->text[GTK_STATE_NORMAL] = pstyle->text;
	style->font_desc = pango_font_description_from_string (pstyle->fontname);
	strcpy (pad->fontname, pstyle->fontname);
	gtk_widget_set_style (GTK_WIDGET(get_text (pad->window)), style);

	gtk_widget_show (GTK_WIDGET(pad->window));
	gtk_widget_grab_focus (GTK_WIDGET(get_text (pad->window)));

	g_free (pstyle);

	open_pad_files (pad, TRUE);

	return pad;
}

pad_node *create_pad_with_info (pad_info *info)
{
	pad_node *pad = start_pad ();
	GtkStyle *style = gtk_style_new();
	GtkTextBuffer *buffer;
	GtkTextView *textbox = get_text (pad->window);	

	gtk_window_set_default_size (pad->window, info->style.width, info->style.height);
	gtk_widget_set_uposition (GTK_WIDGET(pad->window), info->x, info->y);

	/* set content of textbox */
	buffer = gtk_text_view_get_buffer (textbox);
	gtk_text_buffer_set_text (buffer, info->content, -1);

	style->base[GTK_STATE_NORMAL] = info->style.back;
	style->text[GTK_STATE_NORMAL] = info->style.text;
	style->font_desc = pango_font_description_from_string (info->style.fontname);
	strcpy (pad->fontname, info->style.fontname);
	gtk_widget_set_style (GTK_WIDGET(textbox), style);

	gtk_widget_show (GTK_WIDGET(pad->window));
	gtk_widget_grab_focus (GTK_WIDGET(textbox));

	strcpy (pad->infoname, info->infoname);
	strcpy (pad->contentname, info->contentname);
	
	open_pad_files (pad, FALSE);

	return pad;
}











