/*

Copyright (c) 2002 Jamis Buck

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

#include <gtk/gtk.h>
#include "main.h"
#include "tray.h"
#include "eggtrayicon.h"
#include "../images/xpad.xpm"
#include "pad.h"
#include "help.h"
#include "pref.h"
#include "fio.h"

#define PREFERENCES      1
#define HELP_CONT        2
#define HELP_ABT         3
#define QUIT_ITEM        4

#define SHOWN            1
#define HIDDEN           2

/* much of this code is shamelessly copied from docklet.c, in the gaim/plugins/docklet
 * directory... since it was one of the only good examples that I could find for using
 * the system tray API. */

static void  docklet_menu( GdkEventButton *event );
static void  docklet_clicked( GtkWidget *button, GdkEventButton *event, void *data );
static void  docklet_create( void );
static void  docklet_remove( void );
static void  docklet_toggle( void );

static EggTrayIcon    *docklet = NULL;
static GtkWidget      *icon = NULL;
static GtkWidget      *menu = NULL;
static int             toggle_state = SHOWN;


void tray_open (void)
{
  docklet_create();
}


void tray_close (void)
{
  if (docklet && GTK_WIDGET_VISIBLE(docklet)) {
    docklet_remove();
  }
  if( menu )
  {
    gtk_widget_destroy( menu );
  }
}


void tray_toggle (void)
{
  docklet_toggle();
}

static void docklet_menu( GdkEventButton *event )
{
  GtkWidget *entry;
  GtkWidget *windows_menu;
  pad_node  *pad;
  int        n;

  if( menu )
  {
    gtk_widget_destroy( menu );
  }

  windows_menu = gtk_menu_new();

  pad = first_pad;
  n = 0;
  while( pad )
  {
    gchar *result;
    gchar *title;

    n++;
    title = g_strdup (pad->title);
    str_replace_tokens (&title, '_', "__");
    result = g_strdup_printf ("_%d. %s", n, title);
    g_free (title);

    entry = gtk_menu_item_new_with_mnemonic (result);
    g_signal_connect_swapped (G_OBJECT(entry), "activate", 
    	G_CALLBACK (pad_show), pad);
    gtk_menu_shell_append (GTK_MENU_SHELL(windows_menu), entry);

    g_free (result);
    pad = pad->next;
  }

  menu = gtk_menu_new();

  entry = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
  g_signal_connect (G_OBJECT(entry), "activate", G_CALLBACK(pad_new), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), entry);
  
  entry = gtk_separator_menu_item_new();
  gtk_menu_shell_append( GTK_MENU_SHELL(menu), entry );
  
  entry = gtk_menu_item_new_with_mnemonic (_("No_tes"));
  gtk_menu_item_set_submenu( GTK_MENU_ITEM(entry), windows_menu );
  gtk_menu_shell_append( GTK_MENU_SHELL(menu), entry );
  
  entry = gtk_menu_item_new_with_mnemonic (_("_Show All"));
  g_signal_connect( G_OBJECT(entry), "activate", G_CALLBACK (pads_show_all), NULL );
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), entry);
  
  entry = gtk_menu_item_new_with_mnemonic (_("_Hide All"));
  g_signal_connect (G_OBJECT(entry), "activate", G_CALLBACK (pads_hide_all), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), entry);
  
  entry = gtk_separator_menu_item_new();
  gtk_menu_shell_append( GTK_MENU_SHELL(menu), entry );

  entry = gtk_image_menu_item_new_from_stock( GTK_STOCK_PREFERENCES, NULL );
  g_signal_connect( G_OBJECT(entry), "activate", G_CALLBACK(preferences_open), NULL );
  gtk_menu_shell_append( GTK_MENU_SHELL(menu), entry );

  entry = gtk_separator_menu_item_new();
  gtk_menu_shell_append( GTK_MENU_SHELL(menu), entry );

  entry = gtk_image_menu_item_new_from_stock( GTK_STOCK_QUIT, NULL );
  g_signal_connect( G_OBJECT(entry), "activate", G_CALLBACK(pads_close_all), NULL );
  gtk_menu_shell_append( GTK_MENU_SHELL(menu), entry );
  gtk_widget_show_all( menu );
  gtk_menu_popup( GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time );
}


static void docklet_clicked( GtkWidget *button, GdkEventButton *event, void *data )
{
	if (event->type != GDK_BUTTON_PRESS)
		return;

	switch (event->button) {
		case 1:
      docklet_toggle();
			break;
		case 2:
			break;
		case 3:
			docklet_menu(event);
			break;
	}
}


static void docklet_create( void )
{
  GtkWidget *box;
  GdkPixbuf *unscaled;
  GdkPixbuf *scaled;
  GtkTooltips *docklet_tips;

  unscaled = gdk_pixbuf_new_from_xpm_data( xpad_xpm );
  if( !unscaled ) return;

  docklet_remove();

  toggle_state = SHOWN;
  docklet = egg_tray_icon_new ("xpad");
  box = gtk_event_box_new();
  icon = gtk_image_new();

  g_signal_connect( G_OBJECT(box), "button-press-event", G_CALLBACK(docklet_clicked), NULL  );

  gtk_container_add( GTK_CONTAINER(box), icon );
  gtk_container_add( GTK_CONTAINER(docklet), box );
  gtk_widget_show_all( GTK_WIDGET(docklet) );

  g_object_ref( G_OBJECT(docklet) );

  scaled = gdk_pixbuf_scale_simple( unscaled, 24, 24, GDK_INTERP_BILINEAR );
  gtk_image_set_from_pixbuf( GTK_IMAGE(icon), scaled );
  g_object_unref( unscaled );
  g_object_unref( scaled );

  docklet_tips = gtk_tooltips_new();
  gtk_tooltips_set_tip( GTK_TOOLTIPS(docklet_tips), box,
                        _("xpad: right click for more options..."),
                        _("Right click this icon for a menu of options pertaining to XPad. "
                        "Left click it to toggle whether or not the pads are displayed.") );
}

static void docklet_remove( void )
{
  if( docklet )
  {
    g_object_unref( G_OBJECT(docklet) );
    docklet = NULL;
  }
}

static void docklet_toggle( void )
{
  if( toggle_state == SHOWN )
  {
    toggle_state = HIDDEN;
    pads_hide_all();
  }
  else
  {
    toggle_state = SHOWN;
    pads_unhide_all();
  }
}


