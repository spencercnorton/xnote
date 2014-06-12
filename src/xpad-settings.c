/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2013-2014 Arthur Borsboom

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
#include <string.h>
#include "xpad-settings.h"
#include "fio.h"

struct XpadSettingsPrivate
{
	guint width;
	guint height;
	gboolean has_decorations;
	gboolean confirm_destroy;
	gboolean edit_lock;
	gboolean tray_enabled;
	guint tray_click_configuration;
	gboolean has_toolbar;
	gboolean autohide_toolbar;
	gboolean has_scrollbar;
	GdkRGBA *text;
	GdkRGBA *back;
	const gchar *fontname;
	GSList *toolbar_buttons;
	gboolean autostart_wait_systray;
	guint autostart_delay;
	gboolean autostart_new_pad;
	gboolean autostart_sticky;	
	guint autostart_display_pads;
};

G_DEFINE_TYPE_WITH_PRIVATE(XpadSettings, xpad_settings, G_TYPE_OBJECT)

#define DEFAULTS_FILENAME	"default-style"

enum
{
	CHANGE_BUTTONS,
	LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_HAS_DECORATIONS,
  PROP_CONFIRM_DESTROY,
  PROP_EDIT_LOCK,
  PROP_TRAY_ENABLED,
  PROP_TRAY_CLICK_CONFIGURATION,
  PROP_HAS_TOOLBAR,
  PROP_AUTOHIDE_TOOLBAR,
  PROP_HAS_SCROLLBAR,
  PROP_TEXT_COLOR,
  PROP_BACK_COLOR,
  PROP_FONTNAME,
  PROP_AUTOSTART_XPAD,
  PROP_AUTOSTART_WAIT_SYSTRAY,
  PROP_AUTOSTART_DELAY,  
  PROP_AUTOSTART_NEW_PAD,
  PROP_AUTOSTART_STICKY,
  PROP_AUTOSTART_DISPLAY_PADS,
  LAST_PROP
};

static void load_from_file (XpadSettings *settings, const gchar *filename);
static void save_to_file (XpadSettings *settings, const gchar *filename);
static void xpad_settings_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_settings_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_settings_dispose (GObject *object);
static void xpad_settings_finalize (GObject *object);

static guint signals[LAST_SIGNAL] = { 0 };

XpadSettings *
xpad_settings_new (void)
{
	return g_object_new (XPAD_TYPE_SETTINGS, NULL);
}

