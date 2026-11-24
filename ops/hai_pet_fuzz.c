/* hai_pet_fuzz.c - deterministic hostile script generator for the pet C/JS differential case (2026-11-24; guide 02-architecture/HAI-AND-PALS-REUSE-AND-COMPATIBILITY-GUIDE-2026-11-24.md).
 * Rows are ledger rows (BORN|GIVE|TICK|USE|WEIGHT) with the edges that matter: counts 0, 1, 99, 100; minutes 0, 1, 100000, 100001 and odd sizes that force carried remainders; weights 0..41;
 * unknown pets, needs, items and body types; names at and over the length limit; number spellings C atol and JS parseInt must read alike. Own LCG, so the same seed gives the same rows everywhere.
 * Usage: hai_pet_fuzz <seed> <n> <out_file>      Build: gcc -std=gnu11 -Wall -Wextra -O2 -o hai_pet_fuzz hai_pet_fuzz.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long long st;
static unsigned rnd(unsigned n) { st = st * 6364136223846793005ULL + 1442695040888963407ULL; return (unsigned)((st >> 33) % n); }
static const char *pick(const char **a, unsigned n) { return a[rnd(n)]; }
int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: hai_pet_fuzz <seed> <n> <out_file>\n"); return 2; }
    st = (unsigned long long)strtoull(argv[1], NULL, 10) * 2862933555777941757ULL + 3037000493ULL; int n = atoi(argv[2]);
    if (!freopen(argv[3], "w", stdout)) { perror("out_file"); return 2; }
    static const char *pets[] = { "pip", "rob", "cy", "ghost", "has space", "twenty_four_chars_name_ok", "twenty_five_chars_name_xxx", "p1", "p2", "p3", "p4", "p5", "Up" };
    static const char *types[] = { "organic", "organic", "robot", "cyborg", "nothing", "../x", "organic" };
    static const char *needs[] = { "hunger", "drain", "heat", "mood", "nope", "hunger", "drain", "heat" };
    static const char *items[] = { "food.apple", "food.bread", "cell.full", "cell.small", "coolant.pack", "food.apple", "stone", "a.b", "bad item" };
    static const char *nums[] = { "0", "1", "2", "5", "50", "99", "100", "-1", "3x", "+4", " 7", "abc", "", "007", "100000", "100001", "59", "61", "7", "13" };
    static const char *wts[] = { "0", "1", "5", "10", "10", "20", "40", "41", "-1", "x", "" };
    char b[64];
    for (int i = 0; i < n; i++) {
        unsigned v = rnd(100);
        if (i < 5) { printf("BORN|%s|%s\n", pets[i % 3], types[i % 4 == 3 ? 0 : i % 3 + 0 > 2 ? 2 : i % 3]); continue; }  /* a few early births so the rest has pets to act on */
        if (v < 6) { if (rnd(3) == 0) { sprintf(b, "n%u", rnd(30)); printf("BORN|%s|%s\n", b, pick(types, 7)); } else printf("BORN|%s|%s\n", pick(pets, 13), pick(types, 7)); }
        else if (v < 30) { if (rnd(4) == 0) { sprintf(b, "i%u", rnd(40)); printf("GIVE|%s|%s|%s\n", pick(pets, 8), b, pick(nums, 20)); } else printf("GIVE|%s|%s|%s\n", pick(pets, 8), pick(items, 9), pick(nums, 20)); }
        else if (v < 58) { sprintf(b, "%u", 1 + rnd(400)); printf("TICK|%s|%s\n", pick(pets, 8), rnd(3) ? b : pick(nums, 20)); }
        else if (v < 80) printf("USE|%s|%s\n", pick(pets, 8), pick(items, 9));
        else if (v < 92) printf("WEIGHT|%s|%s|%s\n", pick(pets, 8), pick(needs, 8), pick(wts, 11));
        else if (v < 95) printf("GIVE|%s\n", pick(pets, 8));
        else if (v < 97) printf("TICK|%s\n", pick(pets, 8));
        else if (v < 99) printf("WEIGHT|%s|%s\n", pick(pets, 8), pick(needs, 8));
        else printf("SLEEP|%s|1\n", pick(pets, 8));
    }
    return 0;
}
