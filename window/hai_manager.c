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
#include "hai_active.h"
#include <dirent.h>
#include <stdlib.h>

#define ROWMAX 30
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

/* window preferences (state/ui_prefs.txt: left=1 right=1 view=chat why=1). They change what the window SHOWS, never what an agent knows. */
static void read_prefs(const char *pkg, int *left, int *right, int *bank, int *why) {
    *left = 1; *right = 1; *bank = 0; *why = 1;
    char path[1100], line[128]; snprintf(path, sizeof path, "%s/state/ui_prefs.txt", pkg);
    FILE *f = fopen(path, "r"); if (!f) return;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "left=", 5) == 0) *left = atoi(line + 5);
        else if (strncmp(line, "right=", 6) == 0) *right = atoi(line + 6);
        else if (strncmp(line, "why=", 4) == 0) *why = atoi(line + 4);
        else if (strncmp(line, "view=bank", 9) == 0) *bank = 1;
    }
    fclose(f);
}

static int cmp_name(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }

/* label of a session: the first thing the person typed in it (like a chat list), else its name */
static void session_label(const char *ledger, const char *name, char *out, size_t cap) {
    snprintf(out, cap, "%s", name);
    if (strcmp(name, "main") == 0) return;
    FILE *f = fopen(ledger, "r"); if (!f) return;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "TURN|", 5) != 0) continue;
        char *p = strchr(line + 5, '|'); if (!p) continue; p++;
        char *e = strchr(p, '|'); if (!e) continue; *e = 0;
        snprintf(out, cap, "%.24s", p); break;
    }
    fclose(f);
}

