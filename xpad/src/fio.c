/*

Copyright (c) 2001-2003 Michael Terry

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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fio.h"
#include "pad.h"
#include "xpad-settings.h"
#include "xpad-app.h"
#include "xpad-text-view.h"


/* sets filename to full path of filename (prepends xpad_app_get_config_dir () to it) 
   returns 0 if filename was full path, 1 if we added to it.
   
   returned name must be g_free'd
*/
static gchar *fio_fill_filename (const gchar *filename)
{
	if (g_path_is_absolute (filename))
		return g_strdup (filename);
	
	return g_build_filename (xpad_app_get_config_dir (), filename, NULL);
}

/* This callously overwrites name.bak -- but this is fine since this function is for
   our private .xpad directory anyway */
gboolean fio_set_file (const gchar *name, const gchar *value)
{
	FILE *file = NULL;
	gchar *fullpath, *backup;
	gboolean error = FALSE, moved = TRUE;
	
	fullpath = fio_fill_filename (name);
	backup = g_strconcat (fullpath, "~", NULL);
	
	/* we first move the file away so that if the write doesn't succeed, we don't lose data */
	if (g_file_test (fullpath, G_FILE_TEST_EXISTS) && rename (fullpath, backup))
	{
		printf ("errno is %i - from %s to %s\n", errno, fullpath, backup);
		error = TRUE;
		moved = FALSE;
	}
	
	if (!error && (file = fopen (fullpath, "w")) == NULL)
	{
		error = TRUE;
	}
	
	if (!error && fputs (value, file) == EOF)
	{
		error = TRUE;
	}
	
	if (error)
	{
		gchar *usertext;
		
		/* move the file back */
		if (moved)
		{
			rename (backup, fullpath);
		}
		
		usertext = g_strdup_printf (_("Could not write to file %s."), fullpath);
		xpad_app_error (NULL, usertext, NULL);
		g_free (usertext);
	}
	
	g_free (fullpath);
	g_free (backup);
	if (file) fclose (file);
	return !error;
}


/**
 * Returned gchar * must be g_free'd.
 */
gchar *fio_get_file (const gchar *name)
{
	gchar *fullname;
	gchar *rv;
	
	fullname = fio_fill_filename (name);
	
	if (!g_file_get_contents (fullname, &rv, NULL, NULL))
	{
		gchar *usertext;
		
		usertext = g_strdup_printf (_("Could not read from file %s."), fullname);
		xpad_app_error (NULL, usertext, NULL);
		g_free (usertext);
		
		rv = NULL;
	}
	
	g_free (fullname);
	return rv;
}


/**
 * Unfortunately, this function is necessary, since mkstemp() is not portable,
 * and g_mkstemp() makes the file and opens it, which for some god-forsaken reason,
 * requires you to close it with close (), which is not portable, ruining the point of 
 * providing a cross-platform mkstemp.
 * 
 * This will return a string that must be g_free'd.  This filename is guaranteed to be
 * non-existant.
 */ 
gchar *fio_find_free_filename (gchar *pattern)
{
	gchar *s = NULL;
	
	do
	{
		guint32 num = g_random_int ();
		gchar *numstr;
		
		numstr = g_strdup_printf ("%s%u", pattern, num);
		
		if (s)
			g_free (s);
		
		s = g_build_filename (xpad_app_get_config_dir (), numstr, NULL);
		
		g_free (numstr);
	}
	while (g_file_test (s, G_FILE_TEST_EXISTS));
	
	return s;
}


/**
 * This opens any persistant pad files.  It also optionally assigns their names if
 * the |create| flag is true.
 */
void fio_open_pad_files (pad_node *pad, gboolean create)
{
	if (create)
	{
		pad->contentname = fio_find_free_filename ("content-");
		fio_set_file (pad->contentname, "");
		
		pad->infoname = fio_find_free_filename ("info-");
		fio_set_file (pad->infoname, "");
	}
}



/* list is a variable number of (gchar *) / (gchar ** or gint *) groups, 
	terminated by a NULL variable */
/**
 * each gchar ** pointer will hereafter point to memory that must be g_free'd.
 */
