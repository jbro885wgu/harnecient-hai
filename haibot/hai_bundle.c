/* hai_bundle - export and reload a haibot ledger (E2). Rows are copied verbatim; nothing is recomputed.
 *   hai_bundle export <ledger> pdl|csv <out>
 *   hai_bundle check  <file>                  (validate, write nothing)
 *   hai_bundle import <file> <ledger>         (ledger must be missing or empty)
 * PDL: first line "# bundle v1", then one ledger row per line.
 * CSV: first line "kind,n,f1,...,f8"; each row: kind, n = total fields including the kind, then the n-1 values (RFC 4180 quoting).
 * Allowed row kinds (the only ones a bundle may carry): TURN (8 fields), GRADE (9 fields).
 * A row of any other kind (a stat, a state, a rule) is refused with its line number.
 * Exit: 0 ok, 2 usage, 3 cannot read or write, 4 bad row (line named), 5 ledger not empty. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXF 16
#define LINE 4096

/* split a pdl row on '|' keeping empty fields (the same as the chat engine) */
static int split_pipe(char *line, char **f, int max) {
    int n = 0; char *p = line; f[n++] = p;
    while (*p && n < max) { if (*p == '|') { *p = 0; f[n++] = p + 1; } p++; }
    return n;
}

/* RFC 4180 CSV split of one line (no embedded newlines). Returns field count. */
static int split_csv(char *line, char out[][512], int max) {
    int n = 0; char *p = line;
    while (1) {
        char *o = out[n]; size_t k = 0;
        if (*p == '"') {
            p++;
            while (*p) {
                if (*p == '"' && p[1] == '"') { if (k < 511) o[k++] = '"'; p += 2; }
                else if (*p == '"') { p++; break; }
                else { if (k < 511) o[k++] = *p; p++; }
            }
            o[k] = 0;
            if (*p == ',') p++;
            else { if (n < max) n++; break; }
        } else {
            while (*p && *p != ',') { if (k < 511) o[k++] = *p; p++; }
            o[k] = 0;
            if (*p == ',') p++;
            else { if (n < max) n++; break; }
        }
        if (n < max) n++; else break;
    }
    return n;
}

static void csv_cell(FILE *o, const char *s) {
    int q = strpbrk(s, ",\"") != NULL;
    if (!q) { fputs(s, o); return; }
    fputc('"', o);
    for (; *s; s++) { if (*s == '"') fputc('"', o); fputc(*s, o); }
    fputc('"', o);
}

static int allowed(const char *kind, int n) {
    if (!strcmp(kind, "TURN")) return n == 8;
    if (!strcmp(kind, "GRADE")) return n == 9;
    return 0;
}

