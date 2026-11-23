/* hai - the CLI. Numbered menu (nav standard), then the FSM chat engine for each line.
 * Same input rules as the window (window/hai_send.c): /teach w = meaning, /alias w = concept, /say w = text, /forget w, /pack name,
 * /mark w [high|normal], /grade N (or "grade N"), /bank (what hai knows), /why (how the last reply was decided). Anything else is one chat turn.
 * After every turn the WHY row of that turn is shown under the reply: nothing is hidden.
 * Usage: hai [datadir] [ledger]   (defaults: chat/data and chat.ledger in the current folder)
 *        printf 'hi\nbo\n2\n' | hai chat/data chat.ledger   (piped) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hai_chat_core.h"

/* the text of the last WHY row in the ledger (empty when there is none) */
static void last_why(const char *ledger, char *w, size_t cap) {
    FILE *f = fopen(ledger, "r"); char line[2048]; w[0] = 0;
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "WHY|", 4) != 0) continue;
        char *p = strchr(line + 4, '|'); if (p) p = strchr(p + 1, '|');
        if (p) { snprintf(w, cap, "%s", p + 1); w[strcspn(w, "\r\n")] = 0; }
    }
    fclose(f);
}

int main(int argc, char **argv) {
    const char *datadir = argc > 1 ? argv[1] : "chat/data";
    const char *ledger = argc > 2 ? argv[2] : "chat.ledger";
    char line[1024], out[65536], why[1200];
    printf("hai 0.1.0\n[1] Help\n[2] Quit\nType a number, or words. Teach it: /teach word = meaning   /alias word = concept   /say word = text   /forget word   /pack lyfedesk\n"
           "Mark a word that matters: /mark word.   Grade the last reply: /grade 0 to 4.   See what it knows: /bank   See why it answered: /why\n");
    while (printf("hai> "), fflush(stdout), fgets(line, sizeof line, stdin)) {
        int rc;
        char *t = line; while (*t == ' ') t++;
        t[strcspn(t, "\r\n")] = 0;
        if (!*t) continue;
        if (strncmp(t, "/grade ", 7) == 0 || strncmp(t, "grade ", 6) == 0) {
            char *s = t + (t[0] == '/' ? 7 : 6); while (*s == ' ') s++;
            if (*s < '0' || *s > '9') { printf("type grade 0 to 4\n"); fflush(stdout); continue; }
            rc = hai_chat_grade(datadir, ledger, atoi(s), (long)time(NULL), out, sizeof out);
            printf("%s\n", out); fflush(stdout); continue;
        }
        if (strcmp(t, "/bank") == 0) { hai_chat_bank(datadir, ledger, out, sizeof out); fputs(out, stdout); fflush(stdout); continue; }
        if (strcmp(t, "/why") == 0) { last_why(ledger, why, sizeof why); printf("%s\n", why[0] ? why : "no reply yet"); fflush(stdout); continue; }
        {   /* the teaching levers */
            static const char *cmds[][2] = { {"/teach ", "define"}, {"/alias ", "alias"}, {"/say ", "say"}, {"/forget ", "forget"}, {"/pack ", "pack"} };
            int handled = 0;
            for (unsigned i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
                size_t n0 = strlen(cmds[i][0]);
                if (strncmp(t, cmds[i][0], n0) != 0) continue;
                char a1[256] = "", a2[600] = ""; const char *rest = t + n0; const char *eq = strstr(rest, " = ");
                if (eq) { snprintf(a1, sizeof a1, "%.*s", (int)(eq - rest), rest); snprintf(a2, sizeof a2, "%s", eq + 3); } else snprintf(a1, sizeof a1, "%s", rest);
                hai_chat_cmd(datadir, ledger, cmds[i][1], a1, a2, (long)time(NULL), out, sizeof out);
                printf("%s\n", out); fflush(stdout); handled = 1; break;
            }
            if (handled) continue;
        }
        if (strncmp(t, "/mark ", 6) == 0) {
            char word[64] = "", level[16] = "high";
            if (sscanf(t + 6, "%63s %15s", word, level) < 1) { printf("usage: /mark <word> [high|normal]\n"); continue; }
            hai_chat_mark(datadir, ledger, word, level, (long)time(NULL), out, sizeof out);
            printf("%s\n", out); fflush(stdout); continue;
        }
        rc = hai_chat_turn(datadir, ledger, t, out, sizeof out);
        printf("%s\n", out);
        last_why(ledger, why, sizeof why);
        if (why[0]) printf("  why: %s\n", why);
        fflush(stdout);
        if (rc != 0) break;
    }
    return 0;
}
