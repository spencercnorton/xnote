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

#include "pref.h"
#include "fio.h"
#include "pad.h"

pad_style *backup_style;
gint backup_width, backup_height;
gboolean backup_decorations;
gint backup_sync;
gboolean backup_confirm_destroy;

void global_preferences_cancel (GtkWidget *button)
{
	decorations = backup_decorations;
	sync_time = backup_sync;
	dwidth = backup_width;
	dheight = backup_height;
	default_style = *backup_style;
	fio_save_defaults ();

	pad_set_decorations (decorations);

	gtk_widget_destroy (gtk_widget_get_toplevel(button));
}

void global_preferences_apply (GtkWidget *button)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(gtk_container_get_children (GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (gtk_widget_get_toplevel(button)))))->data);
	pad_style temp;
	gint width, height;
	gboolean decor, confirm;

	if (verbosity >= 2) printf ("Applying global preferences.\n");


	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (
	 gtk_bin_get_child (GTK_BIN (
	  gtk_notebook_get_nth_page (notebook, 0)
	 ))
	), &temp.back);


	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (
	 gtk_bin_get_child (GTK_BIN (
	  gtk_notebook_get_nth_page (notebook, 1)
	 ))
	), &temp.text);


	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (
	 gtk_container_get_children (GTK_CONTAINER (
	  gtk_bin_get_child (GTK_BIN (
	   gtk_notebook_get_nth_page (notebook, 2)
	  ))
	 ))->next->data
	), &temp.border);


	temp.border_width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (
					 gtk_container_get_children (GTK_CONTAINER (
					  gtk_container_get_children (GTK_CONTAINER (
					   gtk_bin_get_child (GTK_BIN (
					    gtk_notebook_get_nth_page (notebook, 2)
					   ))
					  ))->data
					 ))->next->data
					));

	strcpy (temp.fontname, gtk_font_selection_get_font_name (GTK_FONT_SELECTION (
					    gtk_bin_get_child (GTK_BIN (
					     gtk_notebook_get_nth_page (notebook, 3)
					    ))
					   ))
	);


	width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (
		 gtk_container_get_children (GTK_CONTAINER (
		  gtk_container_get_children (GTK_CONTAINER (
		   gtk_bin_get_child (GTK_BIN (
		    gtk_notebook_get_nth_page (notebook, 4)
		   ))
		  ))->data
		 ))->next->data
		));


	height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (
		  gtk_container_get_children (GTK_CONTAINER (
		   gtk_container_get_children (GTK_CONTAINER (
		    gtk_bin_get_child (GTK_BIN (
		     gtk_notebook_get_nth_page (notebook, 4)
		    ))
		   ))->next->data
		  ))->next->data
		 ));


	confirm = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
		 gtk_container_get_children (GTK_CONTAINER (
		  gtk_bin_get_child (GTK_BIN (
		   gtk_notebook_get_nth_page (notebook, 5)
		  ))
		 ))->data
		));


	decor = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
		 gtk_container_get_children (GTK_CONTAINER (
		  gtk_bin_get_child (GTK_BIN (
		   gtk_notebook_get_nth_page (notebook, 5)
		  ))
		 ))->next->data
		));


	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
	     gtk_container_get_children (GTK_CONTAINER (
	      gtk_container_get_children (GTK_CONTAINER (
	       gtk_bin_get_child (GTK_BIN (
	        gtk_notebook_get_nth_page (notebook, 5)
	       ))
	      ))->next->next->data
	     ))->data
	    )) == FALSE)

		sync_time = 0;
	else
		sync_time = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (
				     gtk_container_get_children (GTK_CONTAINER (
				      gtk_container_get_children (GTK_CONTAINER (
				       gtk_bin_get_child (GTK_BIN (
				        gtk_notebook_get_nth_page (notebook, 5)
				       ))
				      ))->next->next->data
				     ))->next->data
				    ));

	reset_sync ();
	
	pad_set_decorations (decor);

	decorations = decor;
	dwidth = width;
	dheight = height;
	confirm_destroy = confirm;
	default_style = temp;
	fio_save_defaults ();

	gtk_window_present (GTK_WINDOW(gtk_widget_get_toplevel (button)));
}

void global_preferences_ok (GtkWidget *button)
{
	global_preferences_apply (button);

	gtk_widget_destroy (gtk_widget_get_toplevel(button));
}

