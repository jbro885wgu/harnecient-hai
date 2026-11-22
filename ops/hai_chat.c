/* hai_chat - the FSM chat as a command. The engine is hai_chat_core (shared with hai).
 * Usage: hai_chat turn <datadir> <ledger> <input...>   |   hai_chat state <ledger>
 * Exit: 0 reply, 1 chat done, 2 setup missing, 3 ledger write failed, 4 usage. */
#include <stdio.h>
#include <string.h>
#include "hai_chat_core.h"

int main(int argc, char **argv) {
    char out[1024];
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
    if (argc == 3 && strcmp(argv[1], "state") == 0) {
        hai_chat_state(argv[2], out, sizeof out);
        printf("%s\n", out);
        return 0;
    }
    fprintf(stderr, "usage: hai_chat turn <datadir> <ledger> <input...> | hai_chat state <ledger>\n");
    return 4;
}
