/* hai_chat_core.c - see hai_chat_core.h.
 * Rows are split keeping EMPTY fields (strtok drops them and shifts columns).
 * Learning: a grade adds reward/punish from RULE rows (grade_<g>_reward / grade_<g>_punish) into the
 * GRADE row itself, so the ledger alone decides the weights (rules changing later do not rewrite history).
 * Use of weights: (1) the concept with the most matched words wins, and ties go to the HIGHER weight,
 * then to file order; (2) the fallback options are listed by concept weight, highest first.
 * Replies are never rewritten by learning: weights only choose among what was written. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "hai_chat_core.h"

#define MAXC 64
#define MAXW 16
#define LINE 2048
#define MAXL 256

typedef struct { char id[64], words[MAXW][64]; int nw; char reply[64], label[64]; } concept_t;
typedef struct { char from[32], concept[64], to[32], action[32]; } next_t;
typedef struct { char id[64], text[512]; } reply_t;
typedef struct { char id[64]; double r, p; } count_t;

/* Split a '|' row in place, keeping empty fields; trims spaces and the line end. */
static int split_keep(char *line, char **f, int max) {
    int n = 0; char *p = line;
    if (max <= 0) return 0;
    f[n++] = p;
    while (*p && n < max) {
        if (*p == '|') { *p = 0; f[n++] = p + 1; }
        p++;
    }
    for (int i = 0; i < n; i++) {
        char *s = f[i]; while (*s == ' ') s++; f[i] = s;
        char *e = f[i] + strlen(f[i]);
        while (e > f[i] && (e[-1] == ' ' || e[-1] == '\n' || e[-1] == '\r')) *--e = 0;
    }
    return n;
}

static int rule_found = 0;
static long rule_long(const char *dir, const char *key, long dflt) {
    char path[1024], line[LINE];
    snprintf(path, sizeof path, "%s/chat_rules.pdl", dir);
    FILE *f = fopen(path, "r");
    rule_found = 0;
    if (!f) return dflt;
    while (fgets(line, sizeof line, f)) {
        char k[128]; long v;
        if (sscanf(line, "RULE | %127[^|] | %ld", k, &v) == 2) {
            for (char *p = k; *p; p++) if (*p == ' ') { *p = 0; break; }
            if (strcmp(k, key) == 0) { fclose(f); rule_found = 1; return v; }
        }
    }
    fclose(f);
    return dflt;
}

static int load_concepts(const char *dir, concept_t *cs, int max) {
    char path[1024], line[LINE]; int n = 0;
    snprintf(path, sizeof path, "%s/concepts.pdl", dir);
    FILE *f = fopen(path, "r"); if (!f) return -1;
    while (fgets(line, sizeof line, f) && n < max) {
        if (line[0] == '#' || strncmp(line, "CONCEPT", 7) != 0) continue;
        char *fld[5]; int nf = split_keep(line, fld, 5);
        if (nf < 5) continue;
        concept_t *c = &cs[n];
        snprintf(c->id, sizeof c->id, "%s", fld[1]);
        c->nw = 0;
        char words[LINE]; snprintf(words, sizeof words, "%s", fld[2]);
        char *ws = NULL;
        for (char *w = strtok_r(words, ",", &ws); w && c->nw < MAXW; w = strtok_r(NULL, ",", &ws)) {
            while (*w == ' ') w++;
            snprintf(c->words[c->nw++], 64, "%s", w);
        }
        snprintf(c->reply, sizeof c->reply, "%s", fld[3]);
        snprintf(c->label, sizeof c->label, "%s", fld[4]);
        n++;
    }
    fclose(f);
    return n;
}

static int load_replies(const char *dir, reply_t *rs, int max) {
    char path[1024], line[LINE]; int n = 0;
    snprintf(path, sizeof path, "%s/replies.pdl", dir);
    FILE *f = fopen(path, "r"); if (!f) return -1;
    while (fgets(line, sizeof line, f) && n < max) {
        if (line[0] == '#' || strncmp(line, "REPLY", 5) != 0) continue;
        char *fld[3]; int nf = split_keep(line, fld, 3);
        if (nf < 3) continue;
        snprintf(rs[n].id, sizeof rs[n].id, "%s", fld[1]);
        snprintf(rs[n].text, sizeof rs[n].text, "%s", fld[2]);
        n++;
    }
    fclose(f);
    return n;
}

