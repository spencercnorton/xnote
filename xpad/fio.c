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

#include "fio.h"
#include "pad.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glob.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>

/* helper func to get textbox from window */
GtkTextView *get_text (GtkWindow *window)
{
	GList *kids = gtk_container_children (GTK_CONTAINER (window));
	
	do
	{
		if (strcmp (gtk_widget_get_name (kids->data), "GtkTextView") == 0)
			return kids->data;

		kids = kids->next;
	} while (kids != NULL);
	
	return NULL;
}

int set_file (const char *name, const char *value)
{
        FILE *file;
        gchar temp[MAX_FILENAME_SIZE];

	if (name[0] == '/')
		strcpy (temp, name);
	else
	{
	        strcpy (temp, working_dir);
        	strcat (temp, name);
	}

        if ( (file = fopen (temp, "w")) == NULL)
        {
                fprintf (stderr, "Cannot open %s.\n", temp);
                return 1;
        }
        
        fputs (value, file);

        fclose (file);

	return 0;
}


int get_file (const char *name, char *value)
{
	gchar temp[MAX_FILENAME_SIZE];
        gchar c[2];
	FILE *file;
        
	if (name[0] == '/')
		strcpy (temp, name);
	else
	{
	        strcpy (temp, working_dir);
	        strcat (temp, name);
	}
	
	strcpy (value, "");

	/* check if file exists */
        if ( (file = fopen (temp, "r")) == NULL)
        {
		return 1;
	}
	else
        {
	        c[1] = '\0';
	
	        while ( (c[0] = fgetc(file)) != EOF )
	                strcat (value, c);
	}
	
	fclose (file);
	
	return 0;
}

void open_pad_files (pad_node *pad, gboolean create)
{
	struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, getpid()};

	if (create == TRUE)
	{
		strcpy (pad->infoname, working_dir);
		strcat (pad->infoname, "info-XXXXXX");
		mkstemp (pad->infoname);
	
		strcpy (pad->contentname, working_dir);
		strcat (pad->contentname, "content-XXXXXX");
		mkstemp (pad->contentname);
	}

	if ( (pad->file = fopen (pad->infoname, "w")) == NULL)
        {
                fprintf (stderr, "Cannot open %s.\n", pad->infoname);
                return;
        }

	fcntl (fileno(pad->file), F_SETLK, &fl);
}

void close_pad_files (pad_node *pad)
{
	struct flock fl = {F_UNLCK, SEEK_SET, 0, 0, getpid()};
	fcntl (fileno(pad->file), F_SETLK, &fl);
	fclose (pad->file);
}


void set_default_style_from_pad (pad_node *pad)
{
	GtkStyle *style;
	pad_style pstyle;

	style = gtk_widget_get_style (GTK_WIDGET(get_text(pad->window)));

	gdk_window_get_size (GTK_WIDGET(pad->window)->window, &pstyle.width, &pstyle.height);
	pstyle.text = style->text[GTK_STATE_NORMAL];
	pstyle.back = style->base[GTK_STATE_NORMAL];
	strcpy (pstyle.fontname, pango_font_description_to_string (style->font_desc));

	set_default_style (&pstyle);
}

void set_default_style (pad_style *style)
{
	gchar buf[MAX_FILE_SIZE];

	sprintf (buf, "width %d\nheight %d\nback_red %d\nback_green %d\nback_blue %d\ntext_red %d\ntext_green %d\ntext_blue %d\nfontname %s\n",
		style->width, style->height,
            	style->back.red, style->back.green, style->back.blue,
		style->text.red, style->text.green, style->text.blue,
		style->fontname);

	set_file ("default-style", buf);
}

pad_style *get_default_style ()
{
	gchar buf[MAX_FILE_SIZE];
	gchar *item, *value;
	pad_style *style;
	
	if (get_file ("default-style", buf) > 0)
	{
		return &DEFAULT_INFO.style;
	}

	style = (pad_style *) g_malloc (sizeof (pad_style));

	item = strtok (buf, " ");
	value = strtok (NULL, "\n");

	while (item != NULL)
        {
		if (strcmp (item, "width") == 0)
			style->width = atoi (value);
		else if (strcmp (item, "height") == 0)
			style->height = atoi (value);
		else if (strcmp (item, "back_red") == 0)
			style->back.red = atoi (value);
		else if (strcmp (item, "back_green") == 0)
			style->back.green = atoi (value);
		else if (strcmp (item, "back_blue") == 0)
			style->back.blue = atoi (value);
		else if (strcmp (item, "text_red") == 0)
			style->text.red = atoi (value);
		else if (strcmp (item, "text_green") == 0)
			style->text.green = atoi (value);
		else if (strcmp (item, "text_blue") == 0)
			style->text.blue = atoi (value);
		else if (strcmp (item, "fontname") == 0)
			strcpy(style->fontname, value);

		item = strtok (NULL, " ");
		value = strtok (NULL, "\n");
	}
	
	return style;
}

