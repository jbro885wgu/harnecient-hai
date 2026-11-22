/* hai_manager - the <module> behind the hai window (W0, read-only).
 * The renderer execs this with argv: <house_root> <package_dir>. It loops, reads the chat
 * ledger through the SAME engine the CLI uses (hai_chat_state), and publishes state/ui.txt
 * (key=value lines, read by hai.xhtpm through vars=). It never draws; the renderer does.
 * Ledger: <package_dir>/state/chat.ledger. Exit on SIGTERM (the renderer sends it on close).
 * Build: gcc -std=gnu11 -Wall -Wextra -O2 -I../ops -o bin/hai_manager hai_manager.c ../ops/hai_chat_core.c */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "hai_chat_core.h"

static volatile sig_atomic_t stop = 0;
static void on_term(int sig) { (void)sig; stop = 1; }

static void publish(const char *pkg) {
    char ledger[1024], ui[1024], tmp[1100], state[160];
    snprintf(ledger, sizeof ledger, "%s/state/chat.ledger", pkg);
    snprintf(ui, sizeof ui, "%s/state/ui.txt", pkg);
    snprintf(tmp, sizeof tmp, "%s.tmp", ui);
    hai_chat_state(ledger, state, sizeof state);            /* "state=... name=..." */

    FILE *f = fopen(ledger, "r");
    char rows[8][256]; int nrows = 0;
    if (f) {
        char line[2048];
        while (fgets(line, sizeof line, f)) {
            if (strncmp(line, "TURN|", 5) != 0) continue;
            line[strcspn(line, "\r\n")] = 0;
            char *fld[8]; int n = 0; char *p = line;
            fld[n++] = p;
            while (*p && n < 8) { if (*p == '|') { *p = 0; fld[n++] = p + 1; } p++; }
            if (n < 5) continue;
            snprintf(rows[nrows % 8], sizeof rows[0], "you: %s  ->  %s", fld[2], fld[4]);
            nrows++;
        }
        fclose(f);
    }
    FILE *o = fopen(tmp, "w");
    if (!o) return;
    char stamp[32]; time_t now = time(NULL); strftime(stamp, sizeof stamp, "%H:%M:%S", localtime(&now));
    fprintf(o, "stamp=%s\n%s\n", stamp, state);
    int shown = nrows < 8 ? nrows : 8;
    fprintf(o, "turns=%d\n", nrows);
    for (int i = 0; i < shown; i++) {
        int idx = nrows < 8 ? i : (nrows - 8 + i) % 8;
        fprintf(o, "row_%d_text=%s\n", i, rows[idx]);
    }
    fprintf(o, "rows_count=%d\n", shown);
    fprintf(o, "no_rows=%d\n", shown == 0 ? 1 : 0);
    fclose(o);
    rename(tmp, ui);
}

int main(int argc, char **argv) {
    const char *pkg = argc > 2 ? argv[2] : ".";
    signal(SIGTERM, on_term); signal(SIGINT, on_term); signal(SIGHUP, on_term);
    while (!stop) {
        publish(pkg);
        sleep(2);
    }
    return 0;
}
