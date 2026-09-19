/*
 * test-tray-lifecycle.c -- asynchronous tray enable/disable regressions.
 *
 * The real tray talks to the session bus and depends on the process-global pad
 * group. This test supplies a minimal pad-group double and uses GTestDBus so it
 * can exercise queued g_bus_own_name()/g_bus_watch_name() callbacks without a
 * desktop session or a user's real notes.
 */

#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "xpad-app.h"
#include "xpad-pad.h"
#include "xpad-pad-group.h"
#include "xpad-preferences.h"
#include "xpad-settings.h"
#include "xpad-tray.h"
#include "help.h"

typedef struct {
	GObject parent_instance;
} TestPadGroup;

typedef struct {
	GObjectClass parent_class;
} TestPadGroupClass;

G_DEFINE_TYPE (TestPadGroup, test_pad_group, G_TYPE_OBJECT)

static TestPadGroup *test_group;
static gchar *test_config_dir;
static gboolean group_has_pads;
static guint group_visible_pads;
static guint show_all_count;
static guint new_pad_count;

static void
test_pad_group_class_init (TestPadGroupClass *klass)
{
	g_signal_new ("pad_added", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	g_signal_new ("pad_removed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
}

static void
test_pad_group_init (TestPadGroup *group)
{
	(void) group;
}

/* ---- app/pad-group doubles used by xpad-tray.c and xpad-settings.c ---- */

XpadPadGroup *
xpad_app_get_pad_group (void)
{
	return (XpadPadGroup *) test_group;
}

void
xpad_app_quit (void)
{
	g_assert_not_reached ();
}

void
xpad_app_error (GtkWindow *parent, const gchar *primary, const gchar *secondary)
{
	(void) parent;
	g_error ("unexpected app error: %s: %s", primary, secondary);
}

const gchar *
xpad_app_get_config_dir (void)
{
	return test_config_dir;
}

gboolean
xpad_pad_group_has_pads (XpadPadGroup *group)
{
	(void) group;
	return group_has_pads;
}

guint
xpad_pad_group_num_visible_pads (XpadPadGroup *group)
{
	(void) group;
	return group_visible_pads;
}

GSList *
xpad_pad_group_get_pads_sorted_by_title (XpadPadGroup *group)
{
	(void) group;
	return NULL;
}

gchar *
xpad_pad_get_title_for_menu (XpadPad *pad, gint pad_number)
{
	(void) pad;
	(void) pad_number;
	return g_strdup ("unused");
}

GtkWidget *
xpad_pad_new (XpadPadGroup *group, XpadSettings *settings)
{
	(void) group;
	(void) settings;
	new_pad_count++;
	group_has_pads = TRUE;
	group_visible_pads = 1;
	/* The product code accepts a failed construction defensively; returning
	   NULL keeps this bus-lifecycle test independent of a GDK display. */
	return NULL;
}

void
xpad_pad_group_show_all (XpadPadGroup *group)
{
	(void) group;
	show_all_count++;
	group_visible_pads = group_has_pads ? 1 : 0;
}

void
xpad_pad_group_close_all (XpadPadGroup *group)
{
	(void) group;
}

void
xpad_pad_group_toggle_hide (XpadPadGroup *group)
{
	(void) group;
}

void
xpad_preferences_open (XpadSettings *settings)
{
	(void) settings;
}

void
show_help (void)
{
}

/* ---- main-context helpers ---- */

static void
iterate_for (gint64 microseconds)
{
	gint64 deadline = g_get_monotonic_time () + microseconds;

	do {
		while (g_main_context_iteration (NULL, FALSE))
			;
		g_usleep (1000);
	} while (g_get_monotonic_time () < deadline);
}

static void
wait_until_ready (void)
{
	gint64 deadline = g_get_monotonic_time () + 2 * G_TIME_SPAN_SECOND;

	while (!xpad_tray_ready () && g_get_monotonic_time () < deadline) {
		while (g_main_context_iteration (NULL, FALSE))
			;
		g_usleep (1000);
	}
	g_assert_true (xpad_tray_ready ());
}

static void
test_queued_callbacks_and_reachability (void)
{
	GTestDBus *bus = g_test_dbus_new (G_TEST_DBUS_NONE);
	g_test_dbus_up (bus);

	group_has_pads = TRUE;
	group_visible_pads = 0;
	show_all_count = 0;
	new_pad_count = 0;

	XpadSettings *settings = xpad_settings_new ();
	g_object_set (settings, "tray-enabled", FALSE, NULL);

	/* Initial configuration happens before saved pads are loaded. An initially
	   disabled tray must not manufacture a blank pad; the app's normal startup
	   reachability check handles the post-load state. */
	group_has_pads = FALSE;
	xpad_tray_init (settings);
	g_assert_cmpuint (new_pad_count, ==, 0);
	xpad_tray_dispose (settings);

	group_has_pads = TRUE;
	g_object_set (settings, "tray-enabled", TRUE, NULL);

	/* Disable before the main context can deliver bus-acquired/name callbacks.
	   Stale callbacks must neither touch cleared introspection data nor make
	   the stopped runtime look active. Hidden notes must be revealed first. */
	xpad_tray_init (settings);
	g_object_set (settings, "tray-enabled", FALSE, NULL);
	g_assert_cmpuint (show_all_count, ==, 1);
	g_assert_cmpuint (group_visible_pads, ==, 1);
	g_assert_true (xpad_tray_ready ());
	g_assert_false (xpad_tray_has_indicator ());
	iterate_for (100 * 1000);
	g_assert_true (xpad_tray_ready ());
	g_assert_false (xpad_tray_has_indicator ());

	/* Interleave queued callbacks from successive generations. The private bus
	   deliberately has no StatusNotifierWatcher, so each current generation
	   must resolve ready-but-not-visible without stale state leaking through. */
	for (guint i = 0; i < 20; i++) {
		g_object_set (settings, "tray-enabled", TRUE, NULL);
		if ((i % 2) == 0)
			g_main_context_iteration (NULL, FALSE);
		g_object_set (settings, "tray-enabled", FALSE, NULL);
		g_object_set (settings, "tray-enabled", TRUE, NULL);
		wait_until_ready ();
		g_assert_false (xpad_tray_has_indicator ());
		g_object_set (settings, "tray-enabled", FALSE, NULL);
		g_assert_true (xpad_tray_ready ());
	}
	iterate_for (100 * 1000);

	/* If no notes remain, disabling the only launcher creates a reachable pad. */
	group_has_pads = FALSE;
	group_visible_pads = 0;
	g_object_set (settings, "tray-enabled", TRUE, NULL);
	g_object_set (settings, "tray-enabled", FALSE, NULL);
	g_assert_cmpuint (new_pad_count, ==, 1);

	xpad_tray_dispose (settings);
	iterate_for (100 * 1000);
	g_object_unref (settings);
	g_test_dbus_down (bus);
	g_object_unref (bus);
}

int
main (int argc, char **argv)
{
	GError *error = NULL;
	gchar *test_root = g_dir_make_tmp ("test-tray-lifecycle-XXXXXX", &error);
	g_assert_no_error (error);
	g_assert_nonnull (test_root);

	gchar *xdg_config = g_build_filename (test_root, "config", NULL);
	test_config_dir = g_build_filename (xdg_config, "xnote", NULL);
	g_assert_cmpint (g_mkdir_with_parents (test_config_dir, 0700), ==, 0);
	g_setenv ("XDG_CONFIG_HOME", xdg_config, TRUE);

	test_group = g_object_new (test_pad_group_get_type (), NULL);

	g_test_init (&argc, &argv, NULL);
	g_test_add_func ("/tray/queued_callbacks_and_reachability",
		test_queued_callbacks_and_reachability);

	int result = g_test_run ();

	g_object_unref (test_group);
	gchar *defaults = g_build_filename (test_config_dir, "default-style", NULL);
	g_unlink (defaults);
	g_rmdir (test_config_dir);
	g_rmdir (xdg_config);
	g_rmdir (test_root);
	g_free (defaults);
	g_free (test_config_dir);
	g_free (xdg_config);
	g_free (test_root);

	return result;
}
