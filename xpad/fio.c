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

const gchar *TEMP_PAD_INFO_PREFIX = "temp-info";
const gchar *TEMP_PAD_CONTENT_PREFIX = "content";
const gchar *PAD_INFO_PREFIX = "info";
const gchar *PAD_CONTENT_PREFIX = "content";

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
        gchar temp[1024];

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
	gchar temp[1024];
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

void set_default_style (pad_style *style)
{
	gchar buf[MAX_FILE_SIZE];

	sprintf (buf, "back_red %d\nback_green %d\nback_blue %d\ntext_red %d\ntext_green %d\ntext_blue %d\nfontname %s\n",
            	style->back.red, style->back.green, style->back.blue,
		style->text.red, style->text.green, style->text.blue,
		style->fontname);

	set_file ("default-style", buf);
}

pad_style get_default_style ()
{
	gchar buf[MAX_FILE_SIZE];
	gchar *item, *value;
	pad_style *style = (pad_style *) g_malloc (sizeof (pad_style));
	
	if (get_file ("default-style", buf) > 0)
	{
		return DEFAULT_INFO.style;
	}

	item = strtok (buf, " ");
	value = strtok (NULL, "\n");

	while (item != NULL)
        {
		if (strcmp (item, "back_red") == 0)
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
	
	return *style;
}

void save_info_file (pad_node *pad, gchar *infoname, gchar *contentname)
{
        gchar info_file[MAX_FILE_SIZE] = "";
        gint x, y, height, width;
        gchar *content;
	gchar temp[1024] = "";
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

	sprintf (temp, "content %s\n", contentname);
	strcat (info_file, temp);

        set_file (contentname, content);
        g_free (content);

	set_file (infoname, info_file);
}

/* save contents and locations of a pad */
void save_pad (pad_node *pad)
{
	gint pid = getpid();
	gchar infoname[256], contentname[256];

	sprintf (infoname, "%s-%d-%d", TEMP_PAD_INFO_PREFIX, pid, pad->num);
	sprintf (contentname, "%s-%d-%d", TEMP_PAD_CONTENT_PREFIX, pid, pad->num);

	save_info_file (pad, infoname, contentname);
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

void remove_pad_files (pad_node *pad)
{
	gint pid = getpid();
	gchar infoname[256], contentname[256];

	sprintf (infoname, "%s-%d-%d", TEMP_PAD_INFO_PREFIX, pid, pad->num);
	sprintf (contentname, "%s-%d-%d", TEMP_PAD_CONTENT_PREFIX, pid, pad->num);

	remove (infoname);
	remove (contentname);

}

void commit_pads ()
{
	gint pid = getpid();
	gchar infoname[256], contentname[256];
	gchar tempinfoname[256], tempcontentname[256];
	pad_node *current = first_pad;

	while (current != NULL)
	{
		sprintf (infoname, "%s%s-%d-%d", working_dir, PAD_INFO_PREFIX, pid, current->num);
		sprintf (contentname, "%s%s-%d-%d", working_dir, PAD_CONTENT_PREFIX, pid, current->num);
		sprintf (tempinfoname, "%s%s-%d-%d", working_dir, TEMP_PAD_INFO_PREFIX, pid, current->num);
		sprintf (tempcontentname, "%s%s-%d-%d", working_dir, TEMP_PAD_CONTENT_PREFIX, pid, current->num);

		if (strcmp (tempinfoname, infoname) != 0)
			rename (tempinfoname, infoname);
		if (strcmp (tempcontentname, contentname) != 0)
			rename (tempcontentname, contentname);

		current = current->next;
	}
}

/* removes a file, within working_dir */
void remove_file (gchar *filename)
{
	gchar long_filename[1024];

	if (filename[0] == '/')
		strcpy (long_filename, filename);
	else
		sprintf (long_filename, "%s%s", working_dir, filename);

	remove (long_filename);
}

pad_node *load_info_file (gchar *filename)
{
        gchar info_file[MAX_FILE_SIZE];
	pad_info info;
	gchar *item, *value;
	gint retvalue;
	pad_node *pad;

	if ( (retvalue = get_file (filename, info_file)) > 0)
		return NULL;

	/* get rid of file, now that we've read it. */
	remove_file (filename);

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
                        info.width = atoi (value);
                else if (strcmp (item, "height") == 0)
                        info.height = atoi (value);
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
			remove_file (value);
		}

		item = strtok (NULL, " ");
		value = strtok (NULL, "\n");
	}

	pad = create_pad_with_info (&info);

	return pad;
}


void load_pads ()
{
	gint counter = 0;
	glob_t globbuf;
	gchar pattern[1024];
	pad_node *pad;

	/* make all directory exists */
	mkdir (working_dir, 00777);

	strcpy (pattern, working_dir);
	strcat (pattern, "info*");

	glob (pattern, GLOB_NOSORT, NULL, &globbuf);

	while (counter < globbuf.gl_pathc)
	{
		pad = load_info_file (globbuf.gl_pathv[counter++]);
		save_pad (pad);
	}

	if (counter == 0)
		create_pad ();

	globfree (&globbuf);
}



