void global_preferences_checkbutton_autosave_toggled (GtkWidget *spinner)
{
	gtk_widget_set_sensitive (spinner, !GTK_WIDGET_IS_SENSITIVE (spinner));
}

void global_preferences_open (pad_node *pad)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *notebook = gtk_notebook_new ();
	GtkWidget *buttonbox = gtk_hbutton_box_new ();
	GtkWidget *button_ok = gtk_button_new_from_stock (GTK_STOCK_OK);
	GtkWidget *button_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	GtkWidget *button_apply = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	GtkWidget *label_back = gtk_label_new ("Default Background Color");
	GtkWidget *label_text = gtk_label_new ("Default Text Color");
	GtkWidget *label_border = gtk_label_new ("Default Border");
	GtkWidget *label_border_width = gtk_label_new ("Default Border Width:");
	GtkWidget *label_font = gtk_label_new ("Default Font");
	GtkWidget *label_size = gtk_label_new ("Default Size");
	GtkWidget *label_misc = gtk_label_new ("Miscellaneous");
	GtkWidget *label_misc_autosave = gtk_label_new ("seconds between saves");
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 0);
	GtkWidget *color_back = gtk_color_selection_new ();
	GtkWidget *color_text = gtk_color_selection_new ();
	GtkWidget *color_border = gtk_color_selection_new ();
	GtkWidget *font_selection = gtk_font_selection_new ();
	GtkWidget *vbox_size = gtk_vbox_new (FALSE, 20);
	GtkWidget *vbox_size_width = gtk_vbox_new (FALSE, 0);
	GtkWidget *vbox_size_height = gtk_vbox_new (FALSE, 0);
	GtkObject *adjust_size_width, *adjust_size_height;
	GtkWidget *spinner_size_width, *spinner_size_height;
	GtkWidget *label_size_width = gtk_label_new ("Width:");
	GtkWidget *label_size_height = gtk_label_new ("Height:");
	GtkWidget *vbox_misc = gtk_vbox_new (FALSE, 0);
	GtkWidget *checkbutton_decorations = gtk_check_button_new_with_label ("Allow WM Decorations");
	GtkWidget *checkbutton_autosave = gtk_check_button_new_with_label ("Autosave Pads");
	GtkWidget *checkbutton_confirm_destroy = gtk_check_button_new_with_label ("Confirm Pad Destructions");
	GtkWidget *vbox_border = gtk_vbox_new (FALSE, 0);
	GtkWidget *vbox_border_width = gtk_vbox_new (FALSE, 0);
	GtkWidget *vbox_autosave = gtk_vbox_new (FALSE, 0);
	GtkObject *adjust_border_width;
	GtkWidget *spinner_border_width;
	GtkObject *adjust_misc_autosave;
	GtkWidget *spinner_misc_autosave;
	GtkWidget *align_0 = gtk_alignment_new (0.5, 0.5, 0, 0);
	GtkWidget *align_1 = gtk_alignment_new (0.5, 0.5, 0, 0);
	GtkWidget *align_2 = gtk_alignment_new (0.5, 0.5, 0, 0);
	GtkWidget *align_3 = gtk_alignment_new (0.5, 0.5, 0, 0);
	GtkWidget *align_4 = gtk_alignment_new (0.5, 0.5, 0, 0);
	GtkWidget *align_5 = gtk_alignment_new (0.5, 0.5, 0, 0);

	backup_style = (pad_style *) g_malloc (sizeof (pad_style));
	*backup_style = default_style;
	backup_width = dwidth;
	backup_height = dheight;
	backup_decorations = decorations;
	backup_confirm_destroy = confirm_destroy;
	backup_sync = sync_time;

	gtk_window_set_transient_for (GTK_WINDOW(window), pad->window);
	gtk_window_set_modal (GTK_WINDOW(window), TRUE);
	gtk_container_add (GTK_CONTAINER(window), vbox_global);

	// buttonbox setup
	g_signal_connect (GTK_OBJECT (button_ok), "clicked", 
		G_CALLBACK (global_preferences_ok), (gpointer) pad);
	g_signal_connect (GTK_OBJECT (button_ok), "activate", 
		G_CALLBACK (global_preferences_ok), (gpointer) pad);
	g_signal_connect (GTK_OBJECT (button_cancel), "clicked", 
		G_CALLBACK (global_preferences_cancel), (gpointer) pad);
	g_signal_connect (GTK_OBJECT (button_apply), "clicked", 
		G_CALLBACK (global_preferences_apply), (gpointer) pad);
	g_signal_connect_swapped (GTK_OBJECT (window), "destroy", 
                             G_CALLBACK (g_free), backup_style);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(buttonbox), 10);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_apply);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_ok);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_cancel);
	gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 10);

	// vbox_global setup
	gtk_box_pack_start (GTK_BOX(vbox_global), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox_global), buttonbox, TRUE, TRUE, 0);

	// back setup
	gtk_container_add (GTK_CONTAINER (align_0), color_back);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_0, label_back);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_back), &backup_style->back);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_back), FALSE);

	// text setup
	gtk_container_add (GTK_CONTAINER (align_1), color_text);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_1, label_text);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_text), &backup_style->text);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_text), FALSE);

	// border setup
	gtk_container_add (GTK_CONTAINER (align_2), vbox_border);
	adjust_border_width = gtk_adjustment_new (backup_style->border_width, 0.0, 100.0, 1.0, 5.0, 5.0);
	spinner_border_width = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_border_width), 1.0, 0);
	gtk_box_pack_start_defaults (GTK_BOX (vbox_border_width), label_border_width);
	gtk_box_pack_start_defaults (GTK_BOX (vbox_border_width), spinner_border_width);
	gtk_box_pack_start (GTK_BOX (vbox_border), vbox_border_width, FALSE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX (vbox_border), color_border, FALSE, FALSE, 20);
	gtk_misc_set_alignment (GTK_MISC (label_border_width), 0, 1);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_2, label_border);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_border), &backup_style->border);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_border), FALSE);
	gtk_entry_set_activates_default (GTK_ENTRY (spinner_border_width), TRUE);

	// font setup
	gtk_container_add (GTK_CONTAINER (align_3), font_selection);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_3, label_font);
	gtk_font_selection_set_font_name (GTK_FONT_SELECTION (font_selection), backup_style->fontname);

	// size setup
	adjust_size_width = gtk_adjustment_new (backup_width, 1.0, INT_MAX, 1.0, 5.0, 5.0);
	spinner_size_width = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_size_width), 1.0, 0);
	adjust_size_height = gtk_adjustment_new (backup_height, 1.0, INT_MAX, 1.0, 5.0, 5.0);
	spinner_size_height = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_size_height), 1.0, 0);
	gtk_entry_set_width_chars (GTK_ENTRY (spinner_size_width), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (spinner_size_height), 4);
	gtk_entry_set_activates_default (GTK_ENTRY (spinner_size_width), TRUE);
	gtk_entry_set_activates_default (GTK_ENTRY (spinner_size_height), TRUE);

	// vbox_size setup
	gtk_container_add (GTK_CONTAINER (align_4), vbox_size);
	gtk_misc_set_alignment (GTK_MISC (label_size_width), 0, 1);
	gtk_misc_set_alignment (GTK_MISC (label_size_height), 0, 1);
	gtk_box_pack_start_defaults (GTK_BOX(vbox_size_width), label_size_width);
	gtk_box_pack_start_defaults (GTK_BOX(vbox_size_width), spinner_size_width);
	gtk_box_pack_start_defaults (GTK_BOX(vbox_size_height), label_size_height);
	gtk_box_pack_start_defaults (GTK_BOX(vbox_size_height), spinner_size_height);
	gtk_box_pack_start_defaults (GTK_BOX(vbox_size), vbox_size_width);
	gtk_box_pack_start_defaults (GTK_BOX(vbox_size), vbox_size_height);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_4, label_size);

	// misc. setup
	gtk_container_add (GTK_CONTAINER (align_5), vbox_misc);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_decorations), backup_decorations);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_autosave), backup_sync);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_confirm_destroy), backup_confirm_destroy);
	adjust_misc_autosave = gtk_adjustment_new (sync_time ? sync_time : 1, 1.0, INT_MAX, 1.0, 5.0, 5.0);
	spinner_misc_autosave = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_misc_autosave), 1.0, 0);
	gtk_entry_set_width_chars (GTK_ENTRY (spinner_misc_autosave), 4);
	gtk_widget_set_sensitive (spinner_misc_autosave, backup_sync);
	gtk_box_pack_start_defaults (GTK_BOX (vbox_autosave), checkbutton_autosave);
	gtk_box_pack_start_defaults (GTK_BOX (vbox_autosave), spinner_misc_autosave);
	gtk_box_pack_start_defaults (GTK_BOX (vbox_autosave), label_misc_autosave);
	gtk_box_pack_start (GTK_BOX(vbox_misc), checkbutton_confirm_destroy, FALSE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX(vbox_misc), checkbutton_decorations, FALSE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX(vbox_misc), vbox_autosave, FALSE, FALSE, 20);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_5, label_misc);
	gtk_signal_connect_object (GTK_OBJECT (checkbutton_autosave), "toggled", 
		GTK_SIGNAL_FUNC (global_preferences_checkbutton_autosave_toggled), (gpointer) spinner_misc_autosave);
	gtk_entry_set_activates_default (GTK_ENTRY (spinner_misc_autosave), TRUE);

	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	gtk_widget_grab_focus (button_ok);
	GTK_WIDGET_SET_FLAGS (button_ok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button_ok);

	gtk_widget_show_all (window);
}




