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
	const gchar *errtext;
	
	temp = fio_fill_filename (name);
	
	if ( (file = fopen (temp, "w")) == NULL)
	{
		errtext = strerror(errno);
		fprintf (stderr, 
		         "Could not open file [%s] for writing: %s.\n", 
			 temp,
			 errtext);
		xpad_display_dialog_with_text (GTK_MESSAGE_ERROR, errtext);
		
		g_free (temp);
		return FALSE;
	}
	
	g_free (temp);
	
	if (fputs (value, file) == EOF)
	{
		errtext = strerror(errno);
		fprintf (stderr, 
		         "Failed to write file [%s]: %s.\n", 
			 name,
			 errtext);
		xpad_display_dialog_with_text (GTK_MESSAGE_ERROR, errtext);
		
		fclose (file);
		return FALSE;
	}
	
	fclose (file);
	return TRUE;
}


/**
 * Returned gchar * must be g_free'd.
 */
gchar *fio_get_file (const gchar *name)
{
	gchar *fullname;
	gchar *contents;
	GError *errCode = NULL;
	const gchar *errortext;
	
	fullname = fio_fill_filename (name);
	
	if (!g_file_get_contents (fullname, &contents, NULL, &errCode))
	{
		errortext = errCode ? errCode->message : NULL;
		if (!errortext) errortext = "Unknown error!";
		if (verbosity >= 1)
			fprintf (stderr, 
			    	"Failed to read file [%s]: %s.\n", 
				name,
				errortext);
		xpad_display_dialog_with_text (GTK_MESSAGE_ERROR, errortext);
		g_free (fullname);
		g_error_free (errCode);
		return NULL;
	}
	else
	{
		g_free (fullname);
		return contents;
	}
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
		
		pad->infoname = fio_find_free_filename ("info-");
		if (verbosity >= 2) printf ("Creating file [%s].\n", pad->infoname);
	}
}


/**
 * This closes any persistant pad files.
 */
void fio_close_pad_files (pad_node *pad)
{
}


void fio_save_default_settings (void)
{
	gchar buf[MAX_FILE_SIZE + 1];
	GSList *tmp;
	
	sprintf (buf, "wm_close %i\nedit_lock %i\nconfirm_destroy %i\n"
		"sync_time %i\ndecorations %i\n"
		"width %i\nheight %i\nback_red %d\nback_green %d\nback_blue %d\n"
		"text_red %d\ntext_green %d\ntext_blue %d\nborder_red %d\nborder_green %d\n"
		"border_blue %d\nborder_width %d\npadding %d\nfontname %s\nbuttons ",
		current_settings.wm_close, current_settings.edit_lock, current_settings.confirm_destroy,
		current_settings.sync_time, current_settings.decorations,
		current_settings.width, current_settings.height,
		current_settings.style.back.red, current_settings.style.back.green, current_settings.style.back.blue,
		current_settings.style.text.red, current_settings.style.text.green, current_settings.style.text.blue,
		current_settings.style.border.red, current_settings.style.border.green, current_settings.style.border.blue,
		current_settings.style.border_width, current_settings.style.padding,
		current_settings.style.fontname);
	
	tmp = current_settings.toolbar;
	
	while (tmp)
	{
		strcat (buf, tmp->data);
		tmp = tmp->next;
		
		if (tmp)
			strcat (buf, ", ");
	}
	
	strcat (buf, "\n");
	
	fio_set_file (DEFAULTS_FILENAME, buf);
}


/* list is a variable number of (gchar *) / (gchar ** or gint *) groups, 
	terminated by a NULL variable */
/**
 * each gchar ** pointer will hereafter point to memory that must be g_free'd.
 */
gint fio_get_values_from_file (const gchar *filename, ...)
{
	gchar *buf;
	va_list ap;
	
	buf = fio_get_file (filename);
	
	if (!buf)
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
			*((gchar **) value) = g_strdup (temp);
		
		g_free (temp);
	}

	va_end (ap);
	g_free (buf);

	return 0;
}


