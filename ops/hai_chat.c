/* hai_chat - the FSM chat as a command. The engine is hai_chat_core (shared with hai and the window).
 * Usage: hai_chat turn  <datadir> <ledger> <input...>
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
    if (argc == 4 && strcmp(argv[1], "stats") == 0) {
        int rc = hai_chat_stats(argv[2], argv[3], out, sizeof out);
        printf("%s", out);
        return rc;
    }
    fprintf(stderr, "usage: hai_chat turn <datadir> <ledger> <input...> | grade <datadir> <ledger> <0-4> | state <ledger> | stats <datadir> <ledger>\n");
    return 8;
}