static int check_file(const char *path, FILE **keep_out, int *rows_out) {
    FILE *f = fopen(path, "r");
    if (!f) { printf("ERR cannot read %s\n", path); return 3; }
    char line[LINE]; int ln = 0, rows = 0, csv = 0;
    while (fgets(line, sizeof line, f)) {
        ln++;
        line[strcspn(line, "\r\n")] = 0;
        if (ln == 1) {
            if (!strncmp(line, "kind,n,", 7)) { csv = 1; continue; }
            if (!strcmp(line, "# bundle v1")) continue;
            printf("ERR line 1: not a bundle header\n"); fclose(f); return 4;
        }
        if (line[0] == 0 || line[0] == '#') continue;
        char kind[512]; int n;
        if (csv) {
            static char cells[12][512];
            int nc = split_csv(line, cells, 12);
            if (nc < 2) { printf("ERR line %d: short csv row\n", ln); fclose(f); return 4; }
            snprintf(kind, sizeof kind, "%s", cells[0]); n = atoi(cells[1]);
            if (n < 1 || n > MAXF || nc != n + 1) { printf("ERR line %d: n=%d does not match %d cells\n", ln, n, nc); fclose(f); return 4; }
        } else {
            char *fld[MAXF + 1]; int nf = split_pipe(line, fld, MAXF + 1);
            snprintf(kind, sizeof kind, "%s", fld[0]); n = nf;
        }
        if (!allowed(kind, n)) { printf("ERR line %d: row kind %s with %d fields is not allowed in a bundle\n", ln, kind, n); fclose(f); return 4; }
        rows++;
    }
    fclose(f);
    (void)keep_out;
    *rows_out = rows;
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: hai_bundle export <ledger> pdl|csv <out> | check <file> | import <file> <ledger>\n"); return 2; }
    if (!strcmp(argv[1], "export") && argc == 5) {
        FILE *in = fopen(argv[2], "r");
        FILE *o = fopen(argv[4], "w");
        if (!o) { printf("ERR cannot write %s\n", argv[4]); if (in) fclose(in); return 3; }
        int csv = !strcmp(argv[3], "csv");
        if (!strcmp(argv[3], "pdl")) fputs("# bundle v1\n", o);
        else if (csv) fputs("kind,n,f1,f2,f3,f4,f5,f6,f7,f8,f9\n", o);
        else { fclose(o); if (in) fclose(in); return 2; }
        int rows = 0;
        char line[LINE];
        while (in && fgets(line, sizeof line, in)) {
            line[strcspn(line, "\r\n")] = 0;
            if (!strncmp(line, "TURN|", 5) || !strncmp(line, "GRADE|", 6)) {
                if (!csv) { fprintf(o, "%s\n", line); rows++; continue; }
                char copy[LINE]; snprintf(copy, sizeof copy, "%s", line);
                char *fld[MAXF + 1]; int nf = split_pipe(copy, fld, MAXF + 1);
                csv_cell(o, fld[0]); fprintf(o, ",%d", nf);
                for (int i = 1; i < nf; i++) { fputc(',', o); csv_cell(o, fld[i]); }
                fputc('\n', o); rows++;
            }
        }
        if (in) fclose(in);
        fclose(o);
        printf("OK export rows=%d format=%s\n", rows, argv[3]);
        return 0;
    }
    if (!strcmp(argv[1], "check") && argc == 3) {
        int rows = 0; int rc = check_file(argv[2], NULL, &rows);
        if (rc == 0) printf("OK check rows=%d\n", rows);
        return rc;
    }
    if (!strcmp(argv[1], "import") && argc == 4) {
        int rows = 0; int rc = check_file(argv[2], NULL, &rows);
        if (rc) return rc;
        FILE *l = fopen(argv[3], "r");
        if (l) { int c = fgetc(l); fclose(l); if (c != EOF) { printf("ERR ledger %s is not empty; import into a new ledger\n", argv[3]); return 5; } }
        FILE *in = fopen(argv[2], "r"); FILE *o = fopen(argv[3], "a");
        if (!in || !o) { printf("ERR cannot write %s\n", argv[3]); if (in) fclose(in); if (o) fclose(o); return 3; }
        char line[LINE]; int ln = 0, csv = 0;
        while (fgets(line, sizeof line, in)) {
            ln++; line[strcspn(line, "\r\n")] = 0;
            if (ln == 1) { csv = !strncmp(line, "kind,n,", 7); continue; }
            if (line[0] == 0 || line[0] == '#') continue;
            if (!csv) { fprintf(o, "%s\n", line); continue; }
            char *cells[12]; static char store[12][512]; int nc = 0; char *p = line;
            /* csv -> pdl: the same split as check_file, then re-join fields with '|' */
            static char tmp[12][512];
            int c2 = split_csv(line, tmp, 12); (void)cells; (void)store; (void)p; (void)nc;
            int n = atoi(tmp[1]);
            fputs(tmp[0], o);
            for (int i = 0; i < n - 1 && i + 2 < c2; i++) { fputc('|', o); fputs(tmp[2 + i], o); }
            fputc('\n', o);
        }
        fclose(in); fclose(o);
        printf("OK import rows=%d\n", rows);
        return 0;
    }
    fprintf(stderr, "usage: hai_bundle export <ledger> pdl|csv <out> | check <file> | import <file> <ledger>\n");
    return 2;
}
