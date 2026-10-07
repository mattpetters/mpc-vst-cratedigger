/* Host test for vst/query_edit.h -- the search box's text buffer.
 *
 * The plugin's own wrapper needs an MPC to run, but the text buffer is plain C: this checks the
 * typing/cursor/backspace behaviour off the device (same idea as mpc-vst-plugins' tools/host_test.c
 * for its ports; a hand-written wrapper keeps its own test).
 *
 *   cc -std=c11 -Wall -Wextra -o /tmp/test_query_edit vst/test_query_edit.c && /tmp/test_query_edit
 *
 * Prints PASS/FAIL per check and exits non-zero on the first failure count > 0.
 */
#include <stdio.h>
#include <string.h>

#include "query_edit.h"

static int checks, failures;

static void check(const char *what, int ok, const char *detail) {
    checks++;
    if (ok) {
        printf("PASS %s\n", what);
    } else {
        failures++;
        printf("FAIL %s -- %s\n", what, detail ? detail : "");
    }
}

static const char *text_of(const char *s, int pos) {
    static char buf[QE_LEN * 4];
    qe_display(s, pos, buf, (int)sizeof buf);
    return buf;
}

int main(void) {
    char q[QE_LEN];
    int pos = 0;
    char detail[256];

    /* typing appends and the cursor follows */
    qe_clear(q);
    const char *typed = "ROY AYERS";
    for (const char *c = typed; *c; c++) qe_type(q, &pos, *c);
    snprintf(detail, sizeof detail, "len=%d text='%.*s'", qe_len(q), qe_len(q), q);
    check("typed text lands in the buffer", strncmp(q, typed, qe_len(q)) == 0 && qe_len(q) == 9, detail);
    check("cursor sits past the text after typing", pos == 9, detail);
    snprintf(detail, sizeof detail, "display='%s'", text_of(q, pos));
    check("display brackets the cursor at the end", strcmp(text_of(q, pos), "ROY AYERS[_]") == 0, detail);

    /* cursor left, overwrite in place */
    pos = 0;
    qe_type(q, &pos, 'B');
    check("typing over a character replaces it", strncmp(q, "BOY AYERS", 9) == 0, q);
    check("cursor advanced one cell", pos == 1, "pos != 1");
    snprintf(detail, sizeof detail, "display='%s'", text_of(q, pos));
    check("display brackets the cursor in place", strcmp(text_of(q, pos), "B[O]Y AYERS") == 0, detail);

    /* backspace closes the gap */
    qe_backspace(q, &pos);
    snprintf(detail, sizeof detail, "text='%.*s' pos=%d", qe_len(q), q, pos);
    check("backspace removes the character before the cursor", strncmp(q, "OY AYERS", 8) == 0 && pos == 0, detail);
    check("length follows", qe_len(q) == 8, detail);
    qe_backspace(q, &pos);
    check("backspace at the start of the text is a no-op", pos == 0 && qe_len(q) == 8, detail);

    /* clear, then a full buffer + the one-past-end cursor case */
    qe_clear(q);
    pos = 0;
    check("clear empties the buffer and homes the cursor", qe_len(q) == 0 && pos == 0, "not empty");
    check("empty display is a lone marker", strcmp(text_of(q, 0), "[_]") == 0, text_of(q, 0));
    for (int i = 0; i < QE_LEN + 5; i++) qe_type(q, &pos, (char)('A' + (i % 26)));
    check("typing past the last slot keeps the buffer full, not longer",
          qe_len(q) == QE_LEN && pos == QE_LEN - 1, "overflow");
    snprintf(detail, sizeof detail, "pos=%d len=%d", pos, qe_len(q));
    check("cursor cannot leave the buffer", pos >= 0 && pos < QE_LEN, detail);

    /* a space in the middle is a normal character */
    qe_clear(q);
    pos = 0;
    const char *sp = "SPACE HERE";
    for (const char *c = sp; *c; c++) qe_type(q, &pos, *c);
    check("spaces survive in the middle", strncmp(q, sp, 10) == 0 && qe_len(q) == 10, q);

    /* set (MORE BY ARTIST): replaces the text, cursor at the end, junk dropped */
    qe_set(q, &pos, "Roy Ayers Ubiquity");
    snprintf(detail, sizeof detail, "text='%.*s' pos=%d", qe_len(q), q, pos);
    check("set replaces the whole text", qe_len(q) == 18 && strncmp(q, "Roy Ayers Ubiquity", 18) == 0, detail);
    check("set leaves the cursor at the end", pos == 18, detail);
    check("set display shows the whole text",
          strcmp(text_of(q, pos), "Roy Ayers Ubiquity[_]") == 0, text_of(q, pos));
    qe_set(q, &pos, "ab\ncd\tef");
    snprintf(detail, sizeof detail, "text='%.*s'", qe_len(q), q);
    check("control characters are dropped", strcmp(q, "abcdef") == 0, detail);
    qe_set(q, &pos, "x");
    qe_backspace(q, &pos);
    check("backspace after set empties the buffer", pos == 0 && qe_len(q) == 0, q);

    /* non-printable input is refused */
    qe_clear(q);
    pos = 0;
    qe_type(q, &pos, '\n');
    qe_type(q, &pos, 'A');
    qe_type(q, &pos, (char)200);
    check("non-printable keys do nothing", qe_len(q) == 1 && q[0] == 'A' && pos == 1, q);

    /* a 64-character query (the daemon's ceiling) cannot be typed: the buffer caps at QE_LEN */
    qe_set(q, &pos, "0123456789012345678901234567890123456789012345678901234567890123");
    check("a long set truncates at the buffer size", qe_len(q) == QE_LEN && pos == QE_LEN - 1, "truncation");

    printf("\n%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
