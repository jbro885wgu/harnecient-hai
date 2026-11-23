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

#define ROWMAX 14
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

/* Replace every {key} with val (the teach loop's reply texts carry {word} and {meaning}). */
static void fill_key(char *text, size_t cap, const char *key, const char *val) {
    char pat[40]; snprintf(pat, sizeof pat, "{%s}", key);
    char *p = strstr(text, pat);
    while (p) {
        char tmp[1100]; size_t pre = (size_t)(p - text);
        snprintf(tmp, sizeof tmp, "%.*s%s%s", (int)pre, text, val, p + strlen(pat));
        snprintf(text, cap, "%.*s", (int)cap - 1, tmp);
        p = strstr(text + pre + strlen(val), pat);
    }
}

static void publish(const char *pkg) {
    char ledger[1024], ui[1024], tmp[1100], state[160];
    snprintf(ledger, sizeof ledger, "%s/state/chat.ledger", pkg);
    snprintf(ui, sizeof ui, "%s/state/ui.txt", pkg);
    snprintf(tmp, sizeof tmp, "%s.tmp", ui);
    hai_chat_state(ledger, state, sizeof state);            /* "state=... name=..." */

    int said_mode = 0;                       /* a ledger written since 2026-11-23 records what hai said (SAID rows); an older one is looked up in replies.pdl */
    { FILE *pf = fopen(ledger, "r"); if (pf) { char l2[2048]; while (fgets(l2, sizeof l2, pf)) if (strncmp(l2, "SAID|", 5) == 0) { said_mode = 1; break; } fclose(pf); } }
    FILE *f = fopen(ledger, "r");
    char rows[ROWMAX][320]; int nrows = 0, nturns = 0;
    static char tw[32][64], tm[32][200]; int ntw = 0;       /* words taught so far, rebuilt from the ledger on every pass */
    if (f) {
        char line[2048];
        while (fgets(line, sizeof line, f)) {
            int is_turn = strncmp(line, "TURN|", 5) == 0, is_grade = strncmp(line, "GRADE|", 6) == 0, is_mark = strncmp(line, "MARK|", 5) == 0;
            int is_said = strncmp(line, "SAID|", 5) == 0;
            const char *cmdname = strncmp(line, "DEFINE|", 7) == 0 ? "/teach" : strncmp(line, "ALIAS|", 6) == 0 ? "/alias" : strncmp(line, "SAY|", 4) == 0 ? "/say" :
                                  strncmp(line, "FORGET|", 7) == 0 ? "/forget" : strncmp(line, "PACK|", 5) == 0 ? "/pack" : NULL;
            if (!is_turn && !is_grade && !is_mark && !is_said && !cmdname) continue;
            line[strcspn(line, "\r\n")] = 0;
            char *fld[12]; int n = 0; char *p = line;
            fld[n++] = p;
            while (*p && n < 12) { if (*p == '|') { *p = 0; fld[n++] = p + 1; } p++; }
            if (is_turn && n >= 5) {
                /* TURN|ts|input|concept|reply|state_after|action|typed_name|confidence (the confidence field is missing on older rows) */
                if (n >= 9 && fld[8][0]) snprintf(rows[nrows % ROWMAX], sizeof rows[0], "you: %s  ->  %s  (%s%%)", fld[2], fld[4], fld[8]);
                else snprintf(rows[nrows % ROWMAX], sizeof rows[0], "you: %s  ->  %s", fld[2], fld[4]);
                nrows++; nturns++;
                /* teach loop: store_meaning rows teach a word (typed word = field 7, the meaning = the typed line, field 2) */
                if (n >= 8 && strcmp(fld[6], "store_meaning") == 0 && fld[7][0]) {
                    int k = -1; for (int i = 0; i < ntw; i++) if (strcmp(tw[i], fld[7]) == 0) k = i;
                    if (k < 0 && ntw < 32) k = ntw++;
                    if (k >= 0) { snprintf(tw[k], sizeof tw[k], "%s", fld[7]); snprintf(tm[k], sizeof tm[k], "%s", fld[2]); }
                }
                char rt[300];   /* the sentence hai answered, from replies.pdl (not stored in the ledger, only its id is) */
                if (!said_mode && reply_text(pkg, fld[4], n >= 8 ? fld[7] : "", rt, sizeof rt)) {
                    const char *wd = n >= 8 ? fld[7] : "", *mn = fld[2];
                    if (strncmp(fld[3], "taught:", 7) == 0) { wd = fld[3] + 7; mn = ""; for (int i = 0; i < ntw; i++) if (strcmp(tw[i], wd) == 0) mn = tm[i]; }
                    fill_key(rt, sizeof rt, "word", wd); fill_key(rt, sizeof rt, "meaning", mn); snprintf(rows[nrows % ROWMAX], sizeof rows[0], "    hai: %s", rt); nrows++; }
            } else if (is_said && n >= 4) {
                snprintf(rows[nrows % ROWMAX], sizeof rows[0], "    hai: %s", fld[3]); nrows++;
            } else if (cmdname && n >= 3) {
                if (n >= 5 && strncmp(fld[4], "pack:", 5) == 0) continue;           /* rows a /pack wrote: shown as the one /pack row */
                if (strcmp(cmdname, "/pack") == 0 || strcmp(cmdname, "/forget") == 0) snprintf(rows[nrows % ROWMAX], sizeof rows[0], "you: %s %s", cmdname, fld[2]);
                else snprintf(rows[nrows % ROWMAX], sizeof rows[0], "you: %s %s = %s", cmdname, fld[2], n >= 4 ? fld[3] : "");
                nrows++;
            } else if (is_mark && n >= 4) {
                snprintf(rows[nrows % ROWMAX], sizeof rows[0], "you: /mark %s %s", fld[2], fld[3]);
                nrows++;
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
