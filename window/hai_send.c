/* hai_send - the action behind the hai-cli window's <cli_io id="composer"> (W1: typing in the window).
 * The renderer runs it as:  hai_send <package_dir> <house_root> <typed text>   (same contract as chat-hai's ch_send.sh).
 * <package_dir> is the window folder (window/); the ledger is <package_dir>/state/chat.ledger and the data is <package_dir>/../chat/data.
 * Typed text:  /grade N   grades the last reply (0-4);   anything else is one chat turn.
 * It writes ONLY ledger rows, through the same engine the CLI uses (hai_chat_turn / hai_chat_grade). Nothing else is stored:
 * the window's ui.txt is a projection the manager rebuilds from the ledger (no ghost data).
 * Exit: 0 ok or empty text, 2 setup missing, 3 ledger write failed, 4 bad grade, 5 no reply to grade yet, 6 daily cap, 7 already graded.
 * Build: gcc -std=gnu11 -Wall -Wextra -O2 -I../ops -o bin/hai_send hai_send.c ../ops/hai_chat_core.c */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hai_chat_core.h"

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: hai_send <package_dir> <house_root> <text>\n"); return 2; }
    const char *pkg = argv[1];
    char text[1024] = "";
    for (int i = 3; i < argc; i++) { if (i > 3) strncat(text, " ", sizeof text - strlen(text) - 1); strncat(text, argv[i], sizeof text - strlen(text) - 1); }
    char *t = text; while (*t == ' ') t++;
    size_t n = strlen(t); while (n > 0 && (t[n - 1] == ' ' || t[n - 1] == '\n' || t[n - 1] == '\r')) t[--n] = 0;
    if (!*t) return 0;
    char ledger[1100], data[1100], out[2048];
    snprintf(ledger, sizeof ledger, "%s/state/chat.ledger", pkg);
    snprintf(data, sizeof data, "%s/../chat/data", pkg);
    if (strncmp(t, "/grade ", 7) == 0) {
        char *end = NULL; long g = strtol(t + 7, &end, 10);
        if (end == t + 7 || *end) return 4;
        int rc = hai_chat_grade(data, ledger, (int)g, (long)time(NULL), out, sizeof out);
        fprintf(stderr, "%s\n", out);
        return rc;
    }
    int rc = hai_chat_turn(data, ledger, t, out, sizeof out);
    fprintf(stderr, "%s\n", out);
    return rc == 1 ? 0 : rc;   /* 1 = the chat finished (bye): not an error for the window */
}