static int load_next(const char *dir, next_t *ns, int max) {
    char path[1024], line[LINE]; int n = 0;
    snprintf(path, sizeof path, "%s/states.pdl", dir);
    FILE *f = fopen(path, "r"); if (!f) return -1;
    while (fgets(line, sizeof line, f) && n < max) {
        if (line[0] == '#' || strncmp(line, "NEXT", 4) != 0) continue;
        char *fld[5]; int nf = split_keep(line, fld, 5);
        if (nf < 5) continue;
        snprintf(ns[n].from, sizeof ns[n].from, "%s", fld[1]);
        snprintf(ns[n].concept, sizeof ns[n].concept, "%s", fld[2]);
        snprintf(ns[n].to, sizeof ns[n].to, "%s", fld[3]);
        snprintf(ns[n].action, sizeof ns[n].action, "%s", fld[4]);
        n++;
    }
    fclose(f);
    return n;
}

/* Everything the ledger says, replayed in one pass. */
typedef struct {
    char state[32], name[64];
    int turns, grades_today;
    char last_concept[64], last_reply[64];
    int last_graded_turn;
    count_t concepts[MAXC]; int nconcepts;
    count_t replies[MAXC];  int nreplies;
} ledger_t;

static count_t *count_for(count_t *arr, int *n, int max, const char *id) {
    for (int i = 0; i < *n; i++) if (strcmp(arr[i].id, id) == 0) return &arr[i];
    if (*n >= max) return NULL;
    snprintf(arr[*n].id, sizeof arr[*n].id, "%s", id);
    arr[*n].r = 0; arr[*n].p = 0;
    return &arr[(*n)++];
}

/* Seeds: hand-set starting counts per concept (chat/data/seeds.pdl, rows "SEED | concept | reward | punish").
 * They are counts like any other, applied before the ledger, so they show in the weights and in stats.
 * A seed for a concept that does not exist is refused (returns -1), never ignored. No seeds file = no seeds. */
static int apply_seeds(const char *dir, ledger_t *L) {
    char path[1024], line[LINE];
    snprintf(path, sizeof path, "%s/seeds.pdl", dir);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    static concept_t cs[MAXC];
    int nc = load_concepts(dir, cs, MAXC);
    if (nc < 0) { fclose(f); return -1; }
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || strncmp(line, "SEED", 4) != 0) continue;
        char *fld[4]; int nf = split_keep(line, fld, 4);
        if (nf != 4) continue;
        int known = 0;
        for (int i = 0; i < nc; i++) if (strcmp(cs[i].id, fld[1]) == 0) known = 1;
        if (!known) { fclose(f); return -1; }
        count_t *c = count_for(L->concepts, &L->nconcepts, MAXC, fld[1]);
        if (c) { c->r = atof(fld[2]); c->p = atof(fld[3]); }
    }
    fclose(f);
    return 0;
}

static int replay(const char *ledger, long today, ledger_t *L, const char *datadir) {
    memset(L, 0, sizeof *L);
    snprintf(L->state, sizeof L->state, "start");
    L->last_graded_turn = -1;
    if (datadir && apply_seeds(datadir, L) != 0) return -1;
    FILE *f = fopen(ledger, "r"); if (!f) return 0;
    char line[LINE];
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "TURN|", 5) == 0) {
            char *fld[9]; int nf = split_keep(line, fld, 9);
            if (nf < 8) continue;
            L->turns++;
            snprintf(L->state, sizeof L->state, "%s", fld[5]);
            snprintf(L->last_concept, sizeof L->last_concept, "%s", fld[3]);
            snprintf(L->last_reply, sizeof L->last_reply, "%s", fld[4]);
            if (strcmp(fld[6], "store_name") == 0) snprintf(L->name, sizeof L->name, "%s", fld[7]);
        } else if (strncmp(line, "GRADE|", 6) == 0) {
            char *fld[9]; int nf = split_keep(line, fld, 9);
            if (nf < 9) continue;
            long day = atol(fld[8]);
            if (day == today) L->grades_today++;
            L->last_graded_turn = atoi(fld[2]);
            double rw = atof(fld[6]), pu = atof(fld[7]);
            count_t *c = count_for(L->concepts, &L->nconcepts, MAXC, fld[3]);
            if (c) { c->r += rw; c->p += pu; }
            count_t *r = count_for(L->replies, &L->nreplies, MAXC, fld[4]);
            if (r) { r->r += rw; r->p += pu; }
        }
    }
    fclose(f);
    return 0;
}

