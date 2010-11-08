/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2010 Sergei Riaguzov

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
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
#include <stdlib.h>
#include <glib.h>
#include <glib/glist.h>
#include "xpad-undo.h"
#include "xpad-text-buffer.h"

G_DEFINE_TYPE(XpadUndo, xpad_undo, G_TYPE_OBJECT)
#define XPAD_UNDO_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_UNDO, XpadUndoPrivate))

static GObject* xpad_undo_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties);
static void xpad_undo_dispose (GObject *object);
static void xpad_undo_finalize (GObject *object);
static void xpad_undo_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_undo_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

enum UserActionType
{
	USER_ACTION_INSERT_TEXT,
	USER_ACTION_DELETE_RANGE
};

typedef struct
{
	enum UserActionType action_type;
	gint start;
	gint end;
	gchar *text;
	gint len_in_bytes;
	gint n_utf8_chars;
	gboolean merged;
} UserAction;

static GList* xpad_undo_remove_action_elem (GList *curr);
static void xpad_undo_clear_redo_history (XpadUndo *undo);
static void xpad_undo_clear_history (XpadUndo *undo);
static void xpad_undo_begin_user_action (GtkTextBuffer *buffer, XpadUndo *undo);
static void xpad_undo_end_user_action (GtkTextBuffer *buffer, XpadUndo *undo);
static void xpad_undo_insert_text (GtkTextBuffer *buffer, GtkTextIter *location, gchar *text, gint len, XpadUndo *undo);
static void xpad_undo_delete_range (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end, XpadUndo *undo);

struct XpadUndoPrivate
{
	XpadTextBuffer *buffer;
	/* We always redo the next element in the list but undo the current one.
	   We insert this guard with NULL data in the beginning to ease coding all of this */
	GList *history_start;
	GList *history_curr;
	guint user_action;
	gboolean frozen;
};

enum
{
	PROP_0,
	PROP_BUFFER,
	LAST_PROP
};

XpadUndo*
xpad_undo_new (XpadTextBuffer *buffer)
{
	return XPAD_UNDO (g_object_new (XPAD_TYPE_UNDO, "buffer", buffer, NULL));
}

static void
xpad_undo_class_init (XpadUndoClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	/* TODO: neither dispose nor finalize neither in here nor in xpad-pad.c are
	   actually called, have to fix cleaning up */
	gobject_class->dispose = xpad_undo_dispose;
	gobject_class->finalize = xpad_undo_finalize;
	gobject_class->set_property = xpad_undo_set_property;
	gobject_class->get_property = xpad_undo_get_property;
	gobject_class->constructor = xpad_undo_constructor;

	g_object_class_install_property (gobject_class,
					 PROP_BUFFER,
					 g_param_spec_pointer ("buffer",
								   "Pad Buffer",
								   "Pad buffer connected to this undo",
								   G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (gobject_class, sizeof (XpadUndoPrivate));
}


static void
xpad_undo_init (XpadUndo *undo)
{
	undo->priv = XPAD_UNDO_GET_PRIVATE (undo);

	undo->priv->buffer = NULL;
	undo->priv->history_start = g_list_append (NULL, NULL);
	undo->priv->history_curr = undo->priv->history_start;
	undo->priv->user_action = 0;
	undo->priv->frozen = FALSE;
}

static GObject*
xpad_undo_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	GObjectClass *parent_class = G_OBJECT_CLASS (xpad_undo_parent_class);
	GObject *obj = parent_class->constructor (gtype, n_properties, properties);

	XpadUndo *undo = XPAD_UNDO (obj);

	/* Assert user passed buffer as construct parameter */
	gint i;
	gboolean found_buffer = FALSE;
	for (i = 0; i < n_properties; i++)
	{
		if (!g_strcmp0 (properties[i].pspec->name, "buffer") &&
			G_VALUE_HOLDS_POINTER (properties[i].value) &&
			G_IS_OBJECT (g_value_get_pointer (properties[i].value)))
		{
			found_buffer = TRUE;
		}
	}

	if (!found_buffer)
	{
		undo->priv->buffer = NULL;
		g_warning ("GtkTextBuffer is not passed to XpadUndo constructor, undo will not work!\n");
	}
	else
	{
		/* Set up signals */
		g_signal_connect (G_OBJECT (undo->priv->buffer), "insert-text", G_CALLBACK (xpad_undo_insert_text), undo);
		g_signal_connect (G_OBJECT (undo->priv->buffer), "delete-range", G_CALLBACK (xpad_undo_delete_range), undo);
		g_signal_connect (G_OBJECT (undo->priv->buffer), "begin-user-action", G_CALLBACK (xpad_undo_begin_user_action), undo);
		g_signal_connect (G_OBJECT (undo->priv->buffer), "end-user-action", G_CALLBACK (xpad_undo_end_user_action), undo);
	}

	return obj;
}

static void
xpad_undo_dispose (GObject *object)
{
	XpadUndo *undo = XPAD_UNDO (object);

	if (undo->priv->buffer && G_IS_OBJECT (undo->priv->buffer))
	{
		g_object_unref (undo->priv->buffer);
		undo->priv->buffer = NULL;
	}

	G_OBJECT_CLASS (xpad_undo_parent_class)->dispose (object);
}

static void
xpad_undo_finalize (GObject *object)
{
	XpadUndo *undo = XPAD_UNDO (object);

	/* remove all elements except for the left guard */
	xpad_undo_clear_history (undo);

	/* remove left guard */
	g_list_free (undo->priv->history_start);

	G_OBJECT_CLASS (xpad_undo_parent_class)->finalize (object);
}

static void
xpad_undo_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadUndo *undo = XPAD_UNDO (object);

	switch (prop_id)
	{
	case PROP_BUFFER:
		if (undo->priv->buffer && G_IS_OBJECT (undo->priv->buffer))
			g_object_unref (undo->priv->buffer);
		if (G_VALUE_HOLDS_POINTER (value) && G_IS_OBJECT (g_value_get_pointer (value)))
		{
			undo->priv->buffer = g_value_get_pointer (value);
			g_object_ref (undo->priv->buffer);
		}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
    }
}