void save_info_file (pad_node *pad)
{
        gchar info_file[MAX_FILE_SIZE] = "";
        gint x, y, height, width;
        gchar *content;
	gchar temp[MAX_FILENAME_SIZE] = "";
	GdkColor back, text;
	GtkTextIter s, e;	
	GtkTextBuffer *buf;
	GtkStyle *style;

	style = gtk_widget_get_style (GTK_WIDGET (get_text(GTK_WINDOW(pad->window))));

	back = style->base[GTK_STATE_NORMAL];
	text = style->text[GTK_STATE_NORMAL]; 

        gdk_window_get_origin (GTK_WIDGET(pad->window)->window, &x, &y);
        gdk_window_get_size (GTK_WIDGET(pad->window)->window, &width, &height);
        sprintf (info_file, "x %d\ny %d\nwidth %d\nheight %d\nback_red %d\nback_green %d\nback_blue %d\ntext_red %d\ntext_green %d\ntext_blue %d\nfontname %s\n",
            	x, y, width, height, back.red, back.green, back.blue, text.red, text.green, text.blue, pad->fontname);

	buf = gtk_text_view_get_buffer (get_text(GTK_WINDOW(pad->window)));
	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
        content = gtk_text_buffer_get_text (buf, &s, &e, FALSE);

	sprintf (temp, "content %s\n", pad->contentname);
	strcat (info_file, temp);

        set_file (pad->contentname, content);
        g_free (content);

	rewind (pad->file);
	x = fputs (info_file, pad->file);
	fflush(pad->file);
}

/* save contents and locations of a pad */
void save_pad (pad_node *pad)
{
	save_info_file (pad);
}

/* save contents and locations of all pads */
void save_pads ()
{
	pad_node *current = first_pad;

	while (current != NULL)
	{
		save_pad (current);
		current = current->next;
	}
}

/* removes a file, within working_dir */
void remove_file (gchar *filename)
{
	gchar long_filename[MAX_FILENAME_SIZE];

	if (filename[0] == '/')
		strcpy (long_filename, filename);
	else
		sprintf (long_filename, "%s%s", working_dir, filename);

	remove (long_filename);
}

void remove_pad_files (pad_node *pad)
{
	remove_file (pad->infoname);
	remove_file (pad->contentname);
}

/* filename must be absolute */
pad_node *load_info_file (gchar *filename)
{
        gchar info_file[MAX_FILE_SIZE];
	pad_info info;
	gchar *item, *value;
	gint retvalue;
	pad_node *pad;
	FILE *file;
	struct flock lock;

	if ( (retvalue = get_file (filename, info_file)) > 0)
		return NULL;

	/* check if there is a previous lock on this file */
	file = fopen (filename, "r");
	fcntl (fileno (file), F_GETLK, &lock);
	fclose (file);

	/* there is a lock, so stop loading it */
	if (lock.l_type != F_UNLCK)
		return NULL;

	info = DEFAULT_INFO;

	item = strtok (info_file, " ");
	value = strtok (NULL, "\n");
        
	while (item != NULL)
        {
		if (strcmp (item, "x") == 0)
                	info.x = atoi (value);   
                else if (strcmp (item, "y") == 0)
                        info.y = atoi (value);
                else if (strcmp (item, "width") == 0)
                        info.style.width = atoi (value);
                else if (strcmp (item, "height") == 0)
                        info.style.height = atoi (value);
		else if (strcmp (item, "back_red") == 0)
			info.style.back.red = atoi (value);
		else if (strcmp (item, "back_green") == 0)
			info.style.back.green = atoi (value);
		else if (strcmp (item, "back_blue") == 0)
			info.style.back.blue = atoi (value);
		else if (strcmp (item, "text_red") == 0)
			info.style.text.red = atoi (value);
		else if (strcmp (item, "text_green") == 0)
			info.style.text.green = atoi (value);
		else if (strcmp (item, "text_blue") == 0)
			info.style.text.blue = atoi (value);
		else if (strcmp (item, "fontname") == 0)
			strcpy(info.style.fontname, value);
                else if (strcmp (item, "content") == 0)
		{
                        get_file (value, info.content);
			strcpy (info.contentname, value);
		}

		item = strtok (NULL, " ");
		value = strtok (NULL, "\n");
	}

	strcpy (info.infoname, filename);

	pad = create_pad_with_info (&info);

	return pad;
}


void load_pads ()
{
	gint counter = 0, opened = 0;
	glob_t globbuf;
	gchar pattern[MAX_FILENAME_SIZE];
	pad_node *pad;

	/* make all directory exists */
	mkdir (working_dir, 00777);

	strcpy (pattern, working_dir);
	strcat (pattern, "info-*");

	glob (pattern, GLOB_NOSORT, NULL, &globbuf);

	while (counter < globbuf.gl_pathc)
	{
		pad = load_info_file (globbuf.gl_pathv[counter++]);
		
		if (pad != NULL)
		{
			opened ++;
			save_pad (pad);
		}
	}

	if (opened == 0)
		create_pad ();

	globfree (&globbuf);
}
























