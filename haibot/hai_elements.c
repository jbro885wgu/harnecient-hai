/* hai_elements <file> - checks the element table. Prints "ELEMENTS n=<count> ok" and exits 0,
 * or prints the first bad row and exits 1. Rules: exactly 10 rows; ids, symbols and names unique;
 * every row has 5 values after ELEMENT; symbol is 2 letters; no value empty. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXE 64

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: hai_elements <file>\n"); return 2; }
    FILE *f = fopen(argv[1], "r");
    if (!f) { printf("BAD cannot open %s\n", argv[1]); return 1; }
    char ids[MAXE][32], syms[MAXE][8], names[MAXE][32];
    int n = 0; char line[512];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (strncmp(line, "ELEMENT", 7) != 0) { printf("BAD not an ELEMENT row: %.60s\n", line); fclose(f); return 1; }
        char *fld[6]; int nf = 0; char *p = line;
        fld[nf++] = p;
        while (*p && nf < 6) { if (*p == '|') { *p = 0; fld[nf++] = p + 1; } p++; }
        if (nf != 6) { printf("BAD row %d has %d fields, want 6 (ELEMENT + 5)\n", n + 1, nf); fclose(f); return 1; }
        for (int i = 1; i < 6; i++) {
            char *s = fld[i]; while (*s == ' ') s++;
            char *e = s + strlen(s); while (e > s && (e[-1] == ' ' || e[-1] == '\n')) *--e = 0;
            fld[i] = s;
            if (!*s) { printf("BAD row %d field %d is empty\n", n + 1, i); fclose(f); return 1; }
        }
        if (n >= MAXE) { printf("BAD more than %d rows\n", MAXE); fclose(f); return 1; }
        snprintf(ids[n], sizeof ids[n], "%s", fld[1]);
        snprintf(syms[n], sizeof syms[n], "%s", fld[2]);
        snprintf(names[n], sizeof names[n], "%s", fld[3]);
        if (strlen(syms[n]) != 2 || !isalpha((unsigned char)syms[n][0]) || !isalpha((unsigned char)syms[n][1])) {
            printf("BAD row %d symbol must be 2 letters: %s\n", n + 1, syms[n]); fclose(f); return 1;
        }
        for (int j = 0; j < n; j++) {
            if (!strcmp(ids[j], ids[n]))   { printf("BAD duplicate id %s\n", ids[n]); fclose(f); return 1; }
            if (!strcmp(syms[j], syms[n])) { printf("BAD duplicate symbol %s\n", syms[n]); fclose(f); return 1; }
            if (!strcmp(names[j], names[n])) { printf("BAD duplicate name %s\n", names[n]); fclose(f); return 1; }
        }
        n++;
    }
    fclose(f);
    if (n != 10) { printf("BAD expected 10 elements, found %d\n", n); return 1; }
    printf("ELEMENTS n=%d ok\n", n);
    return 0;
}
