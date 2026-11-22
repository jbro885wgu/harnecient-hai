/* hai_core.c - see hai_core.h.
 * Nav standard (from 1.TPMOS README section 7 and DYNAMIC_SUBMENUS_FUNCTIONAL_STD):
 * numbered menu rows, a number key picks the row, typed text is cli input.
 * The menu is printed by the caller from hai_menu(); this file never draws it by hand. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "hai_core.h"

static void trim(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) s[--n] = 0;
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

const char *hai_menu(void) {
    return "[1] Help\n[2] Quit\nType a number, or words.";
}

int hai_reply(const char *line, char *out, size_t out_len) {
    char buf[512];
    snprintf(buf, sizeof buf, "%s", line ? line : "");
    trim(buf);
    for (char *p = buf; *p; p++) *p = (char)tolower((unsigned char)*p);

    /* numbered rows first (nav standard: KEY:n picks a row) */
    if (strcmp(buf, "1") == 0) strcpy(buf, "help");
    else if (strcmp(buf, "2") == 0) strcpy(buf, "quit");

    if (buf[0] == 0) {
        snprintf(out, out_len, "(say something, or pick a number)");
        return 0;
    }
    if (strcmp(buf, "quit") == 0 || strcmp(buf, "exit") == 0) {
        snprintf(out, out_len, "bye");
        return 1;
    }
    if (strcmp(buf, "help") == 0) {
        snprintf(out, out_len, "hai can chat only with fixed replies for now. Chat arrives at M3.");
        return 0;
    }
    snprintf(out, out_len, "I don't know that yet. Pick a number from the menu.");
    return 0;
}
