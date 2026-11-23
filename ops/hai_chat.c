/* hai_chat - the FSM chat as a command. The engine is hai_chat_core (shared with hai and the window).
 * Usage: hai_chat calib <datadir> <ledger>
 *        hai_chat turn  <datadir> <ledger> <input...>
 *        hai_chat grade <datadir> <ledger> <0-4>
 *        hai_chat state <ledger>
 *        hai_chat stats <datadir> <ledger>
 * Exit (turn): 0 reply, 1 chat done, 2 setup missing, 3 ledger write failed.
 * Exit (grade): 0 ok, 2 setup, 3 write failed, 4 bad grade, 5 no reply yet, 6 daily cap, 7 already graded.
 * Exit (usage): 8. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hai_chat_core.h"

int main(int argc, char **argv) {
    char out[2048];
    if (argc >= 4 && strcmp(argv[1], "turn") == 0) {
        char input[1024] = "";
        for (int i = 4; i < argc; i++) {
            if (i > 4) strncat(input, " ", sizeof input - strlen(input) - 1);
            strncat(input, argv[i], sizeof input - strlen(input) - 1);
        }
        int rc = hai_chat_turn(argv[2], argv[3], input, out, sizeof out);
        printf("%s\n", out);
        return rc;
    }
    if (argc == 5 && strcmp(argv[1], "grade") == 0) {
        char *end = NULL;
        long g = strtol(argv[4], &end, 10);
        if (!end || *end != 0 || argv[4][0] == 0) { printf("grade must be 0 to 4\n"); return 4; }
        int rc = hai_chat_grade(argv[2], argv[3], (int)g, (long)time(NULL), out, sizeof out);
        printf("%s\n", out);
        return rc;
    }
    if (argc == 3 && strcmp(argv[1], "state") == 0) {
        hai_chat_state(argv[2], out, sizeof out);
        printf("%s\n", out);
        return 0;
    }
    if (argc == 4 && strcmp(argv[1], "bank") == 0) {             /* hai_chat bank <datadir> <ledger>: everything this agent knows, one line per item */
        static char out[65536];
        int rc = hai_chat_bank(argv[2], argv[3], out, sizeof out);
        fputs(out, stdout); if (rc) putchar('\n');
        return rc;
    }
    if (argc >= 3 && strcmp(argv[1], "why") == 0) {              /* hai_chat why <ledger> [turn number | last]: how that reply was decided */
        FILE *lf = fopen(argv[2], "r"); if (!lf) { printf("no ledger\n"); return 2; }
        int want = (argc > 3 && strcmp(argv[3], "last") != 0) ? atoi(argv[3]) : -1, turn = 0, found = 0; char line[2048], inp[512] = "", w[1200] = "";
        while (fgets(line, sizeof line, lf)) {
            line[strcspn(line, "\r\n")] = 0;
            if (strncmp(line, "TURN|", 5) == 0) { turn++; char *p = strchr(line + 5, '|'); if (p) { char *e = strchr(p + 1, '|'); if (e) *e = 0; snprintf(inp, sizeof inp, "%s", p + 1); } }
            else if (strncmp(line, "WHY|", 4) == 0 && (want < 0 || turn == want)) { char *p = strchr(line + 4, '|'); if (p) p = strchr(p + 1, '|'); snprintf(w, sizeof w, "%s", p ? p + 1 : ""); found = 1; if (want >= 0) break; }
        }
        fclose(lf);
        if (!found) { printf("no WHY row (older ledger, or no such turn)\n"); return 1; }
        printf("you said: %s\nwhy: %s\n", inp, w);
        return 0;
    }
    if (argc >= 6 && strcmp(argv[1], "cmd") == 0) {              /* hai_chat cmd <datadir> <ledger> <lever> <arg1> [arg2] */
        char out[512];
        int rc = hai_chat_cmd(argv[2], argv[3], argv[4], argv[5], argc > 6 ? argv[6] : "", (long)time(NULL), out, sizeof out);
        printf("%s\n", out);
        return rc;
    }
    if (argc >= 5 && strcmp(argv[1], "mark") == 0) {            /* hai_chat mark <datadir> <ledger> <word> [high|normal] */
        char out[256];
        int rc = hai_chat_mark(argv[2], argv[3], argv[4], argc > 5 ? argv[5] : "high", (long)time(NULL), out, sizeof out);
        printf("%s\n", out);
        return rc;
    }
    if (argc == 4 && strcmp(argv[1], "stats") == 0) {
        int rc = hai_chat_stats(argv[2], argv[3], out, sizeof out);
        printf("%s", out);
        return rc;
    }
    if (argc == 4 && strcmp(argv[1], "calib") == 0) {
        int rc = hai_chat_calibration(argv[2], argv[3], out, sizeof out);
        printf("%s", out);
        return rc;
    }
    fprintf(stderr, "usage: hai_chat turn <datadir> <ledger> <input...> | grade <datadir> <ledger> <0-4> | state <ledger> | stats <datadir> <ledger>\n");
    return 8;
}