static double weight_of(const count_t *arr, int n, const char *id) {
    for (int i = 0; i < n; i++) if (strcmp(arr[i].id, id) == 0)
        return (arr[i].r + 1.0) / (arr[i].r + arr[i].p + 2.0);
    return 0.5;
}

/* Lowercase alphanumeric tokens. Returns the token count. */
static int tokens(const char *in, char toks[][64], int max) {
    int n = 0, k = 0; char cur[64];
    for (const char *p = in; ; p++) {
        if (*p && isalnum((unsigned char)*p)) { if (k < 63) cur[k++] = (char)tolower((unsigned char)*p); }
        else {
            if (k > 0 && n < max) { cur[k] = 0; snprintf(toks[n++], 64, "%s", cur); }
            k = 0;
            if (!*p) break;
        }
    }
    return n;
}

int hai_chat_turn(const char *datadir, const char *ledger, const char *input, char *out, size_t out_len) {
    static concept_t cs[MAXC]; static reply_t rs[MAXC]; static next_t ns[64];
    static ledger_t L;
    int nc = load_concepts(datadir, cs, MAXC), nr = load_replies(datadir, rs, MAXC), nn = load_next(datadir, ns, 64);
    if (nc < 0 || nr < 0 || nn < 0) { snprintf(out, out_len, "setup missing under %s", datadir); return 2; }
    long input_max = rule_long(datadir, "input_max", 0), options_max = rule_long(datadir, "options_max", 0);
    if (input_max <= 0 || options_max <= 0) { snprintf(out, out_len, "setup missing rules"); return 2; }

    char clean[LINE];
    snprintf(clean, sizeof clean, "%s", input ? input : "");
    for (char *p = clean; *p; p++) if (*p == '|' || *p == '\n' || *p == '\r') *p = ' ';
    if ((long)strlen(clean) > input_max) clean[input_max] = 0;

    if (replay(ledger, 0, &L, datadir) != 0) { snprintf(out, out_len, "seed refers to an unknown concept (see chat/data/seeds.pdl)"); return 2; }

    /* classify: most matched tokens wins; a tie goes to the higher weight, then to file order */
    static char toks[64][64];
    int nt = tokens(clean, toks, 64);
    int best = -1, best_hits = 0; double best_w = -1;
    for (int c = 0; c < nc; c++) {
        int hits = 0;
        for (int t = 0; t < nt; t++)
            for (int w = 0; w < cs[c].nw; w++)
                if (strcmp(toks[t], cs[c].words[w]) == 0) { hits++; break; }
        double w = weight_of(L.concepts, L.nconcepts, cs[c].id);
        if (hits > best_hits || (hits == best_hits && hits > 0 && w > best_w)) { best_hits = hits; best = c; best_w = w; }
    }
    const char *concept = best >= 0 ? cs[best].id : "none";
    /* confidence: how much of the input matched the chosen concept, times how much that concept is trusted */
    int conf = (nt > 0 && best_hits > 0) ? (int)((100.0 * best_hits / nt) * best_w + 0.5) : 0;

    /* FSM: the first NEXT row for this state and concept (or *) wins */
    char to[32] = "", action[32] = "none";
    for (int i = 0; i < nn; i++) {
        if (strcmp(ns[i].from, L.state) != 0) continue;
        if (strcmp(ns[i].concept, "*") != 0 && strcmp(ns[i].concept, concept) != 0) continue;
        snprintf(to, sizeof to, "%s", ns[i].to);
        snprintf(action, sizeof action, "%s", ns[i].action);
        break;
    }
    if (!to[0]) snprintf(to, sizeof to, "%s", L.state);

    char typed[64] = "";
    if (strcmp(action, "store_name") == 0 && nt > 0) snprintf(typed, sizeof typed, "%s", toks[0]);

    char reply_id[64] = "r_fallback";
    if (best >= 0) snprintf(reply_id, sizeof reply_id, "%s", cs[best].reply);
    if (strcmp(L.state, "await_name") == 0 && strcmp(action, "store_name") == 0) snprintf(reply_id, sizeof reply_id, "r_named");
    if (strcmp(L.state, "done") == 0) snprintf(reply_id, sizeof reply_id, "r_done");

    char text[512] = "";
    for (int i = 0; i < nr; i++) if (strcmp(rs[i].id, reply_id) == 0) { snprintf(text, sizeof text, "%s", rs[i].text); break; }
    if (!text[0]) snprintf(text, sizeof text, "I don't know that yet.");

    char *nm = strstr(text, "{name}");
    if (nm) {
        char who[64];
        if (strcmp(action, "store_name") == 0) snprintf(who, sizeof who, "%s", typed);
        else snprintf(who, sizeof who, "%s", L.name);
        char tmp[512]; size_t pre = (size_t)(nm - text);
        snprintf(tmp, sizeof tmp, "%.*s%s%s", (int)pre, text, who, nm + 6);
        snprintf(text, sizeof text, "%s", tmp);
    }

    if (strcmp(reply_id, "r_fallback") == 0) {
        /* options: concepts by weight, highest first; ties keep file order */
        int order[MAXC]; for (int i = 0; i < nc; i++) order[i] = i;
        for (int i = 1; i < nc; i++) {                     /* stable insertion sort, by weight */
            int v = order[i]; double wv = weight_of(L.concepts, L.nconcepts, cs[v].id); int j = i - 1;
            while (j >= 0 && weight_of(L.concepts, L.nconcepts, cs[order[j]].id) < wv) { order[j + 1] = order[j]; j--; }
            order[j + 1] = v;
        }
        char opts[256] = ""; int shown = 0;
        for (int k = 0; k < nc && shown < options_max; k++) {
            char piece[96]; snprintf(piece, sizeof piece, "%s[%d] %s", shown ? " " : "", shown + 1, cs[order[k]].label);
            strncat(opts, piece, sizeof opts - strlen(opts) - 1);
            shown++;
        }
        char full[512]; snprintf(full, sizeof full, "%s Did you mean: %s?", text, opts);
        snprintf(text, sizeof text, "%s", full);
    }

    FILE *f = fopen(ledger, "a");
    if (!f) { snprintf(out, out_len, "cannot write chat ledger"); return 3; }
    fprintf(f, "TURN|%ld|%s|%s|%s|%s|%s|%s|%d\n", (long)time(NULL), clean, concept, reply_id, to, action, typed, conf);
    fclose(f);

    char conftxt[64]; snprintf(conftxt, sizeof conftxt, " (confidence %d%%)", conf);
    if (strlen(text) + strlen(conftxt) < sizeof text) strcat(text, conftxt);
    snprintf(out, out_len, "%s", text);
    if (strcmp(to, "done") == 0 && strcmp(L.state, "done") != 0) return 1;
    return 0;
}

