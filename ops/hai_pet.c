/* hai_pet.c - PROOF OF CONCEPT (2026-11-23) for the pet needs model of book 26: a body type (organic, robot, cyborg) is DATA, the needs tick, items satisfy needs, a weight scales a need (0 = off).
 * One authority: an append-only ledger. Needs, remainders, weights and the inventory are whatever replaying the rows gives; nothing else is stored.
 *   BORN|name|type            the pet is born with a body type (data file <data>/body_<type>.pdl)
 *   GIVE|name|item|n          the user puts n items in its inventory
 *   TICK|name|minutes         time passes: every need rises by RATE points per_minutes, scaled by its weight; the sub-point remainder is carried (the house rule of survivor_op)
 *   USE|name|item             uses one item the body knows (a USE row of its file): burns it, moves a need by delta (clamped 0..100)
 *   WEIGHT|name|need|tenths   10 = normal, 0 = that need is off, 40 = four times as fast
 *   REFUSED|verb|name|reason  kept so a refusal can be shown; it changes nothing
 * Body file rows (the SAME shapes as the house needs.pdl / itemdata.pdl, so one data format): BODY|type  RATE|need|points|per_minutes  USE|item|verb|need|delta
 * Not here (book 26): the world, walking, farming, anatomy parts, evolving a body, MP and forms. This only proves the needs core and the body-type-as-data idea.
 *   hai_pet <data_dir> <ledger> born <name> <type> | give <name> <item> <n> | tick <name> <minutes> | use <name> <item> | weight <name> <need> <tenths> | status <name>
 * Exit 0 ok, 3 refused (reason on stdout, a REFUSED row appended), 2 usage. Build: gcc -std=gnu11 -Wall -Wextra -O2 -o hai_pet hai_pet.c */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 8
#define MAXU 16
#define MAXI 16
#define MAXP 16
typedef struct { char need[24]; long pts, per; } rate_t;
typedef struct { char item[32], verb[12], need[24]; long delta; } use_t;
typedef struct { char type[16]; rate_t r[MAXN]; int nr; use_t u[MAXU]; int nu; } body_t;
typedef struct { char name[20]; body_t b; long val[MAXN], rem[MAXN], w[MAXN]; char inv[MAXI][32]; long cnt[MAXI]; int ni; } pet_t;
typedef struct { pet_t p[MAXP]; int n; const char *data; } world_t;

