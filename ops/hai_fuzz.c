/* hai_fuzz.c - a deterministic script generator for the C/JS differential case (2026-11-24, guide 02-architecture/HAI-AND-PALS-REUSE-AND-COMPATIBILITY-GUIDE-2026-11-24.md).
 * Prints N script rows (VERB|args, the same rows a ledger holds) for `hai_world run` and `node docs/world.js run`; the same seed gives the same rows on every machine (own LCG, no libc rand).
 * The rows are deliberately hostile: bad and over-long names, off-grid and wall cells, taken cells, diagonal and huge steps, slots out of range, missing arguments, unknown verbs, and number
 * spellings that C atoi and JS parseInt must read alike ("3x", "+4", " 5", "abc", "-0"). A world built this way reaches the caps and the refusals, not only the happy path.
 * Usage: hai_fuzz <seed> <n> <cols> <rows> <out_file>       Build: gcc -std=gnu11 -Wall -Wextra -O2 -o hai_fuzz hai_fuzz.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long st;
static int early;                                  /* the opening phase spreads agents over the whole grid so the agent cap is reached */
static unsigned rnd(unsigned n) { st = st * 6364136223846793005ULL + 1442695040888963407ULL; return (unsigned)((st >> 33) % n); }

static void name(char *o) {
    static const char *pool[] = { "hai", "prof", "bolt", "scout", "fox", "ann", "bo", "cy", "dee", "eve", "fay", "gus", "hal", "ivy", "jo", "kai", "lee", "max", "ned", "oz", "pam", "quin", "rex", "sam", "tia", "uma", "val", "wes", "xi", "yu", "zed", "a1", "b2", "c3", "d4", "e5" };
    unsigned k = rnd(20);
    if (k < 14) strcpy(o, pool[rnd(sizeof pool / sizeof *pool)]);
    else if (k == 14) strcpy(o, "Upper");                 /* bad: capital */
    else if (k == 15) strcpy(o, "has space");             /* bad: space */
    else if (k == 16) strcpy(o, "sixteen_chars_ok_");     /* 17 chars: over the limit of 16 */
    else if (k == 17) strcpy(o, "exactly_16_chars");      /* 16 chars: exactly at the limit */
    else if (k == 18) strcpy(o, "nobody");                /* usually not an agent */
    else strcpy(o, "x");
}
static void page(char *o) { static const char *p[] = { "home", "home", "home", "garden", "garden", "cave", "BadPage", "p_1" }; strcpy(o, p[rnd(8)]); }
static void num(char *o, int cols, int rows) {
    int span = cols > rows ? cols : rows; unsigned k = rnd(24);
    if (early && k < 20) sprintf(o, "%d", 1 + (int)rnd((unsigned)span - 2));
    else if (k < 8) sprintf(o, "%d", 1 + (int)rnd(4));                        /* a tight cluster (cells 1..4), so agents meet, collide and hit the inner walls */
    else if (k < 16) sprintf(o, "%d", (int)rnd((unsigned)span + 3) - 1);  /* around the grid, one past each edge */
    else if (k == 16) strcpy(o, "3x"); else if (k == 17) strcpy(o, "+4"); else if (k == 18) strcpy(o, " 5"); else if (k == 19) strcpy(o, "abc");
    else if (k == 20) strcpy(o, "-0"); else if (k == 21) strcpy(o, "007"); else if (k == 22) sprintf(o, "%d", -1 - (int)rnd(4)); else strcpy(o, "");
}
static void step(char *o) { static const char *s[] = { "-1", "0", "1", "1", "-1", "1", "-1", "2" }; strcpy(o, s[rnd(8)]); }

int main(int argc, char **argv) {
    if (argc < 6) { fprintf(stderr, "usage: hai_fuzz <seed> <n> <cols> <rows> <out_file>\n"); return 2; }
    if (!freopen(argv[5], "w", stdout)) { perror("out_file"); return 2; }
    st = (unsigned long long)strtoull(argv[1], NULL, 10) * 2862933555777941757ULL + 3037000493ULL;
    int n = atoi(argv[2]), cols = atoi(argv[3]), rows = atoi(argv[4]);
    char a[40], b[40], c[40], d[40];
    for (int i = 0; i < n; i++) {
        unsigned v = rnd(100);
        early = i < 150; if (i < 6 || early) v = 0;                           /* start with a few spawns so the rest has agents to act on */
        if (v < 22) { name(a); page(b); num(c, cols, rows); num(d, cols, rows); printf("SPAWN|%s|%s|%s|%s\n", a, b, c, d); }
        else if (v < 52) { name(a); step(b); step(c); printf("MOVE|%s|%s|%s\n", a, b, c); }
        else if (v < 64) { name(a); page(b); num(c, cols, rows); num(d, cols, rows); printf("TELEPORT|%s|%s|%s|%s\n", a, b, c, d); }
        else if (v < 84) { name(a); sprintf(b, "%d", (int)rnd(8)); printf("REORDER|%s|%s\n", a, b); }
        else if (v < 88) { name(a); printf("MOVE|%s\n", a); }                 /* missing arguments */
        else if (v < 91) { name(a); page(b); printf("SPAWN|%s|%s\n", a, b); }   /* missing arguments */
        else if (v < 94) { name(a); printf("REORDER|%s\n", a); }               /* missing arguments */
        else if (v < 97) printf("DANCE|hai|1\n");                              /* unknown verb */
        else printf("TELEPORT|%s\n", "hai");
    }
    return 0;
}