// must g_free returned style
pad_style *pad_preferences_get_style (GtkWindow *window)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(gtk_container_get_children (GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (window))))->data);
	pad_style *temp = (pad_style *) g_malloc (sizeof (pad_style));

	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (
	 gtk_bin_get_child (GTK_BIN (
	  gtk_notebook_get_nth_page (notebook, 0)
	 ))
	), &temp->back);


	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (
	 gtk_bin_get_child (GTK_BIN (
	  gtk_notebook_get_nth_page (notebook, 1)
	 ))
	), &temp->text);


	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (
	 gtk_container_get_children (GTK_CONTAINER (
	  gtk_bin_get_child (GTK_BIN (
	   gtk_notebook_get_nth_page (notebook, 2)
	  ))
	 ))->next->data
	), &temp->border);


	temp->border_width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (
					  gtk_container_get_children (GTK_CONTAINER (
					   gtk_container_get_children (GTK_CONTAINER (
					    gtk_bin_get_child (GTK_BIN (
					     gtk_notebook_get_nth_page (notebook, 2)
					    ))
					   ))->data
					  ))->next->data
					 ));

	strcpy (temp->fontname, gtk_font_selection_get_font_name (GTK_FONT_SELECTION (
					     gtk_bin_get_child (GTK_BIN (
					      gtk_notebook_get_nth_page (notebook, 3)
					     ))
					    ))
	);

	return temp;
}