static void
xpad_settings_class_init (XpadSettingsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = xpad_settings_dispose;
	gobject_class->finalize = xpad_settings_finalize;
	gobject_class->set_property = xpad_settings_set_property;
	gobject_class->get_property = xpad_settings_get_property;
	
	g_object_class_install_property (gobject_class,
	                                 PROP_WIDTH,
	                                 g_param_spec_uint ("width",
	                                                    "Default Width of Pads",
	                                                    "Window width of pads on creation",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    200,
	                                                    G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_HEIGHT,
	                                 g_param_spec_uint ("height",
	                                                    "Default Height of Pads",
	                                                    "Window height of pads on creation",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    200,
	                                                    G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_HAS_DECORATIONS,
	                                 g_param_spec_boolean ("has-decorations",
	                                                       "Has Decorations",
	                                                       "Whether pads have window decorations",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_CONFIRM_DESTROY,
	                                 g_param_spec_boolean ("confirm-destroy",
	                                                       "Confirm Destroy",
	                                                       "Whether destroying a pad requires user confirmation",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_EDIT_LOCK,
	                                 g_param_spec_boolean ("edit-lock",
	                                                       "Edit Lock",
	                                                       "Whether edit lock mode is enabled",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
	                                 PROP_TRAY_ENABLED,
	                                 g_param_spec_boolean ("tray-enabled",
	                                                       "Enable the tray icon",
	                                                       "Whether to enable or disable the systray icon",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
                                     PROP_TRAY_CLICK_CONFIGURATION,
                                     g_param_spec_uint ("tray-click-configuration",
                                                        "Tray click configuration",
                                                        "What configuration is selected on tray click",
                                                        0,
                                                        G_MAXUINT,
                                                        0,
                                                        G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
	                                 PROP_HAS_TOOLBAR,
	                                 g_param_spec_boolean ("has-toolbar",
	                                                       "Each pad has a toolbar",
	                                                       "Whether pads have toolbars",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_AUTOHIDE_TOOLBAR,
	                                 g_param_spec_boolean ("autohide-toolbar",
	                                                       "Autohide Toolbar",
	                                                       "Whether toolbars hide when not used",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_HAS_SCROLLBAR,
	                                 g_param_spec_boolean ("has-scrollbar",
	                                                       "Has Scrollbar",
	                                                       "Whether pads have scrollbars",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_FONTNAME,
	                                 g_param_spec_string ("fontname",
	                                                      "Font Name",
	                                                      "Default name of pad font",
	                                                      NULL,
	                                                      G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_TEXT_COLOR,
	                                 g_param_spec_boxed ("text-color",
	                                                     "Text Color",
	                                                     "Default color of pad text",
	                                                     GDK_TYPE_RGBA,
	                                                     G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_BACK_COLOR,
	                                 g_param_spec_boxed ("back-color",
	                                                     "Back Color",
	                                                     "Default color of pad background",
	                                                     GDK_TYPE_RGBA,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
	                                 PROP_AUTOSTART_XPAD,
	                                 g_param_spec_boolean ("autostart-xpad",
	                                                       "Automatically start xpad",
	                                                       "Whether to start xpad after login",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
	                                 PROP_AUTOSTART_WAIT_SYSTRAY,
	                                 g_param_spec_boolean ("autostart-wait-systray",
	                                                       "Autostart Xpad wait for systray",
	                                                       "Whether to wait for the systray before starting xpad automatically after login",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
	                                 PROP_AUTOSTART_NEW_PAD,
	                                 g_param_spec_boolean ("autostart-new-pad",
	                                                       "Autostart a new pad",
	                                                       "Whether to create a new pad on startup",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_AUTOSTART_STICKY,
	                                 g_param_spec_boolean ("autostart-sticky",
	                                                       "Default Stickiness",
	                                                       "Whether pads are sticky on creation",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
                                     PROP_AUTOSTART_DELAY,
                                     g_param_spec_uint ("autostart-delay",
                                                        "Delay autostart of Xpad",
                                                        "How many seconds will Xpad wait before continuing startup",
                                                        0,
                                                        G_MAXUINT,
                                                        0,
                                                        G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
                                     PROP_AUTOSTART_DISPLAY_PADS,
                                     g_param_spec_uint ("autostart-display-pads",
                                                        "Autostart display pads",
                                                        "How to show the different pads when Xpad is started",
                                                        0,
                                                        G_MAXUINT,
                                                        2,
                                                        G_PARAM_READWRITE));

	signals[CHANGE_BUTTONS] = 
		g_signal_new ("change_buttons",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadSettingsClass, change_buttons),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
xpad_settings_init (XpadSettings *settings)
{
	settings->priv = xpad_settings_get_instance_private(settings);

	/* A pleasant light yellow background color, similar to commercial sticky notes, with black text. */
	settings->priv->text = gdk_rgba_copy(&(GdkRGBA) {0, 0, 0, 1});
	settings->priv->back = gdk_rgba_copy(&(GdkRGBA) {1, 0.933334350586, 0.6, 1});

	settings->priv->toolbar_buttons = NULL;
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("New"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Delete"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Separator"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Cut"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Copy"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Paste"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Separator"));	
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Undo"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Redo"));	

	load_from_file (settings, DEFAULTS_FILENAME);
}

static void
xpad_settings_dispose (GObject *object)
{
	G_OBJECT_CLASS (xpad_settings_parent_class)->dispose (object);
}

static void
xpad_settings_finalize (GObject *object)
{
	XpadSettings *settings = XPAD_SETTINGS (object);
	
	g_slist_free (settings->priv->toolbar_buttons);

	if (settings->priv->text)
		gdk_rgba_free (settings->priv->text);
	if (settings->priv->back)
		gdk_rgba_free (settings->priv->back);
		
	G_OBJECT_CLASS (xpad_settings_parent_class)->finalize (object);
}

void xpad_settings_add_toolbar_button (XpadSettings *settings, const gchar *button)
{
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup (button));
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);
}

static void xpad_settings_remove_toolbar_list_element (XpadSettings *settings, GSList *element)
{
	g_free (element->data);
	settings->priv->toolbar_buttons = g_slist_delete_link (settings->priv->toolbar_buttons, element);
}

gboolean xpad_settings_remove_all_toolbar_buttons (XpadSettings *settings)
{
	if (settings->priv->toolbar_buttons == NULL)
		return FALSE;

	while (settings->priv->toolbar_buttons)
	{
		g_free (settings->priv->toolbar_buttons->data);
		settings->priv->toolbar_buttons = 
			g_slist_delete_link (settings->priv->toolbar_buttons,
					settings->priv->toolbar_buttons);
	}

	settings->priv->toolbar_buttons = NULL;

	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);

	return TRUE;
}

gboolean xpad_settings_remove_last_toolbar_button (XpadSettings *settings)
{
	GSList *element;
	
	element = g_slist_last (settings->priv->toolbar_buttons);
	
	if (!element) {
		g_slist_free(element);
		return FALSE;
	}
	
	xpad_settings_remove_toolbar_list_element (settings, element);
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);
	
	return TRUE;
}

const GSList *xpad_settings_get_toolbar_buttons (XpadSettings *settings)
{
	return settings->priv->toolbar_buttons;
}

static void
xpad_settings_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadSettings *settings = XPAD_SETTINGS (object);
	
	switch (prop_id)
	{
	case PROP_WIDTH:
		settings->priv->width = g_value_get_uint (value);
		break;
	
	case PROP_HEIGHT:
		settings->priv->height = g_value_get_uint (value);
		break;
	
	case PROP_HAS_DECORATIONS:
		settings->priv->has_decorations = g_value_get_boolean (value);
		break;
	
	case PROP_CONFIRM_DESTROY:
		settings->priv->confirm_destroy = g_value_get_boolean (value);
		break;
	
	case PROP_EDIT_LOCK:
		settings->priv->edit_lock = g_value_get_boolean (value);
		break;
		
	case PROP_TRAY_ENABLED:
		settings->priv->tray_enabled = g_value_get_boolean (value);
		break;

	case PROP_TRAY_CLICK_CONFIGURATION:
		settings->priv->tray_click_configuration = g_value_get_uint(value);
		break;
		
	case PROP_HAS_TOOLBAR:
		settings->priv->has_toolbar = g_value_get_boolean (value);
		break;
	
	case PROP_AUTOHIDE_TOOLBAR:
		settings->priv->autohide_toolbar = g_value_get_boolean (value);
		break;
	
	case PROP_HAS_SCROLLBAR:
		settings->priv->has_scrollbar = g_value_get_boolean (value);
		break;
	
	case PROP_TEXT_COLOR:
		if (settings->priv->text)
			gdk_rgba_free (settings->priv->text);
		if (g_value_get_boxed (value))
			settings->priv->text = gdk_rgba_copy (g_value_get_boxed (value));
		else
			settings->priv->text = NULL;
		break;

	case PROP_BACK_COLOR:
		if (settings->priv->back)
			gdk_rgba_free (settings->priv->back);
		if (g_value_get_boxed (value))
			settings->priv->back = gdk_rgba_copy (g_value_get_boxed (value));
		else
			settings->priv->back = NULL;
		break;

	case PROP_FONTNAME:
		settings->priv->fontname = g_value_get_string (value);
		break;

	case PROP_AUTOSTART_XPAD:
		break;

	case PROP_AUTOSTART_WAIT_SYSTRAY:
		settings->priv->autostart_wait_systray = g_value_get_boolean (value);
		break;

	case PROP_AUTOSTART_NEW_PAD:
		settings->priv->autostart_new_pad = g_value_get_boolean (value);
		break;

	case PROP_AUTOSTART_STICKY:
		settings->priv->autostart_sticky = g_value_get_boolean (value);
		break;
	
	case PROP_AUTOSTART_DELAY:
		settings->priv->autostart_delay = g_value_get_uint (value);
		break;

	case PROP_AUTOSTART_DISPLAY_PADS:
		settings->priv->autostart_display_pads = g_value_get_uint (value);
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		return;
	}
	save_to_file (settings, DEFAULTS_FILENAME);
}

static void
xpad_settings_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadSettings *settings = XPAD_SETTINGS (object);
	
	switch (prop_id)
	{
	case PROP_WIDTH:
		g_value_set_uint (value, settings->priv->width);
		break;
	
	case PROP_HEIGHT:
		g_value_set_uint (value, settings->priv->height);
		break;
	
	case PROP_HAS_DECORATIONS:
		g_value_set_boolean (value, settings->priv->has_decorations);
		break;
	
	case PROP_CONFIRM_DESTROY:
		g_value_set_boolean (value, settings->priv->confirm_destroy);
		break;
	
	case PROP_EDIT_LOCK:
		g_value_set_boolean (value, settings->priv->edit_lock);
		break;
	
	case PROP_TRAY_ENABLED:
		g_value_set_boolean (value, settings->priv->tray_enabled);
		break;

	case PROP_TRAY_CLICK_CONFIGURATION:
		g_value_set_uint (value, settings->priv->tray_click_configuration);
		break;

	case PROP_HAS_TOOLBAR:
		g_value_set_boolean (value, settings->priv->has_toolbar);
		break;
	
	case PROP_AUTOHIDE_TOOLBAR:
		g_value_set_boolean (value, settings->priv->autohide_toolbar);
		break;
	
	case PROP_HAS_SCROLLBAR:
		g_value_set_boolean (value, settings->priv->has_scrollbar);
		break;
	
	case PROP_TEXT_COLOR:
		if (settings->priv->text)
			g_value_set_static_boxed (value, settings->priv->text);
		else
			value = NULL;
		break;
	
	case PROP_BACK_COLOR:
		if (settings->priv->back)
			g_value_set_static_boxed (value, settings->priv->back);
		else
			value = NULL;
		break;
	
	case PROP_FONTNAME:
		g_value_set_string (value, settings->priv->fontname);
		break;

	case PROP_AUTOSTART_XPAD:
		g_value_set_boolean (value, g_file_test (g_strdup_printf ("%s/.config/autostart/xpad.desktop", g_getenv ("HOME")), G_FILE_TEST_EXISTS));
		break;

	case PROP_AUTOSTART_WAIT_SYSTRAY:
		g_value_set_boolean (value, settings->priv->autostart_wait_systray);
		break;
	
	case PROP_AUTOSTART_NEW_PAD:
		g_value_set_boolean (value, settings->priv->autostart_new_pad);
		break;
	
	case PROP_AUTOSTART_STICKY:
		g_value_set_boolean (value, settings->priv->autostart_sticky);
		break;
	
	case PROP_AUTOSTART_DELAY:
		g_value_set_uint (value, settings->priv->autostart_delay);
		break;

	case PROP_AUTOSTART_DISPLAY_PADS:
		g_value_set_uint (value, settings->priv->autostart_display_pads);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		return;
	}
}

static void
load_from_file (XpadSettings *settings, const gchar *filename)
{
	gchar *buttons = NULL;
	gchar *text_color_string = NULL;
	gchar *background_color_string = NULL;
	GdkRGBA text = {0, 0, 0, 0};
	GdkRGBA back = {0, 0, 0, 0};
	gboolean use_text, use_back;
	
	use_text = settings->priv->text ? TRUE : FALSE;
	if (settings->priv->text)
		text = *settings->priv->text;

	use_back = settings->priv->back ? TRUE : FALSE;
	if (settings->priv->back)
		back = *settings->priv->back;

	/* get all the values from the default-style text file in the forms of booleans, ints or strings. */
	if (fio_get_values_from_file (filename, 
		"b|decorations", &settings->priv->has_decorations,
		"u|height", &settings->priv->height,
		"u|width", &settings->priv->width,
		"b|confirm_destroy", &settings->priv->confirm_destroy,
		"b|edit_lock", &settings->priv->edit_lock,
		"b|sticky_on_start", &settings->priv->autostart_sticky,
		"b|tray_enabled", &settings->priv->tray_enabled,
		"u|tray_click_configuration", &settings->priv->tray_click_configuration,
		"s|back", &background_color_string,
		"b|use_back", &use_back,
		"s|text", &text_color_string,
		"b|use_text", &use_text,
		"s|fontname", &settings->priv->fontname,
		"b|toolbar", &settings->priv->has_toolbar,
		"b|auto_hide_toolbar", &settings->priv->autohide_toolbar,
		"b|scrollbar", &settings->priv->has_scrollbar,
		"s|buttons", &buttons,
		"b|autostart_wait_systray", &settings->priv->autostart_wait_systray,
		"u|autostart_delay", &settings->priv->autostart_delay,		
		"b|autostart_new_pad", &settings->priv->autostart_new_pad,
		"u|autostart_display_pads", &settings->priv->autostart_display_pads,
		NULL))
		return;

	if (use_text)
	{
		gdk_rgba_free (settings->priv->text);

		/*
		 * If, for some reason, one of the colors could not be retrieved
		 * (for example due to the migration to the new GdkRGBA colors),
		 * set the color to the default.
		 */
		if (text_color_string == NULL) {
			text = (GdkRGBA) {0, 0, 0, 1};
		}
		else {
			/* If, for some reason, the parsing of the colors fail, set the color to the default. */
			if (!gdk_rgba_parse (&text, text_color_string)) {
				text = (GdkRGBA) {0, 0, 0, 1};
			}
		}

		settings->priv->text = gdk_rgba_copy (&text);
	}

	gdk_rgba_free (settings->priv->back);
	if (use_back) {
		/*
		 * If, for some reason, one of the colors could not be retrieved
		 * (for example due to the migration to the new GdkRGBA colors),
		 * set the color to the default.
		 */
		if (background_color_string == NULL) {
			back = (GdkRGBA) {1, 0.933334350586, 0.6, 1};
		}
		else {
			/* If, for some reason, the parsing of the colors fail, set the color to the default. */
			if (!gdk_rgba_parse (&back, background_color_string)) {
				back = (GdkRGBA) {1, 0.933334350586, 0.6, 1};
			}
		}

		settings->priv->back = gdk_rgba_copy (&back);
	}
	else
		settings->priv->back = NULL;

	if (settings->priv->fontname && strcmp (settings->priv->fontname, "NULL") == 0)
		settings->priv->fontname = NULL;
	
	if (buttons)
	{
		gint i;
		gchar **button_names;
		
		button_names = g_strsplit (buttons, ",", 0);
		
		while (settings->priv->toolbar_buttons)
		{
			g_free (settings->priv->toolbar_buttons->data);
			settings->priv->toolbar_buttons = 
				g_slist_delete_link (settings->priv->toolbar_buttons,
				settings->priv->toolbar_buttons);
		}
		
		for (i = 0; button_names[i]; ++i)
		{
			settings->priv->toolbar_buttons = 
				g_slist_append (settings->priv->toolbar_buttons,
				g_strstrip (button_names[i])); /* takes ownership of string */
		}
		
		g_free (button_names);
		g_free (buttons);
	}
}


static void
save_to_file (XpadSettings *settings, const gchar *filename)
{
	gchar *buttons = g_strdup ("");
	GSList *tmp;
	
	tmp = settings->priv->toolbar_buttons;
	
	while (tmp)
	{
		gchar *tmpstr = buttons;
		
		if (tmp->next)
			buttons = g_strconcat (buttons, tmp->data, ", ", NULL);
		else
			buttons = g_strconcat (buttons, tmp->data, NULL);
		
		g_free (tmpstr);
		tmp = tmp->next;
	}
	
	fio_set_values_to_file (filename, 
		"b|decorations", settings->priv->has_decorations,
		"u|height", settings->priv->height,
		"u|width", settings->priv->width,
		"b|confirm_destroy", settings->priv->confirm_destroy,
		"b|edit_lock", settings->priv->edit_lock,
		"b|sticky_on_start", settings->priv->autostart_sticky,
		"b|tray_enabled", settings->priv->tray_enabled,
		"u|tray_click_configuration", settings->priv->tray_click_configuration,
		"s|back", settings->priv->back ? gdk_rgba_to_string (settings->priv->back) : "NULL",
		"b|use_back", settings->priv->back ? TRUE : FALSE,
		"s|text", settings->priv->text ? gdk_rgba_to_string (settings->priv->text) : "NULL",
		"b|use_text", settings->priv->text ? TRUE : FALSE,
		"s|fontname", settings->priv->fontname ? settings->priv->fontname : "NULL",
		"b|toolbar", settings->priv->has_toolbar,
		"b|auto_hide_toolbar", settings->priv->autohide_toolbar,
		"b|scrollbar", settings->priv->has_scrollbar,
		"s|buttons", buttons,
		"b|autostart_wait_systray", settings->priv->autostart_wait_systray,
		"u|autostart_delay", settings->priv->autostart_delay,		
		"b|autostart_new_pad", settings->priv->autostart_new_pad,
		"u|autostart_display_pads", settings->priv->autostart_display_pads,
		NULL);
	
	g_free (buttons);
}
