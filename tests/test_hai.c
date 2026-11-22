/* test_hai - M1 checks. Prints PASS|name or FAIL|name; exit 1 if any fail. */
#include <stdio.h>
#include <string.h>
#include "../ops/hai_core.h"

static int fails = 0;
static void check(const char *name, int ok) {
    printf("%s|%s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) fails++;
}

int main(void) {
    char out[512];
    int q;

    check("menu shows numbered rows", strstr(hai_menu(), "[1] Help") && strstr(hai_menu(), "[2] Quit"));

    q = hai_reply("help", out, sizeof out);
    check("help replies and keeps going", q == 0 && strstr(out, "fixed replies") != NULL);

    q = hai_reply("  HELP \n", out, sizeof out);
    check("help is case and space tolerant", q == 0 && strstr(out, "fixed replies") != NULL);

    q = hai_reply("1", out, sizeof out);
    check("number 1 picks help", q == 0 && strstr(out, "fixed replies") != NULL);

    q = hai_reply("2", out, sizeof out);
    check("number 2 picks quit", q == 1 && strcmp(out, "bye") == 0);

    q = hai_reply("quit", out, sizeof out);
    check("quit asks to stop", q == 1 && strcmp(out, "bye") == 0);

    q = hai_reply("", out, sizeof out);
    check("empty line gets a prompt, not a crash", q == 0 && strstr(out, "pick a number") != NULL);

    q = hai_reply("what is the weather", out, sizeof out);
    check("unknown words get the fallback", q == 0 && strstr(out, "don't know") != NULL);

    printf("SUMMARY|fail=%d\n", fails);
    return fails ? 1 : 0;
}
