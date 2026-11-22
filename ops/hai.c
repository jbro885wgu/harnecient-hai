/* hai - M1 CLI. Prints the numbered menu, then one reply per line from stdin.
 * Usage: hai            (interactive)   |   printf '1\n2\n' | hai   (piped) */
#include <stdio.h>
#include <string.h>
#include "hai_core.h"

int main(void) {
    char line[512], out[512];
    printf("hai 0.0.0-m1\n%s\n", hai_menu());
    while (printf("hai> "), fflush(stdout), fgets(line, sizeof line, stdin)) {
        int quit = hai_reply(line, out, sizeof out);
        printf("%s\n", out);
        fflush(stdout);
        if (quit) break;
    }
    return 0;
}
