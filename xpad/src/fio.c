/*

Copyright (c) 2001-2004 Michael Terry

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
#include <glib/gi18n.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "fio.h"
#include "xpad-app.h"

/* sets filename to full path of filename (prepends xpad_app_get_config_dir () to it) 
   returns 0 if filename was full path, 1 if we added to it.
   
   returned name must be g_free'd
*/
static gchar *
fio_fill_filename (const gchar *filename)
{
	if (g_path_is_absolute (filename))
		return g_strdup (filename);
	
	return g_build_filename (xpad_app_get_config_dir (), filename, NULL);
}

/* This function returns 'string' with all instances
   of 'obj' replaced with instances of 'replacement'
   It modifies string and re-allocs it.
   */
gchar *str_replace_tokens (gchar **string, gchar obj, gchar *replacement)
{
	gchar *p;
	gint rsize = strlen (replacement);
	gint osize = 1;
	gint diff = rsize - osize;
	
	p = *string;
	while ((p = strchr (p, obj)))
	{
		*string = g_realloc (*string, strlen (*string) + diff + 1);
		g_memmove (p + rsize, p + osize, strlen (p + osize) + 1);
		
		memcpy (p, replacement, rsize);
		
		p = p + rsize;
	}
	
	return *string;
}

gchar *
fio_unique_name (const gchar *prefix)
{
	int fd;
	gchar *name, *base, *pattern;
	
	pattern = g_strconcat (prefix, "XXXXXX", NULL);
	name = fio_fill_filename (pattern);
	g_free (pattern);
	fd = g_mkstemp (name);
	if (fd == -1)
	{
		g_free (name);
		return NULL;
	}
	
	close (fd);
	base = g_path_get_basename (name);
	
	g_free (name);
	
	return base;
}

/* This callously overwrites name~ -- but this is fine since this function is for
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
	gchar *rv = NULL;
	
	fullname = fio_fill_filename (name);
	
	g_file_get_contents (fullname, &rv, NULL, NULL);
	
	g_free (fullname);
	return rv;
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

void fio_remove_file (const gchar *filename)
{
	gchar *temp;
	gchar *backup;
	
	temp = fio_fill_filename (filename);
	backup = g_strconcat (temp, "~", NULL);
	
	remove (temp);
	remove (backup);
	
	g_free (temp);
	g_free (backup);
}
