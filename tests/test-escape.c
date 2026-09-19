/*
 * test-escape.c — unit tests for the U+E000 escape scheme in xpad-text-buffer.
 *
 * Tests the escape_segment / unescape_segment helpers directly (they are
 * exported as xpad_text_buffer_escape_segment / xpad_text_buffer_unescape_segment
 * for exactly this purpose).  No GtkTextBuffer, no display, no XpadPad needed.
 */

#include <glib.h>
#include <string.h>

/* Pull in only the escape helpers — forward-declare to avoid dragging in
 * the full xpad-text-buffer.h dependency chain (XpadPad, GtkSourceBuffer…). */
gchar *xpad_text_buffer_escape_segment   (const gchar *text);
gchar *xpad_text_buffer_unescape_segment (const gchar *text);

/* U+E000 TAG_CHAR, U+E001 ESC_CHAR, U+E002 ESC_PLACEHOLDER encoded as UTF-8. */
#define TAG_UTF8  "\xee\x80\x80"   /* U+E000 */
#define ESC_UTF8  "\xee\x80\x81"   /* U+E001 */
#define PH_UTF8   "\xee\x80\x82"   /* U+E002 — placeholder for escaped TAG_CHAR */

static void
test_plain_text_unchanged (void)
{
	gchar *esc = xpad_text_buffer_escape_segment ("hello world\n");
	g_assert_cmpstr (esc, ==, "hello world\n");
	g_free (esc);
}

static void
test_tag_char_escaped (void)
{
	/* A literal U+E000 in user text must become ESC_CHAR ESC_PLACEHOLDER on disk
	 * (never bare TAG_CHAR, which would confuse the g_strsplit parser). */
	gchar *esc = xpad_text_buffer_escape_segment (TAG_UTF8);
	g_assert_cmpstr (esc, ==, ESC_UTF8 PH_UTF8);
	/* The encoded form must not contain a bare TAG_CHAR. */
	g_assert_null (strstr (esc, TAG_UTF8));
	g_free (esc);
}

static void
test_esc_char_escaped (void)
{
	/* A literal U+E001 in user text must become ESC_CHAR ESC_CHAR on disk. */
	gchar *esc = xpad_text_buffer_escape_segment (ESC_UTF8);
	g_assert_cmpstr (esc, ==, ESC_UTF8 ESC_UTF8);
	g_free (esc);
}

static void
test_round_trip_plain (void)
{
	const gchar *in = "simple note text";
	gchar *esc  = xpad_text_buffer_escape_segment (in);
	gchar *back = xpad_text_buffer_unescape_segment (esc);
	g_assert_cmpstr (back, ==, in);
	g_free (esc);
	g_free (back);
}

static void
test_round_trip_with_tag_char (void)
{
	/* Core data-safety case: user text with literal U+E000 round-trips losslessly. */
	const gchar *in = "note" TAG_UTF8 "with delimiter" TAG_UTF8 "embedded";
	gchar *esc  = xpad_text_buffer_escape_segment (in);
	gchar *back = xpad_text_buffer_unescape_segment (esc);
	g_assert_cmpstr (back, ==, in);
	/* Encoded form must contain no bare TAG_CHAR (g_strsplit safety). */
	g_assert_null (strstr (esc, TAG_UTF8));
	g_free (esc);
	g_free (back);
}

static void
test_round_trip_with_esc_char (void)
{
	const gchar *in = "escape" ESC_UTF8 "introducer" ESC_UTF8 "twice";
	gchar *esc  = xpad_text_buffer_escape_segment (in);
	gchar *back = xpad_text_buffer_unescape_segment (esc);
	g_assert_cmpstr (back, ==, in);
	g_free (esc);
	g_free (back);
}

static void
test_round_trip_mixed (void)
{
	/* Both special chars interleaved with regular text. */
	const gchar *in = TAG_UTF8 ESC_UTF8 "abc" TAG_UTF8 TAG_UTF8 ESC_UTF8;
	gchar *esc  = xpad_text_buffer_escape_segment (in);
	gchar *back = xpad_text_buffer_unescape_segment (esc);
	g_assert_cmpstr (back, ==, in);
	g_free (esc);
	g_free (back);
}

static void
test_legacy_no_escape (void)
{
	/* Existing saved notes have no escape sequences — unescaping them must
	 * be a no-op (backward compatibility). */
	const gchar *stored = "plain old note without any PUA chars";
	gchar *back = xpad_text_buffer_unescape_segment (stored);
	g_assert_cmpstr (back, ==, stored);
	g_free (back);
}

static void
test_legacy_literal_esc_char (void)
{
	/* Regression: a pre-existing note containing a BARE literal U+E001 (never
	 * written by our encoder) followed by ordinary text must survive unescape
	 * unchanged — the decoder must not treat it as an escape and drop it. */
	const gchar *stored = "before" ESC_UTF8 "after";
	gchar *back = xpad_text_buffer_unescape_segment (stored);
	g_assert_cmpstr (back, ==, stored);
	g_free (back);

	/* Trailing lone ESC_CHAR is also preserved. */
	const gchar *tail = "text" ESC_UTF8;
	gchar *back2 = xpad_text_buffer_unescape_segment (tail);
	g_assert_cmpstr (back2, ==, tail);
	g_free (back2);
}

static void
test_empty_string (void)
{
	gchar *esc  = xpad_text_buffer_escape_segment ("");
	gchar *back = xpad_text_buffer_unescape_segment (esc);
	g_assert_cmpstr (back, ==, "");
	g_free (esc);
	g_free (back);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/escape/plain_unchanged",       test_plain_text_unchanged);
	g_test_add_func ("/escape/tag_char_escaped",      test_tag_char_escaped);
	g_test_add_func ("/escape/esc_char_escaped",      test_esc_char_escaped);
	g_test_add_func ("/escape/round_trip_plain",      test_round_trip_plain);
	g_test_add_func ("/escape/round_trip_tag_char",   test_round_trip_with_tag_char);
	g_test_add_func ("/escape/round_trip_esc_char",   test_round_trip_with_esc_char);
	g_test_add_func ("/escape/round_trip_mixed",      test_round_trip_mixed);
	g_test_add_func ("/escape/legacy_no_escape",      test_legacy_no_escape);
	g_test_add_func ("/escape/legacy_literal_esc",    test_legacy_literal_esc_char);
	g_test_add_func ("/escape/empty_string",          test_empty_string);

	return g_test_run ();
}
