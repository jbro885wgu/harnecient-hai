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
#include "hai_active.h"
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: hai_send <package_dir> <house_root> <text>\n"); return 2; }
    const char *pkg = argv[1];
    char text[1024] = "";
    for (int i = 3; i < argc; i++) { if (i > 3) strncat(text, " ", sizeof text - strlen(text) - 1); strncat(text, argv[i], sizeof text - strlen(text) - 1); }
    char *t = text; while (*t == ' ') t++;
    size_t n = strlen(t); while (n > 0 && (t[n - 1] == ' ' || t[n - 1] == '\n' || t[n - 1] == '\r')) t[--n] = 0;
    if (!*t) return 0;
    char ledger[1100], data[1100], out[2048];
    hai_active_ledger(pkg, ledger, sizeof ledger);       /* the active session’s ledger ("main" = state/chat.ledger) */
    hai_active_data(pkg, data, sizeof data);                 /* the active agent’s own data (corpus, replies, rules, packs) */
    if (strncmp(t, "/view ", 6) == 0 || strncmp(t, "/toggle ", 8) == 0 || strcmp(t, "/bank") == 0 || strcmp(t, "/chat") == 0) {   /* window preferences */
        char req[120]; if (!strcmp(t, "/bank")) snprintf(req, sizeof req, "VIEW bank"); else if (!strcmp(t, "/chat")) snprintf(req, sizeof req, "VIEW chat");
        else snprintf(req, sizeof req, "%s%.100s", strncmp(t, "/view ", 6) == 0 ? "VIEW " : "TOGGLE ", t + (strncmp(t, "/view ", 6) == 0 ? 6 : 8));
        char helper[1200]; snprintf(helper, sizeof helper, "%s/bin/hai_session", pkg);
        pid_t pid = fork(); if (pid == 0) { execl(helper, "hai_session", req, pkg, (char *)NULL); _exit(127); }
        int st = 0; if (pid > 0) waitpid(pid, &st, 0);
        return WIFEXITED(st) ? WEXITSTATUS(st) : 3;
    }
    if (strncmp(t, "/agent", 6) == 0 || strncmp(t, "/session", 8) == 0) {      /* /agent new <id> [persona] | /agent <id> | /agent delete <id> | /session new | /session <name> */
        char req[300] = "", a[64] = "", b[64] = "", c[64] = ""; int isagent = strncmp(t, "/agent", 6) == 0;
        sscanf(t + (isagent ? 6 : 8), "%63s %63s %63s", a, b, c);
        if (isagent && strcmp(a, "new") == 0) snprintf(req, sizeof req, "NEWAGENT %s %s", b, c);
        else if (isagent && strcmp(a, "delete") == 0) snprintf(req, sizeof req, "DELETEAGENT %s", b);
        else if (isagent && a[0]) snprintf(req, sizeof req, "AGENT %s", a);
        else if (!isagent && strcmp(a, "new") == 0) snprintf(req, sizeof req, "NEW");
        else if (!isagent && strcmp(a, "delete") == 0) snprintf(req, sizeof req, "DELETE %s", b);
        else if (!isagent && a[0]) snprintf(req, sizeof req, "SWITCH %s", a);
        else return 4;
        char helper[1200]; snprintf(helper, sizeof helper, "%s/bin/hai_session", pkg);
        pid_t pid = fork(); if (pid == 0) { execl(helper, "hai_session", req, pkg, (char *)NULL); _exit(127); }
        int st = 0; if (pid > 0) waitpid(pid, &st, 0);
        return WIFEXITED(st) ? WEXITSTATUS(st) : 3;
    }
    if (strncmp(t, "/grade ", 7) == 0) {
        char *end = NULL; long g = strtol(t + 7, &end, 10);
        if (end == t + 7 || *end) return 4;
        int rc = hai_chat_grade(data, ledger, (int)g, (long)time(NULL), out, sizeof out);
        fprintf(stderr, "%s\n", out);
        return rc;
    }
    {   /* the teaching levers: /teach w = meaning, /alias w = concept, /say w = text, /forget w, /pack name */
        static const char *cmds[][2] = { {"/teach ", "define"}, {"/alias ", "alias"}, {"/say ", "say"}, {"/forget ", "forget"}, {"/pack ", "pack"} };
        for (unsigned i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
            size_t n0 = strlen(cmds[i][0]);
            if (strncmp(t, cmds[i][0], n0) != 0) continue;
            char a1[256] = "", a2[600] = ""; const char *rest = t + n0; const char *eq = strstr(rest, " = ");
            if (eq) { snprintf(a1, sizeof a1, "%.*s", (int)(eq - rest), rest); snprintf(a2, sizeof a2, "%s", eq + 3); } else snprintf(a1, sizeof a1, "%s", rest);
            int rc = hai_chat_cmd(data, ledger, cmds[i][1], a1, a2, (long)time(NULL), out, sizeof out);
            fprintf(stderr, "%s\n", out);
            return rc;
        }
    }
    if (strncmp(t, "/mark ", 6) == 0) {                       /* /mark <word> [high|normal] */
        char word[64] = "", level[16] = "high";
        if (sscanf(t + 6, "%63s %15s", word, level) < 1) return 4;
        int rc = hai_chat_mark(data, ledger, word, level, (long)time(NULL), out, sizeof out);
        fprintf(stderr, "%s\n", out);
        return rc;
    }
    int rc = hai_chat_turn(data, ledger, t, out, sizeof out);
    fprintf(stderr, "%s\n", out);
    return rc == 1 ? 0 : rc;   /* 1 = the chat finished (bye): not an error for the window */
}
