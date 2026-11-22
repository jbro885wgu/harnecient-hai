/* hai_event - runs one event on a page file (E1). The row set is FIXED and documented in
 * book/09-event-pages.md. Anything else is refused with its line number (never guessed).
 *   hai_event <page.pdl> <rules.pdl> <facts.pdl> <event>
 * Output: "ROW <fields joined by |>" for each APPEND, "FACT key=value" for each SET (in order),
 * then "DONE n=<pages run>". Exit: 0 ok, 1 an IF failed nowhere (not used), 2 bad page/rules/facts
 * (message names the line), 3 a value refers to a missing rule or fact.
 * Operands: $name = rule value, @name = fact value, anything else = literal.
 * Facts and rules are "KEY | value" rows (rules: "RULE | key | value"). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXF 256
#define MAXP 64
#define MAXS 64
#define LINE 1024

typedef struct { char k[64], v[256]; } kv_t;
typedef struct { char op[8]; char a[64], b[128]; int line; } if_t;
typedef struct { char verb[8]; char f[8][128]; int nf; int line; } then_t;
typedef struct {
    char name[64], when[64];
    if_t ifs[MAXS]; int nif;
    then_t thens[MAXS]; int nth;
} page_t;

static int split(char *line, char **f, int max) {
    int n = 0; char *p = line;
    f[n++] = p;
    while (*p && n < max) { if (*p == '|') { *p = 0; f[n++] = p + 1; } p++; }
    for (int i = 0; i < n; i++) {
        char *s = f[i]; while (*s == ' ' || *s == '\t') s++; f[i] = s;
        char *e = s + strlen(s); while (e > s && (e[-1] == ' ' || e[-1] == '\n' || e[-1] == '\r')) *--e = 0;
    }
    return n;
}

static int fail(int line, const char *msg, const char *what) {
    fprintf(stderr, "ERR line %d: %s%s%s\n", line, msg, what ? " " : "", what ? what : "");
    return 2;
}

static int load_kv(const char *path, kv_t *arr, int max, int want_rule) {
    FILE *f = fopen(path, "r"); char line[LINE]; int n = 0, ln = 0;
    if (!f) { fprintf(stderr, "ERR cannot open %s\n", path); return -1; }
    while (fgets(line, sizeof line, f)) {
        ln++;
        if (line[0] == '#' || line[0] == '\n') continue;
        char *fld[4]; int nf = split(line, fld, 4);
        if (want_rule) {
            if (nf != 3 || strcmp(fld[0], "RULE") != 0) { fprintf(stderr, "ERR line %d: rules rows are RULE | key | value\n", ln); fclose(f); return -1; }
            if (n >= max) { fclose(f); return -1; }
            snprintf(arr[n].k, sizeof arr[n].k, "%s", fld[1]); snprintf(arr[n].v, sizeof arr[n].v, "%s", fld[2]); n++;
        } else {
            if (nf != 2) { fprintf(stderr, "ERR line %d: facts rows are key | value\n", ln); fclose(f); return -1; }
            if (n >= max) { fclose(f); return -1; }
            snprintf(arr[n].k, sizeof arr[n].k, "%s", fld[0]); snprintf(arr[n].v, sizeof arr[n].v, "%s", fld[1]); n++;
        }
    }
    fclose(f);
    return n;
}

static const char *lookup(kv_t *arr, int n, const char *k) {
    for (int i = n - 1; i >= 0; i--) if (!strcmp(arr[i].k, k)) return arr[i].v;
    return NULL;
}

/* Resolve an operand. Returns 0 ok, 3 missing. */
static int resolve(const char *tok, kv_t *rules, int nr, kv_t *facts, int nf, kv_t *local, int nl, char *out, size_t n) {
    if (tok[0] == '$') {
        const char *v = lookup(rules, nr, tok + 1);
        if (!v) { fprintf(stderr, "ERR missing rule %s\n", tok + 1); return 3; }
        snprintf(out, n, "%s", v); return 0;
    }
    if (tok[0] == '@') {
        const char *v = lookup(local, nl, tok + 1);
        if (!v) v = lookup(facts, nf, tok + 1);
        if (!v) { fprintf(stderr, "ERR missing fact %s\n", tok + 1); return 3; }
        snprintf(out, n, "%s", v); return 0;
    }
    snprintf(out, n, "%s", tok); return 0;
}

