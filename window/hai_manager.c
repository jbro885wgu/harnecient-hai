/* hai_manager - the <module> behind the hai-cli window (W1: it publishes; typing goes through hai_send).
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

#define ROWMAX 12
static volatile sig_atomic_t stop = 0;
static void on_term(int sig) { (void)sig; stop = 1; }

/* Look a reply id up in <pkg>/../chat/data/replies.pdl ("REPLY | id | text"); fills {name}. Returns 1 if found. */
static int reply_text(const char *pkg, const char *id, const char *name, char *out, size_t cap) {
    char path[1100]; snprintf(path, sizeof path, "%s/../chat/data/replies.pdl", pkg);
    FILE *f = fopen(path, "r"); if (!f) return 0;
    char line[1024]; int found = 0;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "REPLY |", 7) != 0) continue;
        char *a = line + 7; while (*a == ' ') a++;
        char *bar = strchr(a, '|'); if (!bar) continue;
        char *e = bar; while (e > a && e[-1] == ' ') e--;
        if ((size_t)(e - a) != strlen(id) || strncmp(a, id, (size_t)(e - a)) != 0) continue;
        char *t = bar + 1; while (*t == ' ') t++;
        t[strcspn(t, "\r\n")] = 0;
        char *nm = strstr(t, "{name}");
        if (nm && name && name[0]) snprintf(out, cap, "%.*s%s%s", (int)(nm - t), t, name, nm + 6);
        else snprintf(out, cap, "%s", t);
        found = 1; break;
    }
    fclose(f); return found;
}

static void publish(const char *pkg) {
    char ledger[1024], ui[1024], tmp[1100], state[160];
    snprintf(ledger, sizeof ledger, "%s/state/chat.ledger", pkg);
    snprintf(ui, sizeof ui, "%s/state/ui.txt", pkg);
    snprintf(tmp, sizeof tmp, "%s.tmp", ui);
    hai_chat_state(ledger, state, sizeof state);            /* "state=... name=..." */

    FILE *f = fopen(ledger, "r");
    char rows[ROWMAX][256]; int nrows = 0, nturns = 0;
    if (f) {
        char line[2048];
        while (fgets(line, sizeof line, f)) {
            int is_turn = strncmp(line, "TURN|", 5) == 0, is_grade = strncmp(line, "GRADE|", 6) == 0;
            if (!is_turn && !is_grade) continue;
            line[strcspn(line, "\r\n")] = 0;
            char *fld[12]; int n = 0; char *p = line;
            fld[n++] = p;
            while (*p && n < 12) { if (*p == '|') { *p = 0; fld[n++] = p + 1; } p++; }
            if (is_turn && n >= 5) {
                /* TURN|ts|input|concept|reply|state_after|action|typed_name|confidence (the confidence field is missing on older rows) */
                if (n >= 9 && fld[8][0]) snprintf(rows[nrows % ROWMAX], sizeof rows[0], "you: %s  ->  %s  (%s%%)", fld[2], fld[4], fld[8]);
                else snprintf(rows[nrows % ROWMAX], sizeof rows[0], "you: %s  ->  %s", fld[2], fld[4]);
                nrows++; nturns++;
                char rt[200];   /* the sentence hai answered, from replies.pdl (not stored in the ledger, only its id is) */
                if (reply_text(pkg, fld[4], n >= 8 ? fld[7] : "", rt, sizeof rt)) { snprintf(rows[nrows % ROWMAX], sizeof rows[0], "    hai: %s", rt); nrows++; }
            } else if (is_grade && n >= 6) {
                /* GRADE|ts|turn_no|concept|reply|grade|reward|punish|day */
                snprintf(rows[nrows % ROWMAX], sizeof rows[0], "    graded %s  (%s)", fld[5], fld[4]);
                nrows++;
            }
        }
        fclose(f);
    }
    FILE *o = fopen(tmp, "w");
    if (!o) return;
    char stamp[32]; time_t now = time(NULL); strftime(stamp, sizeof stamp, "%H:%M:%S", localtime(&now));
    fprintf(o, "stamp=%s\n%s\n", stamp, state);
    int shown = nrows < ROWMAX ? nrows : ROWMAX;
    fprintf(o, "turns=%d\n", nturns);
    for (int i = 0; i < shown; i++) {
        int idx = nrows < ROWMAX ? i : (nrows - ROWMAX + i) % ROWMAX;
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
        sleep(1);
    }
    return 0;
}