gint fio_get_values_from_file (const gchar *filename, ...)
{
	gchar *buf;
	const gchar *item;
	va_list ap;
	
	buf = fio_get_file (filename);
	
	if (!buf)
		return 1;
	
	/* because of the way we look for a matching variable name, which is
		to look for an endline, the variable name, and a space, we insert a
		newline at the beginning, so that the first variable name is caught. */
	buf = g_realloc (buf, strlen (buf) + 1);
	g_memmove (buf + 1, buf, strlen (buf));
	buf[0] = '\n';
	
	va_start (ap, filename);

	while ((item = va_arg (ap, gchar *)))
	{
		gchar *fullitem;
		gint *value;
		gchar *where;
		gint size;
		gchar type;
		
		type = item[0];
		item = &item[2]; /* skip type and '|' */
		fullitem = g_strdup_printf ("\n%s ", item);
		value = va_arg (ap, void *);
		where  = strstr (buf, fullitem);
		
		if (where)
		{
			gchar *temp;

			where = strstr (where, " ") + 1;
			size = strcspn (where, "\n");
		
			temp = g_malloc (size + 1);
			strncpy (temp, where, size);
			temp[size] = '\0';
			
			switch (type)
			{
			case 'i':
				*((gint *) value) = atoi (temp);
				break;
			case 'u':
				*((guint *) value) = (guint) strtoul (temp, NULL, 0);
				break;
			case 's':
				g_free (*((gchar **) value));
				*((gchar **) value) = g_strdup (temp);
				break;
			case 'b':
				*((gboolean *) value) = atoi (temp) ? TRUE : FALSE;
				break;
			}
		
			g_free (temp);
		}
		g_free (fullitem);
	}

	va_end (ap);
	g_free (buf);

	return 0;
}

/* list is a variable number of (gchar *) / (gchar ** or gint *) groups, 
	terminated by a NULL variable */
gint fio_set_values_to_file (const gchar *filename, ...)
{
	gchar *buf, *tmpbuf;
	const gchar *item;
	va_list ap;
	
	va_start (ap, filename);
	
	buf = g_strdup ("");
	while ((item = va_arg (ap, gchar *)))
	{
		gchar *tmp_string, *final_string;
		union {
			gchar *s;
			gint i;
			guint u;
		} value;
		gchar type;
		
		type = item[0];
		item = &item[2]; /* skip type and '|' */
		
		/* translate our types to printf types */
		switch (type)
		{
		case 'b':
			type = 'i';
			value.i = va_arg (ap, gboolean);
			break;
		case 'i':
			value.i = va_arg (ap, gint);
			break;
		case 'u':
			value.u = va_arg (ap, guint);
			break;
		case 's':
			value.s = va_arg (ap, gchar *);
			break;
		}
		
		tmp_string = g_strdup_printf ("%s%s %%%c", (buf[0] == 0) ? "" : "\n", item, type);
		final_string = g_strdup_printf (tmp_string, value);
		g_free (tmp_string);
		
		tmpbuf = buf;
		buf = g_strconcat (buf, final_string, NULL);
		g_free (tmpbuf);
		g_free (final_string);
	}
	
	va_end (ap);
	
	tmpbuf = buf;
	buf = g_strconcat (buf, "\n", NULL);
	g_free (tmpbuf);
	
	if (!fio_set_file (filename, buf))
	{
		g_free (buf);
		return 1;
	}
	
	g_free (buf);
	return 0;
}

void fio_save_pad_info (pad_node *pad)
{
	gchar *info_file;
	gint height;
	GtkRcStyle *style;
	gchar *fontname;
	
	if (!pad || !pad->infoname)	/* don't bother saving hidden pads FIXME (bump check for hidden up) */
		return;
	
	height = pad->height;
	/*if (GTK_WIDGET_IS_VISIBLE (pad->toolbar))
		height -= pad->toolbar->height;*/
	
	style = gtk_widget_get_modifier_style (pad->textview);
	fontname = style->font_desc ? pango_font_description_to_string (style->font_desc) : NULL;
	
	info_file = g_strdup_printf (
		"x %d\ny %d\nwidth %d\nheight %d\nlocked %d\ncontent %s\n"
		"sticky %d\nback_red %d\nback_green %d\nback_blue %d\nuse_back %d\n"
		"text_red %d\ntext_green %d\ntext_blue %d\nuse_text %d\nfontname %s\n"
		"hidden %d\n",
		pad->x, pad->y, pad->width, height, !xpad_text_view_get_follow_global_style (XPAD_TEXT_VIEW (pad->textview)),
		pad->contentname, pad->sticky,
		style->base[GTK_STATE_NORMAL].red, style->base[GTK_STATE_NORMAL].green, style->base[GTK_STATE_NORMAL].blue,
		(style->color_flags[GTK_STATE_NORMAL] & GTK_RC_BASE) ? TRUE : FALSE,
		style->text[GTK_STATE_NORMAL].red, style->text[GTK_STATE_NORMAL].green, style->text[GTK_STATE_NORMAL].blue,
		(style->color_flags[GTK_STATE_NORMAL] & GTK_RC_TEXT) ? TRUE : FALSE,
		fontname ? fontname : "NULL",
		pad->hidden ? 1 : 0);
	
	fio_set_file (pad->infoname, info_file);
	
	g_free (fontname);
	g_free (info_file);
}

