/*
 * test-autostart.c -- regression tests for XNote's autostart launcher.
 *
 * The installed launcher must remain a symlink to the packaged desktop file.
 * In particular, changing the retained legacy wait-systray setting must never
 * rewrite that symlink as a regular per-user copy.
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "xpad-settings.h"

static gchar *test_root;
static gchar *test_config_dir;
static guint app_error_count;

/* ---- xpad-app stubs ---- */

void
xpad_app_error (GtkWindow *parent, const gchar *primary, const gchar *secondary)
{
	(void) parent;
	(void) primary;
	(void) secondary;
	app_error_count++;
}

const gchar *
xpad_app_get_config_dir (void)
{
	return test_config_dir;
}

/* ---- helpers ---- */

static gchar *
autostart_dir_path (void)
{
	return g_build_filename (g_get_user_config_dir (), "autostart", NULL);
}

static gchar *
autostart_link_path (void)
{
	return g_build_filename (g_get_user_config_dir (), "autostart", "xnote.desktop", NULL);
}

static void
clear_autostart_path (void)
{
	gchar *link = autostart_link_path ();
	gchar *dir = autostart_dir_path ();
	gchar *sentinel = g_build_filename (link, "sentinel", NULL);

	g_unlink (sentinel);
	g_remove (link);
	g_rmdir (dir);
	g_unlink (dir);

	g_free (sentinel);
	g_free (link);
	g_free (dir);
}

static void
assert_no_temp_links (void)
{
	gchar *dir_path = autostart_dir_path ();
	GError *error = NULL;
	GDir *dir = g_dir_open (dir_path, 0, &error);
	g_assert_no_error (error);
	g_assert_nonnull (dir);

	const gchar *name;
	while ((name = g_dir_read_name (dir)) != NULL)
		g_assert_false (g_str_has_prefix (name, ".xnote.desktop.tmp-"));

	g_dir_close (dir);
	g_free (dir_path);
}

/* ---- tests ---- */

static void
test_package_link_survives_legacy_setting (void)
{
	clear_autostart_path ();
	app_error_count = 0;

	XpadSettings *settings = xpad_settings_new ();
	g_object_set (settings, "autostart-xpad", TRUE, NULL);

	gchar *link = autostart_link_path ();
	g_assert_true (g_file_test (link, G_FILE_TEST_IS_SYMLINK));

	GError *error = NULL;
	gchar *target = g_file_read_link (link, &error);
	g_assert_no_error (error);
	g_assert_cmpstr (target, ==, "/tmp/applications/xnote.desktop");

	/* This key is still persisted for compatibility, but no longer has any
	   launcher-writing behavior. */
	g_object_set (settings, "autostart-wait-systray", FALSE, NULL);
	g_assert_true (g_file_test (link, G_FILE_TEST_IS_SYMLINK));
	gchar *target_after = g_file_read_link (link, &error);
	g_assert_no_error (error);
	g_assert_cmpstr (target_after, ==, target);

	/* Re-enabling an already-enabled launcher also leaves a package link. */
	g_object_set (settings, "autostart-xpad", TRUE, NULL);
	g_assert_true (g_file_test (link, G_FILE_TEST_IS_SYMLINK));

	gboolean enabled = FALSE;
	g_object_get (settings, "autostart-xpad", &enabled, NULL);
	g_assert_true (enabled);
	g_assert_cmpuint (app_error_count, ==, 0);

	g_object_set (settings, "autostart-xpad", FALSE, NULL);
	g_assert_false (g_file_test (link, G_FILE_TEST_EXISTS));
	g_assert_false (g_file_test (link, G_FILE_TEST_IS_SYMLINK));
	g_assert_cmpuint (app_error_count, ==, 0);

	g_free (target_after);
	g_free (target);
	g_free (link);
	g_object_unref (settings);
	clear_autostart_path ();
}

static void
test_mkdir_failure_is_reported_without_crash (void)
{
	clear_autostart_path ();
	app_error_count = 0;

	gchar *dir = autostart_dir_path ();
	GError *error = NULL;
	g_file_set_contents (dir, "not a directory", -1, &error);
	g_assert_no_error (error);

	XpadSettings *settings = xpad_settings_new ();
	g_object_set (settings, "autostart-xpad", TRUE, NULL);

	gchar *link = autostart_link_path ();
	g_assert_false (g_file_test (link, G_FILE_TEST_EXISTS));
	g_assert_cmpuint (app_error_count, ==, 1);
	gboolean enabled = TRUE;
	g_object_get (settings, "autostart-xpad", &enabled, NULL);
	g_assert_false (enabled);

	g_object_unref (settings);
	g_free (link);
	g_free (dir);
	clear_autostart_path ();
}

