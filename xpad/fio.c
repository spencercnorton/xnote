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
#include <glob.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>


const gchar *DEFAULTS_FILENAME = "default-style";


/* sets filename to full path of filename (prepends working_dir to it) 
   returns 0 if filename was full path, 1 if we added to it.
*/
gint fio_fill_filename (gchar *filename)
{
	if (filename[0] == '/')
		return 0;
	else
	{
		gchar temp[MAX_FILENAME_SIZE + 1];
		
		strcpy (temp, filename);
	        strcpy (filename, working_dir);
        	strcat (filename, temp);
        	
        	return 1;
	}
}

gint fio_set_file (const gchar *name, const gchar *value)
{
        FILE *file;
        gchar temp[MAX_FILENAME_SIZE + 1];

	strcpy (temp, name);
	fio_fill_filename (temp);
        
        if ( (file = fopen (temp, "w")) == NULL)
        {
                if (verbosity >= 1) printf ("Could not open file [%s] for writing.\n", temp);
                return 1;
        }

        fputs (value, file);

        fclose (file);

	return 0;
}


// a negative size means all of the file
gint fio_get_file (const gchar *name, gchar *value, const gint size)
{
	gchar temp[MAX_FILENAME_SIZE + 1];
        gchar c[2];
	FILE *file;
	unsigned int counter = 0;

	strcpy (temp, name);
	fio_fill_filename (temp);

	strcpy (value, "");

	/* check if file exists */
        if ( (file = fopen (temp, "r")) == NULL)
        {
		if (verbosity >= 1) printf ("Could not open file [%s] for reading.\n", temp);
		return 1;
	}
	else
        {
	        c[1] = '\0';

	        while ( (c[0] = fgetc(file)) != EOF )
	                if (size < 0 || counter++ < size)
					strcat (value, c);
				else
				{
					if (verbosity >= 1) printf ("Reached size limit for file [%s].\n", temp);
					break;
				}
	}
	
	fclose (file);
	
	return 0;
}

void fio_open_pad_files (pad_node *pad, gboolean create)
{
	struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0};
	
	fl.l_pid = getpid ();
	
	if (create == TRUE)
	{
		strcpy (pad->infoname, working_dir);
		strcat (pad->infoname, "info-XXXXXX");
		mkstemp (pad->infoname);
		if (verbosity >= 2) printf ("Creating file [%s].\n", pad->infoname);
		
		strcpy (pad->contentname, working_dir);
		strcat (pad->contentname, "content-XXXXXX");
		mkstemp (pad->contentname);
		if (verbosity >= 2) printf ("Creating file [%s].\n", pad->contentname);
	}

	if ( (pad->file = fopen (pad->infoname, "w")) == NULL)
        {
                if (verbosity >= 1) printf ("Could not open file [%s] for writing.\n", pad->infoname);
                return;
        }

	if (verbosity >= 2) printf ("Locking file [%s].\n", pad->infoname);

	fcntl (fileno(pad->file), F_SETLK, &fl);
}


void fio_close_pad_files (pad_node *pad)
{
	struct flock fl = {F_UNLCK, SEEK_SET, 0, 0, 0};
	fl.l_pid = getpid ();
	fcntl (fileno(pad->file), F_SETLK, &fl);
	fclose (pad->file);
}


void fio_save_defaults ()
{
	gchar buf[MAX_FILE_SIZE + 1];

	sprintf (buf, "confirm_destroy %i\nsync_time %i\ndecorations %i\nwidth %i\nheight %i\nback_red %d\nback_green %d\nback_blue %d\ntext_red %d\ntext_green %d\ntext_blue %d\nborder_red %d\nborder_green %d\nborder_blue %d\nborder_width %d\nfontname %s\n",
		confirm_destroy, sync_time, decorations,
		dwidth, dheight,
            	default_style.back.red, default_style.back.green, default_style.back.blue,
		default_style.text.red, default_style.text.green, default_style.text.blue,
		default_style.border.red, default_style.border.green, default_style.border.blue,
		default_style.border_width,
		default_style.fontname);

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

		if (isdigit (temp[0]))
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
	return fio_get_values_from_file (	filename, 
									"back_red", &starter->back.red,
									"back_green", &starter->back.green,
									"back_blue", &starter->back.blue,
									"text_red", &starter->text.red,
									"text_green", &starter->text.green,
									"text_blue", &starter->text.blue,
									"border_red", &starter->border.red,
									"border_green", &starter->border.green,
									"border_blue", &starter->border.blue,
									"border_width", &starter->border_width,
									"fontname", starter->fontname,
									NULL );
}