void fio_save_pad_content (pad_node *pad)
{
	gchar *content;
	GtkTextIter s, e;
	GtkTextBuffer *buf;
	
	if (!pad || !pad->contentname)
		return;
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->textview));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
	content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);
	
	fio_set_file (pad->contentname, content);
	
	g_free (content);
}

/* save contents and locations of a pad */
void fio_save_pad (pad_node *pad)
{
	fio_save_pad_info (pad);
	fio_save_pad_content (pad);
}

/* save contents and locations of all pads */
void fio_save_pads (void)
{
	pad_node *current;

	for (current = first_pad; current; current = current->next)
		fio_save_pad (current);
}

void fio_remove_file (const gchar *filename)
{
	gchar *temp;
	
	temp = fio_fill_filename (filename);
	
	remove (temp);
	
	g_free (temp);
}

void fio_remove_pad_files (pad_node *pad)
{
	gchar *infobackup, *contentbackup;
	
	infobackup = g_strconcat (pad->infoname, "~", NULL);
	contentbackup = g_strconcat (pad->contentname, "~", NULL);
	fio_remove_file (pad->infoname);
	fio_remove_file (pad->contentname);
	fio_remove_file (infobackup);
	fio_remove_file (contentbackup);
	
	g_free (infobackup);
	g_free (contentbackup);
}


/* filename must be absolute */
gint fio_get_info_from_file (const gchar *filename, pad_info *info)
{
	GdkColor text, back;
	gboolean use_text, use_back;
	
	/* set up some sort of defaults for these.  if the value
	   doesn't exist in the file, these will be used instead. */
	info->x = 0;
	info->y = 0;
	info->width = xpad_settings_get_width (xpad_settings ());
	info->height = xpad_settings_get_height (xpad_settings ());
	info->hidden = FALSE;
	info->sticky = xpad_settings_get_sticky (xpad_settings ());
	info->locked = 0;
	info->text = gdk_color_copy (xpad_settings_get_text_color (xpad_settings ()));
	info->back = gdk_color_copy (xpad_settings_get_back_color (xpad_settings ()));
	info->fontname = g_strdup (xpad_settings_get_fontname (xpad_settings ()));
	info->infoname = g_strdup (filename);
	info->contentname = NULL;
	
	use_text = xpad_settings_get_text_color (xpad_settings ()) ? TRUE : FALSE;
	text = *xpad_settings_get_text_color (xpad_settings ());
	
	use_back = xpad_settings_get_back_color (xpad_settings ()) ? TRUE : FALSE;
	back = *xpad_settings_get_back_color (xpad_settings ());
	
	fio_get_values_from_file (filename,
		"i|x", &info->x,
		"i|y", &info->y,
		"i|width", &info->width,
		"i|height", &info->height,
		"s|content", &info->contentname,
		"i|locked", &info->locked,
		"i|sticky", &info->sticky,
		"i|back_red", &back.red,
		"i|back_green", &back.green,
		"i|back_blue", &back.blue,
		"b|use_back", &use_back,
		"i|text_red", &text.red,
		"i|text_green", &text.green,
		"i|text_blue", &text.blue,
		"b|use_text", &use_text,
		"s|fontname", &info->fontname,
		"i|hidden", &info->hidden,
		NULL);
	
	if (use_text)
	{
		gdk_color_free (info->text);
		info->text = gdk_color_copy (&text);
	}
	
	if (use_back)
	{
		gdk_color_free (info->back);
		info->back = gdk_color_copy (&back);
	}
	
	if (!info->fontname && xpad_settings_get_fontname (xpad_settings ()))
	{
		info->fontname = g_strdup (xpad_settings_get_fontname (xpad_settings ()));
	}
	else if (strcmp (info->fontname, "NULL") == 0)
	{
		g_free (info->fontname);
		info->fontname = NULL;
	}
	
	return 0;
}