static void
xpad_undo_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadUndo *undo = XPAD_UNDO (object);

	switch (prop_id)
	{
	case PROP_BUFFER:
		g_value_set_pointer (value, undo->priv->buffer);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_undo_begin_user_action (GtkTextBuffer *buffer, XpadUndo *undo)
{
	undo->priv->user_action++;
}

static void
xpad_undo_end_user_action (GtkTextBuffer *buffer, XpadUndo *undo)
{
	if (undo->priv->user_action > 0)
		undo->priv->user_action--;
}

/* Removes current element and returns a pointer to the previous */
static GList*
xpad_undo_remove_action_elem (GList *curr)
{
	if (curr->data)
	{
		UserAction *action = curr->data;
		g_free (action->text);
		g_free (action);
		if (curr->prev)
			curr->prev->next = curr->next;
		if (curr->next)
			curr->next->prev = curr->prev;
		GList *new_curr = curr->prev;
		curr->prev = NULL;
		curr->next = NULL;
		g_list_free (curr);
		curr = new_curr;
	}
	return curr;
}

/* Redo is impossible after text insertion/deletion, only right after Undo (another Redo),
   so we have to remove every history entry after the current one if there is any */
static void
xpad_undo_clear_redo_history (XpadUndo *undo)
{
	while (undo->priv->history_curr->next)
		xpad_undo_remove_action_elem (undo->priv->history_curr->next);
}
	
static void xpad_undo_clear_history (XpadUndo *undo)
{
	while (undo->priv->history_start->next)
		xpad_undo_remove_action_elem (undo->priv->history_start->next);
	undo->priv->history_curr = undo->priv->history_start;
}

static void
xpad_undo_insert_text (GtkTextBuffer *buffer, GtkTextIter *location, gchar *text, gint len, XpadUndo *undo)
{
	if (undo->priv->frozen)
		return;

	if (undo->priv->user_action)
	{
		xpad_undo_clear_redo_history (undo);

		gint pos = gtk_text_iter_get_offset (location);
		gint n_utf8_chars = g_utf8_strlen (text, len);

		/* Merge similar actions. This is how Undo works in most editors, if there is a series of
		   1-letter insertions - they are merge for Undo */
		if (undo->priv->history_curr->data)
		{
			UserAction *prev_action = undo->priv->history_curr->data;

			if (prev_action->action_type == USER_ACTION_INSERT_TEXT)
			{
				/* series of 1-letter insertions */
				if (n_utf8_chars == 1 // this is a 1-letter insertion
					&& pos == prev_action->end // placed right after the previous text
					&& (prev_action->n_utf8_chars == 1 || prev_action->merged)) // with which we should merge
				{
					/* if there was a space stop merging unless that was a series of spaces */
					if ((!g_unichar_isspace (prev_action->text[0]) && !g_ascii_isspace (text[0])) ||
						(g_unichar_isspace (prev_action->text[0]) && g_unichar_isspace (text[0])))
					{
						gchar *joined_str = g_strjoin (NULL, prev_action->text, text, NULL);
						g_free (prev_action->text);
						prev_action->text = joined_str;
						prev_action->len_in_bytes += len;
						prev_action->end += len;
						prev_action->n_utf8_chars += n_utf8_chars;
						prev_action->merged = TRUE;
						return;
					}
				}
			}
		}

		UserAction *action = g_new (UserAction, 1);
		action->action_type = USER_ACTION_INSERT_TEXT;
		action->text = g_strdup (text);
		action->start = pos;
		action->end = pos + len;
		action->len_in_bytes = abs (action->end - action->start);
		action->n_utf8_chars = n_utf8_chars;
		action->merged = FALSE;

		/* since each operation clears redo we know that there
		   is nothing after history_curr at this point so we
		   insert right after it. history_start won't change
		   since it is a left guard - not NULL */
		GList *dummy_start = g_list_append (undo->priv->history_curr, action); // supress warning, we have left guard for start
		undo->priv->history_curr = g_list_next (undo->priv->history_curr);
	}
}

static void
xpad_undo_delete_range (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end, XpadUndo *undo)
{
	if (undo->priv->frozen)
		return;

	if (undo->priv->user_action)
	{
		xpad_undo_clear_redo_history (undo);

		gchar *text = gtk_text_iter_get_text (start, end);
		gint start_offset = gtk_text_iter_get_offset (start);
		gint end_offset = gtk_text_iter_get_offset (end);
		gint len = abs (end_offset - start_offset);
		gint n_utf8_chars = g_utf8_strlen (text, len);

		UserAction *action = g_new (UserAction, 1);
		action->action_type = USER_ACTION_DELETE_RANGE;
		action->text = g_strdup (text);
		action->start = start_offset;
		action->end = end_offset;
		action->len_in_bytes = len;
		action->n_utf8_chars = n_utf8_chars;
		action->merged = FALSE;

		GList *dummy_start = g_list_append (undo->priv->history_curr, action); // supress warning, we have left guard for start
		undo->priv->history_curr = g_list_next (undo->priv->history_curr);
	}
}

void xpad_undo_exec_undo (XpadUndo *undo)
{
	if (undo->priv->frozen)
		return;

	/* No undo without buffer */
	if (undo->priv->buffer == NULL || !G_IS_OBJECT (undo->priv->buffer))
		return;

	/* Undo current element if it is not a start of the list */
	if (undo->priv->history_curr->data)
	{
		UserAction *action = undo->priv->history_curr->data;

		switch (action->action_type)
		{
		case USER_ACTION_INSERT_TEXT:
			xpad_text_buffer_delete_range (undo->priv->buffer,
				action->start,
				action->end);
			break;
		case USER_ACTION_DELETE_RANGE:
			xpad_text_buffer_insert_text (undo->priv->buffer,
				action->start,
				action->text,
				action->len_in_bytes);
			break;
		}

		undo->priv->history_curr = g_list_previous (undo->priv->history_curr);
	}
}

void xpad_undo_exec_redo (XpadUndo *undo)
{
	if (undo->priv->frozen)
		return;

	/* No undo without buffer */
	if (undo->priv->buffer == NULL || !G_IS_OBJECT (undo->priv->buffer))
		return;

	/* Redo next element if there is any */
	if (undo->priv->history_curr->next)
	{
		UserAction *action = undo->priv->history_curr->next->data;

		switch (action->action_type)
		{
		case USER_ACTION_DELETE_RANGE:
			xpad_text_buffer_delete_range (undo->priv->buffer,
				action->start,
				action->end);
			break;
		case USER_ACTION_INSERT_TEXT:
			xpad_text_buffer_insert_text (undo->priv->buffer,
				action->start,
				action->text,
				action->len_in_bytes);
			break;
		}

		undo->priv->history_curr = g_list_next (undo->priv->history_curr);
	}
}

void xpad_undo_freeze (XpadUndo *undo)
{
	undo->priv->frozen = TRUE;
}

void xpad_undo_thaw (XpadUndo *undo)
{
	undo->priv->frozen = FALSE;
}

