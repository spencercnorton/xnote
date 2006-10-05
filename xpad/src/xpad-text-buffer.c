/**
 * Copyright (c) 2004 Michael Terry
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "xpad-text-buffer.h"

G_DEFINE_TYPE(XpadTextBuffer, xpad_text_buffer, GTK_TYPE_TEXT_BUFFER)

/* Unicode chars in the Private Use Area. */
static gunichar TAG_CHAR = 0xe000;

static GtkTextTagTable *create_tag_table (void);

static GtkTextTagTable *global_text_tag_table = NULL;

GtkTextBuffer *
xpad_text_buffer_new (void)
{
	if (!global_text_tag_table)
		global_text_tag_table = create_tag_table (); /* FIXME: never freed */
	
	return GTK_TEXT_BUFFER (g_object_new (XPAD_TYPE_TEXT_BUFFER, "tag_table", global_text_tag_table, NULL));
}

static void
xpad_text_buffer_class_init (XpadTextBufferClass *klass)
{
}

static void
xpad_text_buffer_init (XpadTextBuffer *buffer)
{
}

void
xpad_text_buffer_set_text_with_tags (XpadTextBuffer *buffer, const gchar *text)
{
	GtkTextIter start, end;
	GList *tags = NULL;
	gchar **tokens;
	gint count;
	gchar tag_char_utf8[7] = {0};
	
	if (!text)
		return;
	
	gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (buffer));
	
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);
	gtk_text_buffer_delete (GTK_TEXT_BUFFER (buffer), &start, &end);
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);
	
	g_unichar_to_utf8 (TAG_CHAR, tag_char_utf8);
	
	tokens = g_strsplit (text, tag_char_utf8, 0);
	
	for (count = 0; tokens[count]; count++)
	{
		if (count % 2 == 0)
		{
			gint offset;
			GList *j;
			
			offset = gtk_text_iter_get_offset (&end);
			gtk_text_buffer_insert (GTK_TEXT_BUFFER (buffer), &end, tokens[count], -1);
			gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &start, offset);
			
			for (j = tags; j; j = j->next)
			{
				gtk_text_buffer_apply_tag_by_name (GTK_TEXT_BUFFER (buffer), j->data, &start, &end);
			}
		}
		else
		{
			if (tokens[count][0] != '/')
			{
				tags = g_list_prepend (tags, tokens[count]);
			}
			else
			{
				GList *element = g_list_find_custom (tags, &(tokens[count][1]), (GCompareFunc) g_ascii_strcasecmp);
				
				if (element)
				{
					tags = g_list_delete_link (tags, element);
				}
			}
		}
	}
	
	gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (buffer));
	
	g_strfreev (tokens);
}


gchar *
xpad_text_buffer_get_text_with_tags (XpadTextBuffer *buffer)
{
	GtkTextIter start, prev;
	GSList *tags = NULL, *i;
	gchar tag_char_utf8[7] = {0};
	gchar *text = g_strdup (""), *oldtext = NULL, *tmp;
	gboolean done = FALSE;
	
	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);
	
	g_unichar_to_utf8 (TAG_CHAR, tag_char_utf8);
	
	prev = start;
	
	while (!done)
	{
		tmp = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer), &prev, &start, TRUE);
		oldtext = text;
		text = g_strconcat (text, tmp, NULL);
		g_free (oldtext);
		g_free (tmp);
		
		tags = gtk_text_iter_get_toggled_tags (&start, TRUE);
		for (i = tags; i; i = i->next)
		{
			gchar *name;
			g_object_get (G_OBJECT (i->data), "name", &name, NULL);
			oldtext = text;
			text = g_strconcat (text, tag_char_utf8, name, tag_char_utf8, NULL);
			g_free (oldtext);
			g_free (name);
		}
		g_slist_free (tags);
		
		tags = gtk_text_iter_get_toggled_tags (&start, FALSE);
		for (i = tags; i; i = i->next)
		{
			gchar *name;
			g_object_get (G_OBJECT (i->data), "name", &name, NULL);
			oldtext = text;
			text = g_strconcat (text, tag_char_utf8, "/", name, tag_char_utf8, NULL);
			g_free (oldtext);
			g_free (name);
		}
		g_slist_free (tags);
		
		if (gtk_text_iter_is_end (&start))
			done = TRUE;
		prev = start;
		gtk_text_iter_forward_to_tag_toggle (&start, NULL);
	}
	
	return text;
}

static GtkTextTagTable *
create_tag_table (void)
{
	GtkTextTagTable *table;
	GtkTextTag *tag;
	
	table = gtk_text_tag_table_new ();
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "bold", "weight", PANGO_WEIGHT_BOLD, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "italic", "style", PANGO_STYLE_ITALIC, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "strikethrough", "strikethrough", TRUE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "underline", "underline", PANGO_UNDERLINE_SINGLE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "small-xx", "scale", PANGO_SCALE_XX_SMALL, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "small-x", "scale", PANGO_SCALE_X_SMALL, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "small", "scale", PANGO_SCALE_SMALL, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "medium", "scale", PANGO_SCALE_MEDIUM, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "large", "scale", PANGO_SCALE_LARGE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "large-x", "scale", PANGO_SCALE_X_LARGE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "large-xx", "scale", PANGO_SCALE_XX_LARGE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	return table;
}
