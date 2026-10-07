/* =============================================================================
 * query_edit.h - the search box's text buffer for Crate Digger, as plain functions.
 *
 * The MPC plugin host offers no text entry at all (mpc-vst-plugins docs/NOTES.md: "No text entry
 * on the page"; a plugin only ever gets string data out via a parameter's display text), so the
 * search text is typed one key at a time from a key grid in the skin. That makes this little
 * buffer the plugin's whole keyboard -- which is why it lives in its own header, with its own
 * host test (vst/test_query_edit.c): it is the one piece of the wrapper that can be checked off
 * the device.
 *
 * Fixed slots, one character each, with a cursor over them:
 *   - a slot holding '\0' is empty; the text is the slots up to the last non-empty one, and the
 *     slots always stay left-packed (type overwrites at the cursor, backspace closes the gap),
 *   - the cursor is an index 0..QE_LEN-1, and may sit one past the last character (appending) or
 *     on a character (overwriting / deleting the one before it),
 *   - the skin has no caret glyph, so qe_display() marks the cursor with brackets: "ROY [A]YERS".
 *
 * Charset: printable ASCII only (32..126). The key grid in vst/params.json can only produce a
 * subset of that, so this is a guard against a hand-written chunk/filter rather than a UI filter.
 * ========================================================================== */
#ifndef CRATEDIG_QUERY_EDIT_H
#define CRATEDIG_QUERY_EDIT_H

#define QE_LEN 32   /* slots = the longest search text the plugin can hold */

static int qe_ok(char c) { return c >= 32 && c < 127; }

/* Number of characters in the buffer (first empty slot). */
static int qe_len(const char *s) {
    int n = 0;
    while (n < QE_LEN && s[n]) n++;
    return n;
}

static void qe_clear(char *s) {
    for (int i = 0; i < QE_LEN; i++) s[i] = 0;
}

/* Type one character AT the cursor and step right (an append when the cursor is past the text). */
static void qe_type(char *s, int *pos, char c) {
    if (!qe_ok(c)) return;
    if (*pos < 0) *pos = 0;
    if (*pos > QE_LEN - 1) *pos = QE_LEN - 1;
    s[*pos] = c;
    if (*pos < QE_LEN - 1) (*pos)++;
}

/* Delete the character before the cursor (backspace); a no-op at the start of the text. */
static void qe_backspace(char *s, int *pos) {
    if (!s || !pos || *pos <= 0) return;
    for (int i = *pos - 1; i < QE_LEN - 1; i++) s[i] = s[i + 1];
    s[QE_LEN - 1] = 0;
    (*pos)--;
}

/* Replace the whole text (used by MORE BY ARTIST); the cursor lands at its end. */
static void qe_set(char *s, int *pos, const char *text) {
    qe_clear(s);
    int n = 0;
    for (const char *q = text; q && *q; q++) {
        if (!qe_ok(*q) || n >= QE_LEN) continue;
        s[n++] = *q;
    }
    *pos = n < QE_LEN ? n : QE_LEN - 1;
}

/* The text as the search readout shows it: the whole buffer, cursor cell in brackets, and a
 * lone [_] when the cursor is past the end. Returns the length written to out. */
static int qe_display(const char *s, int pos, char *out, int outlen) {
    int n = qe_len(s), o = 0;
    if (outlen <= 0) return 0;
    if (pos < 0) pos = 0;
    if (pos > QE_LEN - 1) pos = QE_LEN - 1;
    for (int i = 0; i < n && o < outlen - 1; i++) {
        if (i == pos) out[o++] = '[';
        if (o < outlen - 1) out[o++] = s[i];
        if (i == pos && o < outlen - 1) out[o++] = ']';
    }
    if (pos >= n) {
        if (o < outlen - 1) out[o++] = '[';
        if (o < outlen - 1) out[o++] = '_';
        if (o < outlen - 1) out[o++] = ']';
    }
    out[o] = 0;
    return o;
}

#endif /* CRATEDIG_QUERY_EDIT_H */
