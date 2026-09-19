/*
 * test-fio.c — unit tests for fio key/value read-write round-trip.
 *
 * fio_set_values_to_file / fio_get_values_from_file use CONFIG_DIR for
 * relative filenames but take an absolute path verbatim.  The tests use
 * a mkstemp file to stay hermetic.
 *
 * fio.c calls xpad_app_error on write failures; we provide a stub so we
 * don't have to link the entire app.  xpad_app_get_config_dir is only
 * called for relative paths, so absolute paths in these tests bypass it.
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Forward declarations matching fio.h without importing xpad-app.h. */
gint fio_set_values_to_file (const gchar *filename, ...);
gint fio_get_values_from_file (const gchar *filename, ...);
gboolean fio_set_file (const gchar *name, const gchar *value);
gchar *fio_get_file (const gchar *filename, int dirType);

/* Stub: xpad_app_error is called by fio on write failure.  In tests we just
 * print to stderr so the test harness can report it. */
void
xpad_app_error (void *parent, const gchar *primary, const gchar *secondary)
{
	(void) parent;
	g_printerr ("fio error: %s\n", primary ? primary : "(null)");
}

/* Stub: only needed for relative-path resolution; never reached in these tests
 * because we always pass an absolute path. */
const gchar *
xpad_app_get_config_dir (void)
{
	return "/tmp";
}

/* ---- helpers ---- */

static gchar *
make_temp_path (void)
{
	gchar *path = g_strdup ("/tmp/test-fio-XXXXXX");
	int fd = g_mkstemp (path);
	g_assert_cmpint (fd, >=, 0);
	close (fd);
	return path;
}

/* ---- tests ---- */

static void
test_raw_set_get (void)
{
	gchar *path = make_temp_path ();
	gboolean ok = fio_set_file (path, "hello\n");
	g_assert_true (ok);

	gchar *contents = fio_get_file (path, 1 /* CURRENT_WORK_DIR — irrelevant for abs path */);
	g_assert_nonnull (contents);
	g_assert_cmpstr (contents, ==, "hello\n");
	g_free (contents);
	g_unlink (path);
	g_free (path);
}

static void
test_kv_int_round_trip (void)
{
	gchar *path = make_temp_path ();
	gint written = 42, read_back = 0;

	gint rc = fio_set_values_to_file (path, "i|width", (gint) written, NULL);
	g_assert_cmpint (rc, ==, 0);

	rc = fio_get_values_from_file (path, "i|width", &read_back, NULL);
	g_assert_cmpint (rc, ==, 0);
	g_assert_cmpint (read_back, ==, written);

	g_unlink (path);
	g_free (path);
}

static void
test_kv_string_round_trip (void)
{
	gchar *path = make_temp_path ();
	gchar *read_back = NULL;

	gint rc = fio_set_values_to_file (path, "s|title", "my note title", NULL);
	g_assert_cmpint (rc, ==, 0);

	rc = fio_get_values_from_file (path, "s|title", &read_back, NULL);
	g_assert_cmpint (rc, ==, 0);
	g_assert_cmpstr (read_back, ==, "my note title");

	g_free (read_back);
	g_unlink (path);
	g_free (path);
}

static void
test_kv_bool_round_trip (void)
{
	gchar *path = make_temp_path ();
	gboolean read_back = FALSE;

	gint rc = fio_set_values_to_file (path, "b|sticky", (gboolean) TRUE, NULL);
	g_assert_cmpint (rc, ==, 0);

	rc = fio_get_values_from_file (path, "b|sticky", &read_back, NULL);
	g_assert_cmpint (rc, ==, 0);
	g_assert_true (read_back);

	g_unlink (path);
	g_free (path);
}

static void
test_kv_multiple_values (void)
{
	gchar *path = make_temp_path ();
	gint  w_out = 300, h_out = 200;
	gchar *title_out = NULL;
	gint  w_in = 0,  h_in = 0;
	gchar *title_in = NULL;

	gint rc = fio_set_values_to_file (path,
		"i|width",  w_out,
		"i|height", h_out,
		"s|title",  title_out ? title_out : "note",
		NULL);
	g_assert_cmpint (rc, ==, 0);

	rc = fio_get_values_from_file (path,
		"i|width",  &w_in,
		"i|height", &h_in,
		"s|title",  &title_in,
		NULL);
	g_assert_cmpint (rc, ==, 0);
	g_assert_cmpint (w_in, ==, w_out);
	g_assert_cmpint (h_in, ==, h_out);
	g_assert_cmpstr (title_in, ==, "note");

	g_free (title_in);
	g_unlink (path);
	g_free (path);
}

static void
test_missing_key_leaves_default (void)
{
	gchar *path = make_temp_path ();
	gint value = 99;

	gint rc = fio_set_values_to_file (path, "i|width", 42, NULL);
	g_assert_cmpint (rc, ==, 0);

	/* "height" is not in the file; value should stay at 99. */
	rc = fio_get_values_from_file (path, "i|height", &value, NULL);
	g_assert_cmpint (rc, ==, 0);
	g_assert_cmpint (value, ==, 99);

	g_unlink (path);
	g_free (path);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/fio/raw_set_get",             test_raw_set_get);
	g_test_add_func ("/fio/kv_int_round_trip",       test_kv_int_round_trip);
	g_test_add_func ("/fio/kv_string_round_trip",    test_kv_string_round_trip);
	g_test_add_func ("/fio/kv_bool_round_trip",      test_kv_bool_round_trip);
	g_test_add_func ("/fio/kv_multiple_values",      test_kv_multiple_values);
	g_test_add_func ("/fio/missing_key_leaves_default", test_missing_key_leaves_default);

	return g_test_run ();
}
