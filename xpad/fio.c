/*

Copyright (c) 2001-2002 Michael Terry

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

#include "fio.h"
#include "pad.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


/* sets filename to full path of filename (prepends working_dir to it) 
   returns 0 if filename was full path, 1 if we added to it.
*/
static gint fio_fill_filename (gchar *filename)
{
	size_t filename_len;

	if (g_path_is_absolute (filename))
		return 0;
	
	filename_len = strlen(filename);
	
	if ((filename_len + working_dir_len) > MAX_FILENAME_SIZE)
	{
		fprintf(stderr, "Full path to file is too long!\n");
		return 0;
	}
	
	memmove(filename + working_dir_len, filename, filename_len + 1);
	memcpy(filename, working_dir, working_dir_len);
	
	return 1;
}

gint fio_set_file (const gchar *name, const gchar *value)
{
	FILE *file;
	gchar temp[MAX_FILENAME_SIZE + 1];
	
	strcpy (temp, name);
	fio_fill_filename (temp);
	
	if ( (file = fopen (temp, "w+")) == NULL)
	{
		if (verbosity >= 1) 
			printf ("Could not open file [%s] for writing.\n", temp);
			return 1;
	}
	
	if (fputs (value, file) == EOF)
	{
		fprintf(stderr, "Failed to write file [%s].\n", name);
		return 1;
	}

	fclose (file);
	
	return 0;
}


gint fio_get_file(const gchar *name, gchar *value, gint size)
{
	gchar temp[MAX_FILENAME_SIZE + 1];
	FILE *file;
	size_t bytesread;

	strcpy(temp, name);
	fio_fill_filename(temp);

	file = fopen(temp, "r");
	if (!file)
	{
		value[0] = '\0';
		if (verbosity >= 1)
			printf("Could not open file [%s] "
				"for reading.\n",
				temp);
		return 1;
	}

	if (size <= 0)
	{
		/* If reading 0 bytes.  Just report success but 
		 * don't try to read anything.
		 */
		 fclose(file);
		 strcpy (value, "");
		 return 0;
	}

	clearerr(file);

	/* Try to read exactly "size + 1" bytes.  Note that we'll actually
	 * only keep at most "size" However, attempting to read that 
	 * one extra byte will let us be absolutely sure if the file is larger
	 * than our buffer.
	 */
	bytesread = fread(value, 1, size + 1, file);

	if (bytesread >= size + 1)
	{
		bytesread = size;
		
		if (verbosity >= 1)
			printf ("Warning: File [%s] larger than "
				"buffer size.",
				temp);
	}

	value[bytesread] = '\0';

	if (ferror(file))
	{
		fclose(file);
		if (verbosity >= 1) 
			printf("Error reading from [%s]: %s\n",
				temp,
				g_strerror(ferror(file)));
		return 1;
	}

	fclose(file);
	return 0;
}


void fio_open_pad_files (pad_node *pad, gboolean create)
{
	if (create)
	{
		int fd;
		
		strcpy (pad->contentname, working_dir);
		strcat (pad->contentname, "content-XXXXXX");
		fd = g_mkstemp (pad->contentname);
		
		if (fd != -1)
			close (fd);
		
		if (verbosity >= 2) printf ("Creating file [%s].\n", pad->contentname);
		
		strcpy (pad->infoname, working_dir);
		strcat (pad->infoname, "info-XXXXXX");
		fd = g_mkstemp (pad->infoname);
		
		if (fd != -1)
			close (fd);
		
		if (verbosity >= 2) printf ("Creating file [%s].\n", pad->infoname);
	}
}


void fio_close_pad_files (pad_node *pad)
{
}


void fio_save_as_defaults (struct settings *set)
{
	gchar buf[MAX_FILE_SIZE + 1];
	
	sprintf (buf, "wm_close %i\nedit_lock %i\nconfirm_destroy %i\n"
		"sync_time %i\ndecorations %i\n"
		"width %i\nheight %i\nback_red %d\nback_green %d\nback_blue %d\n"
		"text_red %d\ntext_green %d\ntext_blue %d\nborder_red %d\nborder_green %d\n"
		"border_blue %d\nborder_width %d\npadding %d\nfontname %s\n",
		set->wm_close, set->edit_lock, set->confirm_destroy,
		set->sync_time, set->decorations,
		set->width, set->height,
		set->style.back.red, set->style.back.green, set->style.back.blue,
		set->style.text.red, set->style.text.green, set->style.text.blue,
		set->style.border.red, set->style.border.green, set->style.border.blue,
		set->style.border_width, set->style.padding,
		set->style.fontname);

	fio_set_file (DEFAULTS_FILENAME, buf);
}


/* list is a variable number of (gchar *) / (gchar * or gint *) groups, 
	terminated by a NULL variable */
gint fio_get_values_from_file (const gchar *filename, ...)
{
	gchar buf[MAX_FILE_SIZE + 1];
	va_list ap;

	if (fio_get_file (filename, buf, MAX_FILE_SIZE))
		return 1;

	va_start (ap, filename);

	while (1)
	{
		gchar *item;
		gint *value;
		gchar *where;
		gchar *temp;
		gint size;

		item = va_arg (ap, gchar *);
		if (!item)
			break;
		value = va_arg (ap, void *);
		where  = strstr (buf, item);

		if (!where) continue;

		where = strstr (where, " ") + 1;

		temp = (gchar *) g_malloc ((size = strcspn (where, "\n")) + 1);
		strncpy (temp, where, size);
		temp[size] = '\0';

		if (g_ascii_isdigit (temp[0]))
			*((gint *) value) = atoi (temp);
		else
			strcpy ((gchar *) value, temp);

		g_free (temp);
	}

	va_end (ap);

	return 0;
}


