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