static void publish(const char *pkg) {
    char ledger[1024], ui[1024], tmp[1100], state[160];
    hai_active_ledger(pkg, ledger, sizeof ledger);
    snprintf(ui, sizeof ui, "%s/state/ui.txt", pkg);
    snprintf(tmp, sizeof tmp, "%s.tmp", ui);
    hai_chat_state(ledger, state, sizeof state);            /* "state=... name=..." */

    int pl, pr, pbank, show_why; read_prefs(pkg, &pl, &pr, &pbank, &show_why);
    int said_mode = 0;                       /* a ledger written since 2026-11-23 records what hai said (SAID rows); an older one is looked up in replies.pdl */
    { FILE *pf = fopen(ledger, "r"); if (pf) { char l2[2048]; while (fgets(l2, sizeof l2, pf)) if (strncmp(l2, "SAID|", 5) == 0) { said_mode = 1; break; } fclose(pf); } }
    FILE *f = fopen(ledger, "r");
    char rows[ROWMAX][360]; int nrows = 0, nturns = 0;
    static char tw[32][64], tm[32][200]; int ntw = 0;       /* words taught so far, rebuilt from the ledger on every pass */
    if (f) {
        char line[2048];
        while (fgets(line, sizeof line, f)) {
            int is_turn = strncmp(line, "TURN|", 5) == 0, is_grade = strncmp(line, "GRADE|", 6) == 0, is_mark = strncmp(line, "MARK|", 5) == 0;
            int is_said = strncmp(line, "SAID|", 5) == 0, is_why = strncmp(line, "WHY|", 4) == 0;
            const char *cmdname = strncmp(line, "DEFINE|", 7) == 0 ? "/teach" : strncmp(line, "ALIAS|", 6) == 0 ? "/alias" : strncmp(line, "SAY|", 4) == 0 ? "/say" :
                                  strncmp(line, "FORGET|", 7) == 0 ? "/forget" : strncmp(line, "PACK|", 5) == 0 ? "/pack" : NULL;
            if (!is_turn && !is_grade && !is_mark && !is_said && !is_why && !cmdname) continue;
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
            } else if (is_why && n >= 4) {
                if (!show_why) continue;
                snprintf(rows[nrows % ROWMAX], sizeof rows[0], "    why: %s", fld[3]); nrows++;
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
        /* colour class: the person, hai, or a lever/grade note (css in hai.css) */
        const char *cls = strncmp(rows[idx], "    why:", 8) == 0 ? "chat-why" : strncmp(rows[idx], "    hai:", 8) == 0 ? "chat-hai" : strncmp(rows[idx], "you: /", 6) == 0 ? "chat-note" : strncmp(rows[idx], "you:", 4) == 0 ? "chat-you" : "chat-note";
        fprintf(o, "row_%d_text=%s\nrow_%d_cls=%s\n", i, rows[idx], i, cls);
    }
    fprintf(o, "rows_count=%d\n", shown);
    fprintf(o, "no_rows=%d\n", shown == 0 ? 1 : 0);
    {   /* layout flags (preferences), and the Bank view: everything the active agent knows, from the core's own replay */
        char act0[64]; hai_agent_active(pkg, act0, sizeof act0);
        fprintf(o, "left_open=%d\nright_open=%d\nbank_on=%d\nchat_on=%d\nwhy_on=%d\n", pl, pr, pbank, !pbank, show_why);
        fprintf(o, "left_closed=%d\nleft_cls=%s\n", !pl, pl ? "hai-left-on" : "hai-left-off");   /* the sidebar element must always exist (the layout trigger), so closing it only narrows it */
        fprintf(o, "pad_cls=%s\n", pl && pr ? "pad-lr" : pl ? "pad-l" : pr ? "pad-r" : "pad-none");
        fprintf(o, "tg_left_cls=%s\ntg_right_cls=%s\ntg_bank_cls=%s\ntg_why_cls=%s\n", pl ? "hai-on" : "hai-off", pr ? "hai-on" : "hai-off", pbank ? "hai-on" : "hai-off", show_why ? "hai-on" : "hai-off");
        char dd[1200], lg[1300]; hai_data_of(pkg, act0, dd, sizeof dd); hai_active_ledger(pkg, lg, sizeof lg);
        static char bank[65536]; int nb = 0, ncon = 0, ntau = 0, nali = 0;
        if (hai_chat_bank(dd, lg, bank, sizeof bank) == 0) {
            char *save = NULL;
            for (char *ln = strtok_r(bank, "\n", &save); ln && nb < 120; ln = strtok_r(NULL, "\n", &save)) {
                char *f[6]; int nf = 0; char *p = ln; f[nf++] = p; while (*p && nf < 6) { if (*p == '|') { *p = 0; f[nf++] = p + 1; } p++; }
                char text[400] = "", cl[16] = "bank-note";
                if (!strcmp(f[0], "CONCEPT") && nf >= 5) { snprintf(text, sizeof text, "meaning  %s  =  %s   (trust %s)", f[1], f[2], f[4]); snprintf(cl, sizeof cl, "bank-core"); ncon++; }
                else if (!strcmp(f[0], "TAUGHT") && nf >= 5) { snprintf(text, sizeof text, "taught  %s  =  %s%s   (trust %s)", f[1], f[2], f[3][0] == '1' ? "   [marked]" : "", f[4]); snprintf(cl, sizeof cl, "bank-learned"); ntau++; }
                else if (!strcmp(f[0], "ALIAS") && nf >= 3) { snprintf(text, sizeof text, "alias  %s  means the same as  %s", f[1], f[2]); snprintf(cl, sizeof cl, "bank-learned"); nali++; }
                else if (!strcmp(f[0], "SAY") && nf >= 3) { snprintf(text, sizeof text, "answer  when you say %s:  %s", f[1], f[2]); snprintf(cl, sizeof cl, "bank-learned"); }
                else continue;
                fprintf(o, "bk_%d_text=%s\nbk_%d_cls=%s\n", nb, text, nb, cl); nb++;
            }
        }
        fprintf(o, "bank_count=%d\nbank_title=Bank of %s: %d built-in meanings, %d taught words, %d aliases (the ledger replayed: exactly what the next reply will use)\n", nb, act0, ncon, ntau, nali);
    }
    {   /* sidebar: the agents (each a folder with its own corpus and personality) and the active agent's sessions */
        char act[64], asess[64]; hai_agent_active(pkg, act, sizeof act); hai_session_active(pkg, act, asess, sizeof asess);
        char names[32][64]; int na = 0; snprintf(names[na++], 64, "hai");
        char adir[1200]; snprintf(adir, sizeof adir, "%s/../chat/agents", pkg);
        DIR *d = opendir(adir); struct dirent *e;
        if (d) { char tmpn[24][64]; int nt = 0; while ((e = readdir(d)) && nt < 24) if (e->d_name[0] != '.' && hai_name_ok(e->d_name)) snprintf(tmpn[nt++], 64, "%s", e->d_name);
                 closedir(d); qsort(tmpn, (size_t)nt, 64, cmp_name); for (int i = 0; i < nt && na < 32; i++) snprintf(names[na++], 64, "%s", tmpn[i]); }
        fprintf(o, "agent=%s\nagents_count=%d\n", act, na);
        for (int i = 0; i < na; i++) {
            int on = strcmp(names[i], act) == 0; char card[1400], persona[64] = "";
            if (strcmp(names[i], "hai") == 0) snprintf(persona, sizeof persona, "original");
            else { snprintf(card, sizeof card, "%s/%s/agent.pdl", adir, names[i]); FILE *c = fopen(card, "r");
                   if (c) { char ln[256], a1[64], a2[64]; if (fgets(ln, sizeof ln, c) && sscanf(ln, "AGENT | %63s | %63s", a1, a2) == 2) snprintf(persona, sizeof persona, "%s", a2); fclose(c); } }
            fprintf(o, "agent_%d_name=%s\nagent_%d_label=%s%s  (%s)\nagent_%d_cls=%s\n", i, names[i], i, on ? "> " : "  ", names[i], persona, i, on ? "session-item session-active" : "session-item");
        }
        char sroot[1200], sdir[1300]; hai_root(pkg, act, sroot, sizeof sroot); snprintf(sdir, sizeof sdir, "%s/sessions", sroot);
        char sn[40][64]; int ns = 0; snprintf(sn[ns++], 64, "main");
        d = opendir(sdir);
        if (d) { char tmps[36][64]; int nt = 0; while ((e = readdir(d)) && nt < 36) { size_t L = strlen(e->d_name); if (L > 7 && strcmp(e->d_name + L - 7, ".ledger") == 0) { snprintf(tmps[nt], 64, "%.*s", (int)(L - 7), e->d_name); if (hai_name_ok(tmps[nt])) nt++; } }
                 closedir(d); qsort(tmps, (size_t)nt, 64, cmp_name); for (int i = 0; i < nt && ns < 40; i++) snprintf(sn[ns++], 64, "%s", tmps[i]); }
        fprintf(o, "sessions_count=%d\n", ns);
        for (int i = 0; i < ns; i++) {
            char lp[1500], lab[64]; hai_session_ledger(pkg, act, sn[i], lp, sizeof lp); session_label(lp, sn[i], lab, sizeof lab);
            int on = strcmp(sn[i], asess) == 0;
            fprintf(o, "session_%d_name=%s\nsession_%d_label=%s%s\nsession_%d_cls=%s\n", i, sn[i], i, on ? "> " : "  ", lab, i, on ? "session-item session-active" : "session-item");
        }
    }
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