static int ok_name(const char *s, int dots) { size_t n = strlen(s); if (n < 1 || n > 24) return 0; for (size_t i = 0; i < n; i++) { char c = s[i]; if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || (dots && c == '.'))) return 0; } return 1; }
static long clampv(long v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
static int split(char *line, char **f, int max) { int n = 0; char *p = line; while (n < max) { f[n++] = p; char *q = strchr(p, '|'); if (!q) break; *q = 0; p = q + 1; } return n; }

static int load_body(const char *data, const char *type, body_t *b) {
    char path[600]; snprintf(path, sizeof path, "%s/body_%s.pdl", data, type); FILE *fp = fopen(path, "r"); if (!fp) return -1;
    memset(b, 0, sizeof *b); snprintf(b->type, sizeof b->type, "%s", type);
    char line[300];
    while (fgets(line, sizeof line, fp)) {
        line[strcspn(line, "\r\n")] = 0; if (!line[0] || line[0] == '#') continue;
        char *f[6]; int nf = split(line, f, 6);
        if (!strcmp(f[0], "RATE") && nf >= 4 && b->nr < MAXN && ok_name(f[1], 0)) { rate_t *r = &b->r[b->nr++]; snprintf(r->need, sizeof r->need, "%s", f[1]); r->pts = atol(f[2]); r->per = atol(f[3]); if (r->per <= 0) r->per = 60; }
        else if (!strcmp(f[0], "USE") && nf >= 5 && b->nu < MAXU && ok_name(f[1], 1)) { use_t *u = &b->u[b->nu++]; snprintf(u->item, sizeof u->item, "%s", f[1]); snprintf(u->verb, sizeof u->verb, "%s", f[2]); snprintf(u->need, sizeof u->need, "%s", f[3]); u->delta = atol(f[4]); }
    }
    fclose(fp); return b->nr > 0 ? 0 : -1;
}
static pet_t *find(world_t *w, const char *name) { for (int i = 0; i < w->n; i++) if (!strcmp(w->p[i].name, name)) return &w->p[i]; return NULL; }
static int need_ix(pet_t *p, const char *need) { for (int i = 0; i < p->b.nr; i++) if (!strcmp(p->b.r[i].need, need)) return i; return -1; }
static int inv_ix(pet_t *p, const char *item, int create) { for (int i = 0; i < p->ni; i++) if (!strcmp(p->inv[i], item)) return i; if (!create || p->ni >= MAXI) return -1; snprintf(p->inv[p->ni], 32, "%s", item); p->cnt[p->ni] = 0; return p->ni++; }

/* the one rule set: NULL when allowed (and applied), else the reason. Replay and new rows both come through here, so a hand-edited ledger cannot cheat. */
static const char *apply(world_t *w, const char *verb, char **f, int nf) {
    if (!strcmp(verb, "BORN")) {
        if (nf < 2) return "missing arguments";
        if (!ok_name(f[0], 0) || !ok_name(f[1], 0)) return "bad name";
        if (find(w, f[0])) return "name already used";
        if (w->n >= MAXP) return "too many pets";
        body_t b; if (load_body(w->data, f[1], &b) != 0) return "no such body type";
        pet_t *p = &w->p[w->n++]; memset(p, 0, sizeof *p); snprintf(p->name, sizeof p->name, "%s", f[0]); p->b = b; for (int i = 0; i < MAXN; i++) p->w[i] = 10; return NULL;
    }
    pet_t *p = nf ? find(w, f[0]) : NULL; if (!p) return "no such pet";
    if (!strcmp(verb, "GIVE")) {
        if (nf < 3) return "missing arguments";
        if (!ok_name(f[1], 1)) return "bad item name";
        long n = atol(f[2]); if (n < 1 || n > 99) return "count must be 1..99";
        int i = inv_ix(p, f[1], 1); if (i < 0) return "inventory full"; p->cnt[i] += n; return NULL;
    }
    if (!strcmp(verb, "TICK")) {
        if (nf < 2) return "missing arguments";
        long m = atol(f[1]); if (m < 1 || m > 100000) return "minutes must be 1..100000";
        for (int i = 0; i < p->b.nr; i++) { long denom = p->b.r[i].per * 10; long tot = p->rem[i] + m * p->b.r[i].pts * p->w[i]; p->val[i] = clampv(p->val[i] + tot / denom); p->rem[i] = tot % denom; }
        return NULL;
    }
    if (!strcmp(verb, "USE")) {
        if (nf < 2) return "missing arguments";
        use_t *u = NULL; for (int i = 0; i < p->b.nu; i++) if (!strcmp(p->b.u[i].item, f[1])) u = &p->b.u[i];
        if (!u) return "this body cannot use that item";
        int ni = need_ix(p, u->need); if (ni < 0) return "unknown need";
        int ii = inv_ix(p, f[1], 0); if (ii < 0 || p->cnt[ii] < 1) return "holds none of that item";
        p->cnt[ii]--; p->val[ni] = clampv(p->val[ni] + u->delta); return NULL;
    }
    if (!strcmp(verb, "WEIGHT")) {
        if (nf < 3) return "missing arguments";
        int ni = need_ix(p, f[1]); if (ni < 0) return "unknown need";
        long t = atol(f[2]); if (t < 0 || t > 40) return "weight must be 0..40 tenths";
        p->w[ni] = t; return NULL;
    }
    return "unknown verb";
}
static void replay(const char *ledger, world_t *w) {
    FILE *fp = fopen(ledger, "r"); if (!fp) return; char line[400];
    while (fgets(line, sizeof line, fp)) { line[strcspn(line, "\r\n")] = 0; if (!line[0] || line[0] == '#') continue; char *f[8]; int nf = split(line, f, 8); if (nf < 1 || !strcmp(f[0], "REFUSED")) continue; apply(w, f[0], f + 1, nf - 1); }
    fclose(fp);
}
int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: hai_pet <data_dir> <ledger> born|give|tick|use|weight|status <name> ...\n"); return 2; }
    world_t *w = calloc(1, sizeof *w); if (!w) return 2; w->data = argv[1]; const char *ledger = argv[2], *cmd = argv[3];
    replay(ledger, w);
    if (!strcmp(cmd, "status")) {
        pet_t *p = find(w, argv[4]); if (!p) { printf("REFUSED: no such pet\n"); free(w); return 3; }
        printf("NEEDS %s", p->name); for (int i = 0; i < p->b.nr; i++) printf(" %s=%ld", p->b.r[i].need, p->val[i]); printf("\nBODY %s weights", p->b.type); for (int i = 0; i < p->b.nr; i++) printf(" %s=%ld", p->b.r[i].need, p->w[i]); printf("\nINVENTORY"); for (int i = 0; i < p->ni; i++) if (p->cnt[i] > 0) printf(" %s:%ld", p->inv[i], p->cnt[i]); printf("\n"); free(w); return 0;
    }
    char verb[12]; const char *vm[][2] = { { "born", "BORN" }, { "give", "GIVE" }, { "tick", "TICK" }, { "use", "USE" }, { "weight", "WEIGHT" } }; verb[0] = 0;
    for (size_t i = 0; i < sizeof vm / sizeof vm[0]; i++) if (!strcmp(cmd, vm[i][0])) snprintf(verb, sizeof verb, "%s", vm[i][1]);
    if (!verb[0]) { fprintf(stderr, "unknown command %s\n", cmd); free(w); return 2; }
    char *f[6]; int nf = 0; for (int i = 4; i < argc && nf < 6; i++) { if (strchr(argv[i], '|') || strchr(argv[i], '\n')) { fprintf(stderr, "bad character\n"); free(w); return 2; } f[nf++] = argv[i]; }
    const char *why = apply(w, verb, f, nf); FILE *o = fopen(ledger, "a"); if (!o) { perror("ledger"); free(w); return 2; }
    if (why) { fprintf(o, "REFUSED|%s|%s|%s\n", verb, nf ? f[0] : "-", why); fclose(o); printf("REFUSED %s: %s\n", verb, why); free(w); return 3; }
    fprintf(o, "%s", verb); for (int i = 0; i < nf; i++) fprintf(o, "|%s", f[i]); fprintf(o, "\n"); fclose(o); printf("ok %s\n", verb); free(w); return 0;
}