int hai_chat_grade(const char *datadir, const char *ledger, int grade, long now, char *out, size_t out_len) {
    if (grade < 0 || grade > 4) { snprintf(out, out_len, "grade must be 0 to 4"); return 4; }
    char key[64], path[1024];
    long cap = rule_long(datadir, "grade_daily_cap", 0);
    if (cap <= 0) { snprintf(out, out_len, "setup missing rule grade_daily_cap"); return 2; }
    snprintf(key, sizeof key, "grade_%d_reward", grade);
    long reward = rule_long(datadir, key, -1); if (!rule_found) { snprintf(out, out_len, "setup missing rule %s", key); return 2; }
    snprintf(key, sizeof key, "grade_%d_punish", grade);
    long punish = rule_long(datadir, key, -1); if (!rule_found) { snprintf(out, out_len, "setup missing rule %s", key); return 2; }
    (void)path;

    long day = now / 86400;
    ledger_t L; if (replay(ledger, day, &L, datadir) != 0) { snprintf(out, out_len, "seed refers to an unknown concept"); return 2; }
    if (L.turns == 0 || L.last_concept[0] == 0) { snprintf(out, out_len, "no reply to grade yet"); return 5; }
    if (L.last_graded_turn == L.turns) { snprintf(out, out_len, "this reply is already graded"); return 7; }
    if (L.grades_today >= cap) { snprintf(out, out_len, "daily grade limit reached (%ld)", cap); return 6; }

    FILE *f = fopen(ledger, "a");
    if (!f) { snprintf(out, out_len, "cannot write chat ledger"); return 3; }
    fprintf(f, "GRADE|%ld|%d|%s|%s|%d|%ld|%ld|%ld\n", now, L.turns, L.last_concept, L.last_reply, grade, reward, punish, day);
    fclose(f);

    /* the new weight of the reply, after this grade */
    replay(ledger, day, &L, datadir);
    double w = weight_of(L.replies, L.nreplies, L.last_reply);
    snprintf(out, out_len, "OK grade %d for %s weight=%.4f", grade, L.last_reply, w);
    return 0;
}