static void
test_failed_atomic_replace_preserves_launcher (void)
{
	clear_autostart_path ();
	app_error_count = 0;

	gchar *dir = autostart_dir_path ();
	gchar *link = autostart_link_path ();
	g_assert_cmpint (g_mkdir_with_parents (dir, 0700), ==, 0);

	GFile *launcher = g_file_new_for_path (link);
	GError *error = NULL;
	g_assert_true (g_file_make_symbolic_link (
		launcher, "/tmp/applications/old-xnote.desktop", NULL, &error));
	g_assert_no_error (error);

	XpadSettings *settings = xpad_settings_new ();
	g_setenv ("XPAD_TEST_AUTOSTART_MOVE_FAIL", "1", TRUE);
	g_object_set (settings, "autostart-xpad", TRUE, NULL);
	g_unsetenv ("XPAD_TEST_AUTOSTART_MOVE_FAIL");

	gchar *target = g_file_read_link (link, &error);
	g_assert_no_error (error);
	g_assert_cmpstr (target, ==, "/tmp/applications/old-xnote.desktop");
	g_assert_cmpuint (app_error_count, ==, 1);
	assert_no_temp_links ();

	/* A regular-file launcher is also left byte-for-byte intact when the
	   atomic replacement step fails. */
	g_assert_cmpint (g_unlink (link), ==, 0);
	g_file_set_contents (link, "existing launcher\n", -1, &error);
	g_assert_no_error (error);
	g_setenv ("XPAD_TEST_AUTOSTART_MOVE_FAIL", "1", TRUE);
	g_object_set (settings, "autostart-xpad", TRUE, NULL);
	g_unsetenv ("XPAD_TEST_AUTOSTART_MOVE_FAIL");

	gchar *contents = NULL;
	gsize length = 0;
	g_file_get_contents (link, &contents, &length, &error);
	g_assert_no_error (error);
	g_assert_cmpstr (contents, ==, "existing launcher\n");
	g_assert_cmpuint (app_error_count, ==, 2);
	assert_no_temp_links ();

	g_free (contents);
	g_free (target);
	g_object_unref (settings);
	g_object_unref (launcher);
	g_free (link);
	g_free (dir);
	clear_autostart_path ();
}

static void
test_non_file_object_is_not_enabled (void)
{
	clear_autostart_path ();
	app_error_count = 0;

	gchar *dir = autostart_dir_path ();
	gchar *link = autostart_link_path ();
	g_assert_cmpint (g_mkdir_with_parents (link, 0700), ==, 0);

	XpadSettings *settings = xpad_settings_new ();
	gboolean enabled = TRUE;
	g_object_get (settings, "autostart-xpad", &enabled, NULL);
	g_assert_false (enabled);
	g_assert_cmpuint (app_error_count, ==, 0);

	g_object_unref (settings);
	g_free (link);
	g_free (dir);
	clear_autostart_path ();
}

int
main (int argc, char **argv)
{
	GError *error = NULL;
	test_root = g_dir_make_tmp ("test-autostart-XXXXXX", &error);
	g_assert_no_error (error);
	g_assert_nonnull (test_root);

	gchar *xdg_config = g_build_filename (test_root, "config", NULL);
	test_config_dir = g_build_filename (xdg_config, "xnote", NULL);
	g_assert_cmpint (g_mkdir_with_parents (test_config_dir, 0700), ==, 0);
	g_setenv ("XDG_CONFIG_HOME", xdg_config, TRUE);

	g_test_init (&argc, &argv, NULL);
	g_test_add_func ("/autostart/package_link_survives_legacy_setting",
	                 test_package_link_survives_legacy_setting);
	g_test_add_func ("/autostart/mkdir_failure_is_reported_without_crash",
	                 test_mkdir_failure_is_reported_without_crash);
	g_test_add_func ("/autostart/failed_atomic_replace_preserves_launcher",
	                 test_failed_atomic_replace_preserves_launcher);
	g_test_add_func ("/autostart/non_file_object_is_not_enabled",
	                 test_non_file_object_is_not_enabled);

	int result = g_test_run ();

	clear_autostart_path ();
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