static void fio_save_info_file (pad_node *pad)
{
	gchar info_file[MAX_FILE_SIZE + 1];
	gchar *content;
	GtkTextIter s, e;
	GtkTextBuffer *buf;
	gint height;
	
	if (verbosity >= 2) printf ("Saving pad [%s].\n", pad->infoname);
	
	height = pad->height;
	if (GTK_WIDGET_VISIBLE (pad->toolbar->bar))
		height -= pad->toolbar->height;
	
	/* we don't really need to save the style, since we don't use it, but it makes
	   later running an older version of xpad nice.  At some point this will be removed. */
    sprintf (info_file, "x %d\ny %d\nwidth %d\nheight %d\ncontent %s\n",
		pad->x, pad->y, pad->width, height, 
		pad->contentname);
	
	fio_set_file (pad->infoname, info_file);
	
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

gint fio_load_default_settings (void)
{
	/**
	 * We need to set up int values for all these to take the value from the file.
	 * These will be assigned back to the appropriate values after load.
	 */
	gint back_R = current_settings.style.back.red,
		 back_G = current_settings.style.back.green,
		 back_B = current_settings.style.back.blue,
		 
	     text_R = current_settings.style.text.red,
		 text_G = current_settings.style.text.green,
		 text_B = current_settings.style.text.blue,
		 
	     bord_R = current_settings.style.border.red,
		 bord_G = current_settings.style.border.green,
		 bord_B = current_settings.style.border.blue;
	
	gchar *buttons = NULL;
	
	if (fio_get_values_from_file (DEFAULTS_FILENAME, 
						"decorations", &current_settings.decorations,
						"sync_time", &current_settings.sync_time,
						"height", &current_settings.height,
						"width", &current_settings.width,
						"confirm_destroy", &current_settings.confirm_destroy,
						"edit_lock", &current_settings.edit_lock,
						"wm_close", &current_settings.wm_close,
						"back_red", &back_R,
						"back_green", &back_G,
						"back_blue", &back_B,
						"text_red", &text_R,
						"text_green", &text_G,
						"text_blue", &text_B,
						"border_red", &bord_R,
						"border_green", &bord_G,
						"border_blue", &bord_B,
						"border_width", &current_settings.style.border_width,
						"padding", &current_settings.style.padding,
						"fontname", &current_settings.style.fontname,
						"buttons", &buttons,
						NULL ))
		return 1;
	
	current_settings.style.back.red = back_R;
	current_settings.style.back.green = back_G;
	current_settings.style.back.blue = back_B;
	
	current_settings.style.text.red = text_R;
	current_settings.style.text.green = text_G;
	current_settings.style.text.blue = text_B;
	
	current_settings.style.border.red = bord_R;
	current_settings.style.border.green = bord_G;
	current_settings.style.border.blue = bord_B;
	
	if (!buttons) /* no buttons specified, so we make our own */
	{
		current_settings.toolbar = g_slist_append (current_settings.toolbar, g_strdup ("New"));
		current_settings.toolbar = g_slist_append (current_settings.toolbar, g_strdup ("Delete"));
		current_settings.toolbar = g_slist_append (current_settings.toolbar, g_strdup ("sep"));
		current_settings.toolbar = g_slist_append (current_settings.toolbar, g_strdup ("Clear"));
	}
	else
	{
		gint i;
		gchar **button_names;
		gchar *temp;
		gchar *dup;
		
		button_names = g_strsplit (buttons, ",", 50);
		
		i = 0;
		while ((temp = button_names[i++]))
		{
			dup = g_strstrip (g_strdup (temp));
			
			current_settings.toolbar = g_slist_append (current_settings.toolbar, dup);
		}
		
		g_strfreev  (button_names);
		g_free (buttons);
	}
	
	return 0;
}

/* filename must be absolute */
static gint fio_get_info_from_file (const gchar *filename, pad_info *info)
{
	if (verbosity >= 2) printf ("Loading [%s].\n", filename);

	fio_get_values_from_file (  filename,
							"x", &info->x,
							"y", &info->y,
							"width", &info->width,
							"height", &info->height,
							"content", &info->contentname,
							NULL);
	info->infoname = g_strdup (filename);

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
	info.x = 0;
	info.y = 0;
	info.width = 260;
	info.height = 260;
	
	dir = g_dir_open (working_dir, 0, NULL);
	
	if (!dir)
	{
		fprintf (stderr, "Can't open working directory [%s].\n", working_dir);
		gtk_main_quit ();
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
	
	if (opened == 0)
		pad_new ();
	
	g_pattern_spec_free (spec);
	g_dir_close (dir);
}


