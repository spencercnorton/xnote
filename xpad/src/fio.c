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

#include "fio.h"
#include "pad.h"
#include "main.h"
#include "toolbar.h"
#include "settings.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/* sets filename to full path of filename (prepends working_dir to it) 
   returns 0 if filename was full path, 1 if we added to it.
   
   returned name must be g_free'd
*/
static gchar *fio_fill_filename (const gchar *filename)
{
	if (g_path_is_absolute (filename))
		return g_strdup (filename);
	
	return g_build_filename (working_dir, filename, NULL);
}

gboolean fio_set_file (const gchar *name, const gchar *value)
{
	FILE *file;
	gchar *temp;
	gboolean error = FALSE;
	
	temp = fio_fill_filename (name);
	
	if ((file = fopen (temp, "w")) == NULL)
	{
		error = TRUE;
	}
	
	if (!error && fputs (value, file) == EOF)
	{
		error = TRUE;
	}
	
	if (error)
	{
		gchar usertext [524];
		
		sprintf (usertext, _("Could not write to file %s."), temp);
		xpad_show_error (NULL, usertext, NULL);
	}
	
	g_free (temp);
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
		gchar usertext[524];
		
		sprintf (usertext, _("Could not read from file %s."), fullname);
		xpad_show_error (NULL, usertext, NULL);
		
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
		gchar *numstr = (gchar *) g_malloc (strlen (pattern) + 11); /* 10 for size of largest num, 1 for null byte */
		
		sprintf (numstr, "%s%u", pattern, num);
		
		if (s)
			g_free (s);
		
		s = g_build_filename (working_dir, numstr, NULL);
		
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
		if (verbosity >= 2) printf ("Creating file [%s].\n", pad->contentname);
		fio_set_file (pad->contentname, "");
		
		pad->infoname = fio_find_free_filename ("info-");
		if (verbosity >= 2) printf ("Creating file [%s].\n", pad->infoname);
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
		
		fullitem = (gchar *) g_malloc (strlen (item) + 3);
		sprintf (fullitem, "\n%s ", item);
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
		
			if (g_ascii_isdigit (temp[0]))
				*((gint *) value) = atoi (temp);
			else
				*((gchar **) value) = g_strdup (temp);
		
			g_free (temp);
		}
		g_free (fullitem);
	}

	va_end (ap);
	g_free (buf);

	return 0;
}


void fio_save_pad_info (pad_node *pad)
{
	gchar info_file[MAX_FILE_SIZE + 1];
	gint height;
	
	if (!pad || pad->hidden || !pad->infoname)	/* don't bother saving hidden pads */
		return;
	
	if (verbosity >= 2) printf ("Saving pad [%s].\n", pad->infoname);
	
	height = pad->height;
	if (toolbar_is_visible (pad->toolbar))
		height -= pad->toolbar->height;
	
	sprintf (info_file, "x %d\ny %d\nwidth %d\nheight %d\nlocked %d\ncontent %s\n"
		"sticky %d\nback_red %d\nback_green %d\nback_blue %d\nuse_back %d\n"
		"text_red %d\ntext_green %d\ntext_blue %d\nuse_text %d\n"
		"border_red %d\nborder_green %d\n"
		"border_blue %d\nborder_width %d\npadding %d\nfontname %s\n",
		pad->x, pad->y, pad->width, height, pad->locked,
		pad->contentname, pad->sticky,
		pad->style.back.red, pad->style.back.green, pad->style.back.blue,
		pad->style.use_back,
		pad->style.text.red, pad->style.text.green, pad->style.text.blue,
		pad->style.use_text,
		pad->style.border.red, pad->style.border.green, pad->style.border.blue,
		pad->style.border_width, pad->style.padding,
		pad->style.fontname ? pad->style.fontname : "NULL");
	
	fio_set_file (pad->infoname, info_file);
}

void fio_save_pad_content (pad_node *pad)
{
	gchar *content;
	GtkTextIter s, e;
	GtkTextBuffer *buf;
	
	if (!pad || !pad->contentname)
		return;
	
	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
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
	fio_remove_file (pad->infoname);
	fio_remove_file (pad->contentname);
}


/* filename must be absolute */
static gint fio_get_info_from_file (const gchar *filename, pad_info *info)
{
	/**
	 * We need to set up int values for all these to take the value from the file.
	 * These will be assigned back to the appropriate values after load.
	 */
	GdkColor back = xpad_settings_style_get_back_color (),
		text = xpad_settings_style_get_text_color (),
		border = xpad_settings_style_get_border_color ();
	gint 	back_R = back.red,
		back_G = back.green,
		back_B = back.blue,
		
		text_R = text.red,
		text_G = text.green,
		text_B = text.blue,
		
		bord_R = border.red,
		bord_G = border.green,
		bord_B = border.blue;

	if (verbosity >= 2) printf ("Loading [%s].\n", filename);
	
	info->style.fontname = NULL;
	info->style.padding = xpad_settings_style_get_padding ();
	info->style.border_width = xpad_settings_style_get_border_width ();
	
	fio_get_values_from_file (  filename,
		"x", &info->x,
		"y", &info->y,
		"width", &info->width,
		"height", &info->height,
		"content", &info->contentname,
		"locked", &info->locked,
		"sticky", &info->sticky,
		"back_red", &back_R,
		"back_green", &back_G,
		"back_blue", &back_B,
		"use_back", &info->style.use_back,
		"text_red", &text_R,
		"text_green", &text_G,
		"text_blue", &text_B,
		"use_text", &info->style.use_text,
		"border_red", &bord_R,
		"border_green", &bord_G,
		"border_blue", &bord_B,
		"border_width", &info->style.border_width,
		"padding", &info->style.padding,
		"fontname", &info->style.fontname,
		NULL);
	info->infoname = g_strdup (filename);
	
	if (!info->style.fontname && xpad_settings_style_get_fontname ())
	{
		info->style.fontname = g_strdup (xpad_settings_style_get_fontname ());
	}
	else if (strcmp (info->style.fontname, "NULL") == 0)
	{
		g_free (info->style.fontname);
		info->style.fontname = NULL;
	}
	
	info->style.back.red = back_R;
	info->style.back.green = back_G;
	info->style.back.blue = back_B;
	
	info->style.text.red = text_R;
	info->style.text.green = text_G;
	info->style.text.blue = text_B;
	
	info->style.border.red = bord_R;
	info->style.border.green = bord_G;
	info->style.border.blue = bord_B;
	
	return 0;
}


int fio_load_pads (void)
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
	info.x = 0;
	info.y = 0;
	info.width = 260;
	info.height = 260;
	info.sticky = 0;
	info.locked = 0;
	
	dir = g_dir_open (working_dir, 0, NULL);
	
	if (!dir)
	{
		gchar errtext [500];
		
		sprintf (errtext, _("Could not open xpad directory %s."), working_dir);
		
		xpad_show_error (NULL, errtext,
			_("This directory is needed to store preference and pad information.  Xpad will close now."));
		
		gtk_main_quit ();
		return -1;
	}
	
	while ((name = g_dir_read_name (dir)))
	{
		if (g_pattern_match_string (spec, name) &&
			!fio_get_info_from_file (name, &info))
		{
			/**
			 * Fill pad from the info struct.  We don't need to free strings
			 * because the new pad takes over their care.
			 */
			pad = pad_new_with_info (&info);
			opened ++;
		}
	}
	
	if (verbosity >= 2) printf ("Done loading files.\n");
	
	g_pattern_spec_free (spec);
	g_dir_close (dir);
	
	return opened;
}