int hai_chat_stats(const char *datadir, const char *ledger, char *out, size_t out_len) {
    static concept_t cs[MAXC];
    int nc = load_concepts(datadir, cs, MAXC);
    if (nc < 0) { snprintf(out, out_len, "setup missing under %s", datadir); return 2; }
    ledger_t L; if (replay(ledger, 0, &L, datadir) != 0) { snprintf(out, out_len, "seed refers to an unknown concept"); return 2; }
    out[0] = 0;
    char line[256];
    for (int c = 0; c < nc; c++) {
        const count_t *k = NULL;
        for (int i = 0; i < L.nconcepts; i++) if (strcmp(L.concepts[i].id, cs[c].id) == 0) k = &L.concepts[i];
        double r = k ? k->r : 0, p = k ? k->p : 0;
        snprintf(line, sizeof line, "concept %.60s reward=%.0f punish=%.0f weight=%.4f\n", cs[c].id, r, p, (r + 1.0) / (r + p + 2.0));
        strncat(out, line, out_len - strlen(out) - 1);
    }
    return 0;
}

int hai_chat_state(const char *ledger, char *out, size_t out_len) {
    ledger_t L; replay(ledger, 0, &L, NULL);
    snprintf(out, out_len, "state=%s name=%s", L.state, L.name);
    return 0;
}

/* Calibration: does a stated confidence match how often the reply was graded good?
 * Joins each GRADE row to its TURN row by turn number and reads the confidence the turn stored (9th field).
 * Bins of width calib_bin_width (RULE row). A grade counts as good when grade >= good_min_grade (RULE row).
 * ECE = sum over bins of (n_bin / N) * |good rate in bin - mean stated confidence in bin| (0 = perfectly calibrated).
 * Output: "BIN lo=L hi=H n=N good=G rate=R mean_conf=C" per non-empty bin, then "ECE=E graded=N". */
#define MAXT 4096
#define MAXB 101
int hai_chat_calibration(const char *datadir, const char *ledger, char *out, size_t out_len) {
    long width = rule_long(datadir, "calib_bin_width", 0);
    if (width <= 0 || width > 100) { snprintf(out, out_len, "setup missing or bad rule calib_bin_width"); return 2; }
    long good_min = rule_long(datadir, "good_min_grade", -1);
    if (!rule_found) { snprintf(out, out_len, "setup missing rule good_min_grade"); return 2; }
    static int conf[MAXT]; for (int i = 0; i < MAXT; i++) conf[i] = -1;
    int turns = 0;
    static int bn[MAXB], bg[MAXB]; static double bsum[MAXB];
    memset(bn, 0, sizeof bn); memset(bg, 0, sizeof bg); memset(bsum, 0, sizeof bsum);
    FILE *f = fopen(ledger, "r");
    if (f) {
        char line[LINE];
        while (fgets(line, sizeof line, f)) {
            if (strncmp(line, "TURN|", 5) == 0) {
                char *fld[10]; int nf = split_keep(line, fld, 10);
                if (nf < 9) continue;                       /* 8-field rows from before confidence carry none */
                if (turns < MAXT) conf[turns] = atoi(fld[8]);
                turns++;
            } else if (strncmp(line, "GRADE|", 6) == 0) {
                char *fld[9]; int nf = split_keep(line, fld, 9);
                if (nf < 9) continue;
                int t = atoi(fld[2]), g = atoi(fld[5]);
                if (t < 1 || t > MAXT || conf[t - 1] < 0) continue;
                int c = conf[t - 1];
                int b = c / (int)width; if (b >= MAXB) b = MAXB - 1;
                bn[b]++; bsum[b] += c; if (g >= good_min) bg[b]++;
            }
        }
        fclose(f);
    }
    int total = 0; for (int b = 0; b < MAXB; b++) total += bn[b];
    double ece = 0;
    char line[160]; out[0] = 0;
    for (int b = 0; b < MAXB; b++) {
        if (bn[b] == 0) continue;
        int lo = b * (int)width, hi = lo + (int)width - 1; if (hi > 100) hi = 100;
        double rate = (double)bg[b] / bn[b], meanc = bsum[b] / bn[b] / 100.0;
        ece += ((double)bn[b] / total) * (rate > meanc ? rate - meanc : meanc - rate);
        snprintf(line, sizeof line, "BIN lo=%d hi=%d n=%d good=%d rate=%.4f mean_conf=%.4f\n", lo, hi, bn[b], bg[b], rate, meanc);
        strncat(out, line, out_len - strlen(out) - 1);
    }
    snprintf(line, sizeof line, "ECE=%.4f graded=%d\n", ece, total);
    strncat(out, line, out_len - strlen(out) - 1);
    return 0;
}