void pad_preferences_cancel (GtkWidget *button, pad_node *pad)
{
	pad_set_style (pad, backup_style);
	gtk_widget_destroy (gtk_widget_get_toplevel(button));
}

void pad_preferences_apply (GtkWidget *button, pad_node *pad)
{
	pad_style *temp;

	if (verbosity >= 2) printf ("Applying local pad preferences [%s].\n", pad->infoname);

	temp = pad_preferences_get_style (GTK_WINDOW (gtk_widget_get_toplevel (button)));

	pad_set_style (pad, temp);

	fio_save_pad (pad);

	g_free (temp);
}

void pad_preferences_ok (GtkWidget *button, pad_node *pad)
{
	pad_preferences_apply (button, pad);
	gtk_widget_destroy (gtk_widget_get_toplevel(button));
}

void pad_preferences_save_as_default (GtkWidget *button, pad_node *pad)
{
	pad_style *temp;

	temp = pad_preferences_get_style (GTK_WINDOW (gtk_widget_get_toplevel (button)));

	default_style = *temp;
	fio_save_defaults ();

	g_free (temp);
}

void pad_preferences_open (pad_node *pad)
{
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *notebook = gtk_notebook_new ();
	GtkWidget *buttonbox = gtk_hbutton_box_new ();
	GtkWidget *button_ok = gtk_button_new_from_stock (GTK_STOCK_OK);
	GtkWidget *button_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	GtkWidget *button_apply = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	GtkWidget *button_save_default = gtk_button_new_with_mnemonic ("_Set as Default");
	GtkWidget *label_back = gtk_label_new ("Background Color");
	GtkWidget *label_text = gtk_label_new ("Text Color");
	GtkWidget *label_border = gtk_label_new ("Border");
	GtkWidget *label_font = gtk_label_new ("Font");
	GtkWidget *label_border_width = gtk_label_new ("Border Width:");
	GtkWidget *vbox_global = gtk_vbox_new (FALSE, 0);
	GtkWidget *color_back = gtk_color_selection_new ();
	GtkWidget *color_text = gtk_color_selection_new ();
	GtkWidget *color_border = gtk_color_selection_new ();
	GtkWidget *font_selection = gtk_font_selection_new ();
	GtkWidget *vbox_border = gtk_vbox_new (FALSE, 0);
	GtkWidget *vbox_border_width = gtk_vbox_new (FALSE, 0);
	GtkObject *adjust_border_width;
	GtkWidget *spinner_border_width;
	GtkWidget *align_0 = gtk_alignment_new (0.5, 0.5, 0, 0);
	GtkWidget *align_1 = gtk_alignment_new (0.5, 0.5, 0, 0);
	GtkWidget *align_2 = gtk_alignment_new (0.5, 0.5, 0, 0);
	GtkWidget *align_3 = gtk_alignment_new (0.5, 0.5, 0, 0);

	backup_style = pad_get_style (pad);

	gtk_window_set_transient_for (GTK_WINDOW(window), pad->window);
	gtk_window_set_modal (GTK_WINDOW(window), TRUE);
	gtk_container_add (GTK_CONTAINER(window), vbox_global);

	// vbox_global setup
	gtk_box_pack_start (GTK_BOX(vbox_global), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox_global), buttonbox, TRUE, TRUE, 0);

	// buttonbox setup
	g_signal_connect (GTK_OBJECT (button_ok), "clicked", 
		G_CALLBACK (pad_preferences_ok), (gpointer) pad);
	g_signal_connect (GTK_OBJECT (button_ok), "activate", 
		G_CALLBACK (pad_preferences_ok), (gpointer) pad);
	g_signal_connect (GTK_OBJECT (button_cancel), "clicked", 
		G_CALLBACK (pad_preferences_cancel), (gpointer) pad);
	g_signal_connect (GTK_OBJECT (button_apply), "clicked", 
		G_CALLBACK (pad_preferences_apply), (gpointer) pad);
	g_signal_connect (GTK_OBJECT (button_save_default), "clicked", 
		G_CALLBACK (pad_preferences_save_as_default), (gpointer) pad);
	g_signal_connect_swapped (GTK_OBJECT (window), "destroy", 
                             G_CALLBACK (g_free), backup_style);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(buttonbox), 10);
	gtk_box_pack_start_defaults (GTK_BOX(buttonbox), button_save_default);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_apply);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_ok);
	gtk_box_pack_end_defaults (GTK_BOX(buttonbox), button_cancel);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (buttonbox), button_save_default, TRUE);
	GTK_WIDGET_SET_FLAGS (button_ok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button_ok);
	gtk_widget_grab_focus (button_ok);
	gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 10);

	// back setup
	gtk_container_add (GTK_CONTAINER (align_0), color_back);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_0, label_back);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_back), &backup_style->back);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_back), FALSE);

	// text setup
	gtk_container_add (GTK_CONTAINER (align_1), color_text);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_1, label_text);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_text), &backup_style->text);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_text), FALSE);

	// border setup
	gtk_container_add (GTK_CONTAINER (align_2), vbox_border);
	adjust_border_width = gtk_adjustment_new (backup_style->border_width, 0.0, 100.0, 1.0, 5.0, 5.0);
	spinner_border_width = gtk_spin_button_new (GTK_ADJUSTMENT(adjust_border_width), 1.0, 0);
	gtk_box_pack_start_defaults (GTK_BOX (vbox_border_width), label_border_width);
	gtk_box_pack_start_defaults (GTK_BOX (vbox_border_width), spinner_border_width);
	gtk_box_pack_start (GTK_BOX (vbox_border), vbox_border_width, FALSE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX (vbox_border), color_border, FALSE, FALSE, 20);
	gtk_misc_set_alignment (GTK_MISC (label_border_width), 0, 1);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_2, label_border);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (color_border), &backup_style->border);
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (color_border), FALSE);
	gtk_entry_set_activates_default (GTK_ENTRY (spinner_border_width), TRUE);

	// font setup
	gtk_container_add (GTK_CONTAINER (align_3), font_selection);
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), align_3, label_font);
	gtk_font_selection_set_font_name (GTK_FONT_SELECTION (font_selection), backup_style->fontname);

	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	gtk_widget_show_all (window);
}