static int is_num(const char *s) {
    if (!*s) return 0;
    if (*s == '-') s++;
    if (!*s) return 0;
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

static int compare(const char *op, const char *l, const char *r, int *ok) {
    int cmp; long a, b;
    if (is_num(l) && is_num(r)) { a = atol(l); b = atol(r); cmp = (a > b) - (a < b); }
    else cmp = strcmp(l, r) < 0 ? -1 : (strcmp(l, r) > 0 ? 1 : 0);
    if (!strcmp(op, "eq")) *ok = cmp == 0;
    else if (!strcmp(op, "ne")) *ok = cmp != 0;
    else if (!strcmp(op, "ge")) *ok = cmp >= 0;
    else if (!strcmp(op, "gt")) *ok = cmp > 0;
    else if (!strcmp(op, "le")) *ok = cmp <= 0;
    else if (!strcmp(op, "lt")) *ok = cmp < 0;
    else return 0;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 5) { fprintf(stderr, "usage: hai_event <page.pdl> <rules.pdl> <facts.pdl> <event>\n"); return 2; }
    static kv_t rules[MAXF], facts[MAXF], local[MAXF];
    static page_t pages[MAXP];
    int nr = load_kv(argv[2], rules, MAXF, 1); if (nr < 0) return 2;
    int nf = load_kv(argv[3], facts, MAXF, 0); if (nf < 0) return 2;
    FILE *f = fopen(argv[1], "r"); char line[LINE]; int ln = 0, np = 0;
    if (!f) { fprintf(stderr, "ERR cannot open %s\n", argv[1]); return 2; }
    while (fgets(line, sizeof line, f)) {
        ln++;
        if (line[0] == '#' || line[0] == '\n') continue;
        char *fld[10]; int n = split(line, fld, 10);
        if (!strcmp(fld[0], "PAGE")) {
            if (n != 2 || np >= MAXP) { fclose(f); return fail(ln, "PAGE takes one name", NULL); }
            page_t *p = &pages[np++]; memset(p, 0, sizeof *p); snprintf(p->name, sizeof p->name, "%s", fld[1]);
        } else if (!strcmp(fld[0], "WHEN")) {
            if (np == 0 || n != 2) { fclose(f); return fail(ln, "WHEN needs a PAGE above it and one event", NULL); }
            snprintf(pages[np - 1].when, sizeof pages[np - 1].when, "%s", fld[1]);
        } else if (!strcmp(fld[0], "IF")) {
            if (np == 0 || n != 4 || pages[np - 1].nif >= MAXS) { fclose(f); return fail(ln, "IF is: IF | left | op | right", NULL); }
            if_t *c = &pages[np - 1].ifs[pages[np - 1].nif++];
            snprintf(c->a, sizeof c->a, "%s", fld[1]); snprintf(c->op, sizeof c->op, "%s", fld[2]); snprintf(c->b, sizeof c->b, "%s", fld[3]); c->line = ln;
            if (strcmp(c->op, "eq") && strcmp(c->op, "ne") && strcmp(c->op, "ge") && strcmp(c->op, "gt") && strcmp(c->op, "le") && strcmp(c->op, "lt")) {
                fclose(f); return fail(ln, "unknown IF operator", c->op);
            }
        } else if (!strcmp(fld[0], "THEN")) {
            if (np == 0 || n < 2 || pages[np - 1].nth >= MAXS) { fclose(f); return fail(ln, "THEN needs a PAGE above it", NULL); }
            then_t *t = &pages[np - 1].thens[pages[np - 1].nth++];
            t->line = ln; snprintf(t->verb, sizeof t->verb, "%s", fld[1]); t->nf = 0;
            if (!strcmp(t->verb, "APPEND")) {
                for (int i = 2; i < n && t->nf < 8; i++) snprintf(t->f[t->nf++], sizeof t->f[0], "%s", fld[i]);
            } else if (!strcmp(t->verb, "SET") && n == 4) {
                snprintf(t->f[0], sizeof t->f[0], "%s", fld[2]); snprintf(t->f[1], sizeof t->f[1], "%s", fld[3]); t->nf = 2;
            } else if (!strcmp(t->verb, "STOP") && n == 2) {
                t->nf = 0;
            } else {
                fclose(f); return fail(ln, "THEN verbs are APPEND | fields..., SET | key | value, STOP", t->verb);
            }
        } else {
            fclose(f); return fail(ln, "unknown row kind (refused, not guessed)", fld[0]);
        }
    }
    fclose(f);

    int ran = 0, nlocal = 0;
    for (int p = 0; p < np; p++) {
        if (strcmp(pages[p].when, argv[4]) != 0) continue;
        ran++;
        int pass = 1;
        for (int i = 0; i < pages[p].nif && pass; i++) {
            if_t *c = &pages[p].ifs[i];
            char l[256], r[256];
            if (resolve(c->a, rules, nr, facts, nf, local, nlocal, l, sizeof l)) return 3;
            if (resolve(c->b, rules, nr, facts, nf, local, nlocal, r, sizeof r)) return 3;
            int ok = 0;
            if (!compare(c->op, l, r, &ok)) return fail(c->line, "unknown IF operator", c->op);
            if (!ok) pass = 0;
        }
        if (!pass) continue;
        for (int t = 0; t < pages[p].nth; t++) {
            then_t *th = &pages[p].thens[t];
            if (!strcmp(th->verb, "STOP")) break;
            if (!strcmp(th->verb, "APPEND")) {
                char row[1024] = ""; 
                for (int i = 0; i < th->nf; i++) {
                    char v[256];
                    if (resolve(th->f[i], rules, nr, facts, nf, local, nlocal, v, sizeof v)) return 3;
                    if (i) strncat(row, "|", sizeof row - strlen(row) - 1);
                    strncat(row, v, sizeof row - strlen(row) - 1);
                }
                printf("ROW %s\n", row);
            } else if (!strcmp(th->verb, "SET")) {
                char v[256];
                if (resolve(th->f[1], rules, nr, facts, nf, local, nlocal, v, sizeof v)) return 3;
                snprintf(local[nlocal].k, sizeof local[nlocal].k, "%s", th->f[0]);
                snprintf(local[nlocal].v, sizeof local[nlocal].v, "%s", v); nlocal++;
                printf("FACT %s=%s\n", th->f[0], v);
            }
        }
    }
    printf("DONE n=%d\n", ran);
    return 0;
}