gint fio_get_style_from_file (const gchar *filename, pad_style *starter)
{
	gint back_R, back_G, back_B,
	     text_R, text_G, text_B,
	     bord_R, bord_G, bord_B;
	gint Result;

	Result = fio_get_values_from_file (filename, 
								"back_red", &back_R,
								"back_green", &back_G,
								"back_blue", &back_B,
								"text_red", &text_R,
								"text_green", &text_G,
								"text_blue", &text_B,
								"border_red", &bord_R,
								"border_green", &bord_G,
								"border_blue", &bord_B,
								"border_width", &starter->border_width,
								"padding", &starter->padding,
								"fontname", starter->fontname,
								NULL );
	if (Result == 0)
	{
		starter->back.red = back_R;
		starter->back.green = back_G;
		starter->back.blue = back_B;
	
		starter->text.red = text_R;
		starter->text.green = text_G;
		starter->text.blue = text_B;
	
		starter->border.red = bord_R;
		starter->border.green = bord_G;
		starter->border.blue = bord_B;
	}

	return Result;
}


static void fio_save_info_file (pad_node *pad)
{
	gchar info_file[MAX_FILE_SIZE + 1];
	gchar *content;
	gchar temp[MAX_FILENAME_SIZE + 1];
	pad_style *pstyle;
	GtkTextIter s, e;
	GtkTextBuffer *buf;

	if (verbosity >= 2) printf ("Saving pad [%s].\n", pad->infoname);

	pstyle = pad_get_style (pad);

	/* we don't really need to save the style, since we don't use it, but it makes
	   later running an older version of xpad nice.  At some point this will be removed. */
    sprintf (info_file, "x %d\ny %d\nwidth %d\nheight %d\nback_red %d\nback_green %d\nback_blue %d\ntext_red %d\ntext_green %d\ntext_blue %d\nborder_red %d\nborder_green %d\nborder_blue %d\nborder_width %d\npadding %d\nfontname %s\n",
		pad->x, pad->y, pad->width, pad->height, 
		pstyle->back.red, pstyle->back.green, pstyle->back.blue,
		pstyle->text.red, pstyle->text.green, pstyle->text.blue,
		pstyle->border.red, pstyle->border.green, pstyle->border.blue,
		pstyle->border_width, pstyle->padding,
		pstyle->fontname);
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
	content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);

	sprintf (temp, "content %s\n", pad->contentname);
	strcat (info_file, temp);

	fio_set_file (pad->contentname, content);
    g_free (content);
	
	fio_set_file (pad->infoname, info_file);
	
	g_free (pstyle);
}

/* save contents and locations of a pad */
void fio_save_pad (pad_node *pad)
{
	fio_save_info_file (pad);
}

/* save contents and locations of all pads */
void fio_save_pads (void)
{
	pad_node *current = first_pad;

	while (current != NULL)
	{
		fio_save_pad (current);
		current = current->next;
	}
}

static void fio_remove_file (gchar *filename)
{
	gchar long_filename[MAX_FILENAME_SIZE + 1];

	if (filename[0] == '/')
		strcpy (long_filename, filename);
	else
		sprintf (long_filename, "%s%s", working_dir, filename);

	remove (long_filename);
}

void fio_remove_pad_files (pad_node *pad)
{
	fio_remove_file (pad->infoname);
	fio_remove_file (pad->contentname);
}

/* filename must be absolute */
static gint fio_get_info_from_file (const gchar *filename, pad_info *info)
{
	if (verbosity >= 2) printf ("Loading [%s].\n", filename);

	/* grab from standard defaults, not from pad's memory of what they were,
	    because a pad might be closed, default changed, and when we load it up again,
	    we want all pads to be uniform */
	fio_get_style_from_file (DEFAULTS_FILENAME, &info->style);
	fio_get_values_from_file (  filename,
							"x", &info->x,
							"y", &info->y,
							"width", &info->width,
							"height", &info->height,
							"content", info->contentname,
							NULL);
	strcpy (info->infoname, filename);

	return 0;
}


void fio_load_pads (void)
{
	gint opened = 0;
	pad_node *pad;
	pad_info info;
	GDir *dir;
	G_CONST_RETURN gchar *name;
	GPatternSpec *spec;

	spec = g_pattern_spec_new ("info-*");

	/* set up some sort of defaults for these.  if xpad works
	   right, these won't be used. */
	info.style = current_settings.style;
	info.x = 0;
	info.y = 0;
	info.width = 260;
	info.height = 260;
	
	dir = g_dir_open (working_dir, 0, NULL);
	
	while ((name = g_dir_read_name (dir)))
	{
		if (g_pattern_match_string (spec, name) &&
			!fio_get_info_from_file (name, &info))
		{
			/* some older versions of xpad only had relative filename
				so, we have to add full path if it isn't there. */
			fio_fill_filename (info.contentname);
			pad = pad_new_with_info (&info);
			opened ++;
		}
	}

	if (opened == 0)
		pad_new ();

	g_pattern_spec_free (spec);
	g_dir_close (dir);
}