void fio_save_info_file (pad_node *pad)
{
        gchar info_file[MAX_FILE_SIZE + 1];
        gint x, y, height, width;
        gchar *content;
	gchar temp[MAX_FILENAME_SIZE + 1];
	pad_style *pstyle;
	GtkTextIter s, e;
	GtkTextBuffer *buf;

	if (verbosity >= 2) printf ("Saving pad [%s].\n", pad->infoname);

	pstyle = pad_get_style (pad);

        gtk_window_get_position (pad->window, &x, &y);
        gtk_window_get_size (pad->window, &width, &height);
        sprintf (info_file, "x %d\ny %d\nwidth %d\nheight %d\nback_red %d\nback_green %d\nback_blue %d\ntext_red %d\ntext_green %d\ntext_blue %d\nborder_red %d\nborder_green %d\nborder_blue %d\nborder_width %d\nfontname %s\n",
            	x, y, width, height, 
		pstyle->back.red, pstyle->back.green, pstyle->back.blue,
		pstyle->text.red, pstyle->text.green, pstyle->text.blue,
		pstyle->border.red, pstyle->border.green, pstyle->border.blue,
		pstyle->border_width,
		pstyle->fontname);

	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
        content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);

	sprintf (temp, "content %s\n", pad->contentname);
	strcat (info_file, temp);

        fio_set_file (pad->contentname, content);
        g_free (content);

	rewind (pad->file);
	x = fputs (info_file, pad->file);
	fflush(pad->file);

	g_free (pstyle);
}

/* save contents and locations of a pad */
void fio_save_pad (pad_node *pad)
{
	fio_save_info_file (pad);
}

/* save contents and locations of all pads */
void fio_save_pads ()
{
	pad_node *current = first_pad;

	while (current != NULL)
	{
		fio_save_pad (current);
		current = current->next;
	}
}

void fio_remove_file (gchar *filename)
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
gint fio_get_info_from_file (const gchar *filename, pad_info *info)
{
	gint fd;

	/* check if there is a previous lock on this file */
	fd = open (filename, O_RDONLY);

	if (fd == -1)
	{
		if (verbosity >= 1) printf ("Could not open file [%s] for reading.\n", filename);
		return 1;
	}

	/* if there is a lock, stop loading it */
	if (lockf (fd, F_TEST, 0) == -1)
	{
		if (verbosity >= 2) printf ("Ignoring [%s].\n", filename);
		return 1;
	}

	close (fd);

	if (verbosity >= 2) printf ("Loading [%s].\n", filename);

	fio_get_style_from_file (filename, &info->style);
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


void fio_load_pads ()
{
	gint counter = 0, opened = 0;
	glob_t globbuf;
	gchar pattern[MAX_FILENAME_SIZE + 1];
	pad_node *pad;
	pad_info info;

	/* make sure directory exists */
	mkdir (working_dir, 00777);

	strcpy (pattern, working_dir);
	strcat (pattern, "info-*");

	glob (pattern, GLOB_NOSORT, NULL, &globbuf);

	/* set up some sort of defaults for these.  if xpad works
	   right, these won't be used. */
	info.style = default_style;
	info.x = 0;
	info.y = 0;
	info.width = 260;
	info.height = 260;

	while (counter < globbuf.gl_pathc)
	{
		if (!fio_get_info_from_file (globbuf.gl_pathv[counter++], &info))
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

	globfree (&globbuf);
}


