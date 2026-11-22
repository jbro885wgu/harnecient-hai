/* hai - the CLI. Numbered menu (nav standard), then the FSM chat engine for each line.
 * Typing "grade N" (0 to 4) grades the last reply.
 * Usage: hai [datadir] [ledger]   (defaults: chat/data and chat.ledger in the current folder)
 *        printf 'hi\nbo\n2\n' | hai chat/data chat.ledger   (piped) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hai_chat_core.h"

int main(int argc, char **argv) {
    const char *datadir = argc > 1 ? argv[1] : "chat/data";
    const char *ledger = argc > 2 ? argv[2] : "chat.ledger";
    char line[512], out[1024];
    printf("hai 0.0.0-m5\n[1] Help\n[2] Quit\nType a number, or words. After a reply, type grade 0 to 4.\n");
    while (printf("hai> "), fflush(stdout), fgets(line, sizeof line, stdin)) {
        int rc;
        char *t = line; while (*t == ' ') t++;
        if (strncmp(t, "grade ", 6) == 0) {
            int g = atoi(t + 6);
            char *s = t + 6; while (*s == ' ') s++;
            if (*s < '0' || *s > '9') { printf("type grade 0 to 4\n"); fflush(stdout); continue; }
            rc = hai_chat_grade(datadir, ledger, g, (long)time(NULL), out, sizeof out);
            printf("%s\n", out);
            fflush(stdout);
            continue;
        }
        rc = hai_chat_turn(datadir, ledger, line, out, sizeof out);
        printf("%s\n", out);
        fflush(stdout);
        if (rc != 0) break;
    }
    return 0;
}
