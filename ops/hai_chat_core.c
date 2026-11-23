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
#define TMAX 64

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
    char pending[64];                                   /* the word hai asked about and is waiting to hear the meaning of */
    struct { char word[64]; char meaning[200]; } taught[TMAX]; int ntaught;
    char marked[TMAX][64]; int nmarked;                  /* words marked "high" attention */
    struct { char word[64], concept[64]; } alias[TMAX]; int nalias;   /* ALIAS rows: a new word that triggers a built-in concept */
    struct { char word[64], text[200]; } say[TMAX]; int nsay;         /* SAY rows: the person's own answer for a word */
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

static void l_define(ledger_t *L, const char *word, const char *meaning) {
    int k = -1; for (int i = 0; i < L->ntaught; i++) if (strcmp(L->taught[i].word, word) == 0) k = i;
    if (k < 0 && L->ntaught < TMAX) k = L->ntaught++;
    if (k >= 0) { snprintf(L->taught[k].word, sizeof L->taught[k].word, "%s", word); snprintf(L->taught[k].meaning, sizeof L->taught[k].meaning, "%s", meaning); }
}
static void l_forget(ledger_t *L, const char *word) {
    for (int i = 0; i < L->ntaught; i++) if (strcmp(L->taught[i].word, word) == 0) { L->taught[i] = L->taught[--L->ntaught]; break; }
    for (int i = 0; i < L->nalias; i++) if (strcmp(L->alias[i].word, word) == 0) { L->alias[i] = L->alias[--L->nalias]; break; }
    for (int i = 0; i < L->nsay; i++) if (strcmp(L->say[i].word, word) == 0) { L->say[i] = L->say[--L->nsay]; break; }
    for (int i = 0; i < L->nmarked; i++) if (strcmp(L->marked[i], word) == 0) { snprintf(L->marked[i], 64, "%s", L->marked[--L->nmarked]); break; }
}

/* One ledger line applied to the replayed state (replay() calls it per line; the viz verb calls it to look at the state after each turn). */
static void replay_line(ledger_t *L, char *line, long today) {
        if (strncmp(line, "TURN|", 5) == 0) {
            char *fld[9]; int nf = split_keep(line, fld, 9);
            if (nf < 8) return;
            L->turns++;
            snprintf(L->state, sizeof L->state, "%s", fld[5]);
            snprintf(L->last_concept, sizeof L->last_concept, "%s", fld[3]);
            snprintf(L->last_reply, sizeof L->last_reply, "%s", fld[4]);
            if (strcmp(fld[6], "store_name") == 0) snprintf(L->name, sizeof L->name, "%s", fld[7]);
            /* the teach loop: ask_word leaves a word pending; store_meaning turns the typed line into that word's meaning */
            if (strcmp(fld[6], "ask_word") == 0) snprintf(L->pending, sizeof L->pending, "%s", fld[7]); else L->pending[0] = 0;
            if (strcmp(fld[6], "store_meaning") == 0 && fld[7][0]) l_define(L, fld[7], fld[2]);
        } else if (strncmp(line, "DEFINE|", 7) == 0) {
            char *fld[6]; int nf = split_keep(line, fld, 6);           /* DEFINE|unix|word|meaning|origin */
            if (nf >= 4) l_define(L, fld[2], fld[3]);
        } else if (strncmp(line, "ALIAS|", 6) == 0) {
            char *fld[6]; int nf = split_keep(line, fld, 6);           /* ALIAS|unix|word|concept|origin */
            if (nf >= 4) {
                int k = -1; for (int i = 0; i < L->nalias; i++) if (strcmp(L->alias[i].word, fld[2]) == 0) k = i;
                if (k < 0 && L->nalias < TMAX) k = L->nalias++;
                if (k >= 0) { snprintf(L->alias[k].word, 64, "%s", fld[2]); snprintf(L->alias[k].concept, 64, "%s", fld[3]); }
            }
        } else if (strncmp(line, "SAY|", 4) == 0) {
            char *fld[6]; int nf = split_keep(line, fld, 6);           /* SAY|unix|word|text|origin */
            if (nf >= 4) {
                int k = -1; for (int i = 0; i < L->nsay; i++) if (strcmp(L->say[i].word, fld[2]) == 0) k = i;
                if (k < 0 && L->nsay < TMAX) k = L->nsay++;
                if (k >= 0) { snprintf(L->say[k].word, 64, "%s", fld[2]); snprintf(L->say[k].text, 200, "%s", fld[3]); }
                int known = 0; for (int i = 0; i < L->ntaught; i++) if (strcmp(L->taught[i].word, fld[2]) == 0) known = 1;
                if (!known) l_define(L, fld[2], "");                   /* a word with only an answer is still a word hai recognises */
            }
        } else if (strncmp(line, "FORGET|", 7) == 0) {
            char *fld[4]; int nf = split_keep(line, fld, 4);           /* FORGET|unix|word */
            if (nf >= 3) l_forget(L, fld[2]);
        } else if (strncmp(line, "MARK|", 5) == 0) {
            char *fld[5]; int nf = split_keep(line, fld, 5);          /* MARK|unix|word|level */
            if (nf < 4) return;
            int k = -1; for (int i = 0; i < L->nmarked; i++) if (strcmp(L->marked[i], fld[2]) == 0) k = i;
            if (strcmp(fld[3], "high") == 0) { if (k < 0 && L->nmarked < TMAX) snprintf(L->marked[L->nmarked++], 64, "%s", fld[2]); }
            else if (k >= 0) { for (int i = k; i + 1 < L->nmarked; i++) snprintf(L->marked[i], 64, "%s", L->marked[i + 1]); L->nmarked--; }
        } else if (strncmp(line, "GRADE|", 6) == 0) {
            char *fld[9]; int nf = split_keep(line, fld, 9);
            if (nf < 9) return;
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

static int replay(const char *ledger, long today, ledger_t *L, const char *datadir) {
    memset(L, 0, sizeof *L);
    snprintf(L->state, sizeof L->state, "start");
    L->last_graded_turn = -1;
    if (datadir && apply_seeds(datadir, L) != 0) return -1;
    FILE *f = fopen(ledger, "r"); if (!f) return 0;
    char line[LINE];
    while (fgets(line, sizeof line, f)) replay_line(L, line, today);
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

/* Replace every {key} in text with val. */
static void fill(char *text, size_t cap, const char *key, const char *val) {
    char pat[80]; snprintf(pat, sizeof pat, "{%s}", key);
    char *p = strstr(text, pat);
    while (p) {
        char tmp[1100]; size_t pre = (size_t)(p - text);
        snprintf(tmp, sizeof tmp, "%.*s%s%s", (int)pre, text, val, p + strlen(pat));
        snprintf(text, cap, "%.*s", (int)cap - 1, tmp);
        p = strstr(text + pre + strlen(val), pat);
    }
}
static int is_stop(const char *dir, const char *w) {
    char path[1024], line[LINE]; snprintf(path, sizeof path, "%s/stopwords.pdl", dir);
    FILE *f = fopen(path, "r"); if (!f) return 0; int hit = 0;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "STOP", 4) != 0) continue;
        char *fld[3]; int nf = split_keep(line, fld, 3);
        if (nf >= 2 && strcmp(fld[1], w) == 0) { hit = 1; break; }
    }
    fclose(f); return hit;
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
        for (int t = 0; t < nt; t++)                                  /* words the person aliased onto this concept */
            for (int a = 0; a < L.nalias; a++)
                if (strcmp(L.alias[a].concept, cs[c].id) == 0 && strcmp(toks[t], L.alias[a].word) == 0) { hits++; break; }
        double w = weight_of(L.concepts, L.nconcepts, cs[c].id);
        if (hits > best_hits || (hits == best_hits && hits > 0 && w > best_w)) { best_hits = hits; best = c; best_w = w; }
    }
    const char *concept = best >= 0 ? cs[best].id : "none";
    /* confidence: how much of the input matched the chosen concept, times how much that concept is trusted */
    int conf = (nt > 0 && best_hits > 0) ? (int)((100.0 * best_hits / nt) * best_w + 0.5) : 0;
    /* WHY: a plain-language record of how this reply was decided (written to the ledger, shown in the window, dumped for tutorials) */
    char why[600] = "";
    if (best >= 0) {
        char m[240] = "";
        for (int t = 0; t < nt; t++) {
            int hit = 0;
            for (int w = 0; w < cs[best].nw && !hit; w++) if (strcmp(toks[t], cs[best].words[w]) == 0) hit = 1;
            for (int a = 0; a < L.nalias && !hit; a++) if (strcmp(L.alias[a].concept, cs[best].id) == 0 && strcmp(toks[t], L.alias[a].word) == 0) hit = 1;
            if (hit && strlen(m) + strlen(toks[t]) + 6 < sizeof m) { char piece[96]; snprintf(piece, sizeof piece, "%s\"%s\"", m[0] ? ", " : "", toks[t]); strcat(m, piece); }
        }
        snprintf(why, sizeof why, "matched %s as %s: %d of %d words x trust %.2f = %d%%", m, cs[best].id, best_hits, nt, best_w, conf);
    } else snprintf(why, sizeof why, "no word matched any meaning I know");

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

    /* the teach loop: a pending question is answered; a taught word is recognised; an unknown content word is asked about */
    char tword[64] = "", tmean[200] = "", ov_concept[96] = "";
    if (rule_long(datadir, "teach_enabled", 0) > 0 && strcmp(L.state, "await_name") != 0 && strcmp(L.state, "done") != 0) {
        long minlen = rule_long(datadir, "teach_min_len", 3); if (minlen < 1) minlen = 3;
        long mult = rule_long(datadir, "attn_high_mult", 1); if (mult < 1) mult = 1;
        if (strcmp(L.state, "await_meaning") == 0 && L.pending[0]) {
            snprintf(tword, sizeof tword, "%s", L.pending);
            snprintf(ov_concept, sizeof ov_concept, "teach"); concept = ov_concept;
            snprintf(to, sizeof to, "idle");
            if (nt == 1 && strcmp(toks[0], "skip") == 0) { snprintf(reply_id, sizeof reply_id, "r_skipped"); snprintf(action, sizeof action, "none"); typed[0] = 0; conf = 0; }
            else { snprintf(reply_id, sizeof reply_id, "r_learned"); snprintf(action, sizeof action, "store_meaning"); snprintf(typed, sizeof typed, "%s", tword); snprintf(tmean, sizeof tmean, "%s", clean); conf = 100; }
            if (strcmp(action, "store_meaning") == 0) snprintf(why, sizeof why, "I asked what \"%s\" means, so I stored your whole answer as its meaning (certain: you told me)", tword);
            else snprintf(why, sizeof why, "I asked what \"%s\" means and you passed, so nothing was stored", tword);
        } else {
            int ncontent = 0; for (int t = 0; t < nt; t++) if (!is_stop(datadir, toks[t])) ncontent++;
            int hit_idx = -1, hits = 0, marked_hit = 0;
            for (int t = 0; t < nt; t++) for (int k = 0; k < L.ntaught; k++) if (strcmp(toks[t], L.taught[k].word) == 0) {
                hits++; if (hit_idx < 0) hit_idx = k;
                for (int m = 0; m < L.nmarked; m++) if (strcmp(L.marked[m], toks[t]) == 0) marked_hit = 1;
            }
            if (hit_idx >= 0 && hits >= best_hits) {      /* a taught word beats a weaker or equal built-in match */
                snprintf(tword, sizeof tword, "%s", L.taught[hit_idx].word); snprintf(tmean, sizeof tmean, "%s", L.taught[hit_idx].meaning);
                snprintf(ov_concept, sizeof ov_concept, "taught:%s", tword); concept = ov_concept;
                snprintf(reply_id, sizeof reply_id, "r_taught"); snprintf(to, sizeof to, "%s", L.state);
                for (int k = 0; k < L.nsay; k++) {
                    if (strcmp(L.say[k].word, tword) != 0) continue;
                    snprintf(reply_id, sizeof reply_id, "r_said"); snprintf(tmean, sizeof tmean, "%s", L.say[k].text);
                }
                snprintf(action, sizeof action, "none"); typed[0] = 0;
                double w = weight_of(L.concepts, L.nconcepts, ov_concept);
                double c = 100.0 * hits * (marked_hit ? mult : 1) / (ncontent > 0 ? ncontent : 1) * w;
                conf = c > 100 ? 100 : (int)(c + 0.5);
                snprintf(why, sizeof why, "you taught me \"%s\"%s: %d hit of %d content words%s x trust %.2f = %d%%", tword, tmean[0] ? "" : " (no meaning, only an answer)", hits, ncontent > 0 ? ncontent : 1,
                         marked_hit ? " (marked: counts x2 or more)" : "", w, conf);
            } else if (best < 0) {
                int pick = -1, pick_marked = 0; size_t pick_len = 0;
                for (int t = 0; t < nt; t++) {
                    if ((long)strlen(toks[t]) < minlen || is_stop(datadir, toks[t])) continue;
                    int known = 0; for (int c = 0; c < nc && !known; c++) for (int w = 0; w < cs[c].nw; w++) if (strcmp(toks[t], cs[c].words[w]) == 0) known = 1;
                    for (int a = 0; a < L.nalias && !known; a++) if (strcmp(L.alias[a].word, toks[t]) == 0) known = 1;
                    if (known) continue;
                    int mk = 0; for (int m = 0; m < L.nmarked; m++) if (strcmp(L.marked[m], toks[t]) == 0) mk = 1;
                    if (pick < 0 || (mk && !pick_marked) || (mk == pick_marked && strlen(toks[t]) > pick_len)) { pick = t; pick_marked = mk; pick_len = strlen(toks[t]); }
                }
                if (pick >= 0) {
                    snprintf(tword, sizeof tword, "%s", toks[pick]);
                    snprintf(ov_concept, sizeof ov_concept, "ask"); concept = ov_concept;
                    snprintf(reply_id, sizeof reply_id, "r_ask_meaning"); snprintf(to, sizeof to, "await_meaning");
                    snprintf(action, sizeof action, "ask_word"); snprintf(typed, sizeof typed, "%s", tword); conf = 0;
                    snprintf(why, sizeof why, "\"%s\" is a content word that is not a keyword, an alias or a taught word, so I ask what it means", tword);
                }
            }
        }
    }

    char text[512] = "";
    for (int i = 0; i < nr; i++) if (strcmp(rs[i].id, reply_id) == 0) { snprintf(text, sizeof text, "%s", rs[i].text); break; }
    if (!text[0]) snprintf(text, sizeof text, "I don't know that yet.");

    if (tword[0]) { fill(text, sizeof text, "word", tword); fill(text, sizeof text, "meaning", tmean); fill(text, sizeof text, "say", tmean); }
    char *nm = strstr(text, "{name}");
    if (nm) {
        char who[64];
        if (strcmp(action, "store_name") == 0) snprintf(who, sizeof who, "%s", typed);
        else snprintf(who, sizeof who, "%s", L.name);
        char tmp[512]; size_t pre = (size_t)(nm - text);
        snprintf(tmp, sizeof tmp, "%.*s%s%s", (int)pre, text, who, nm + 6);
        snprintf(text, sizeof text, "%s", tmp);
    }

    if (strcmp(reply_id, "r_fallback") == 0 && strncmp(why, "no word matched", 15) == 0) snprintf(why, sizeof why, "no word matched any meaning I know and there is no new content word to ask about, so I offer my top options");
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
    {   /* SAID: what hai actually said this turn (a recorded fact), so a window shows the transcript without re-deriving it */
        char said[512]; snprintf(said, sizeof said, "%s", text);
        for (char *p = said; *p; p++) if (*p == '|' || *p == '\n') *p = '/';
        fprintf(f, "SAID|%ld|%d|%s\n", (long)time(NULL), L.turns + 1, said);
        for (char *p = why; *p; p++) if (*p == '|' || *p == '\n') *p = '/';
        fprintf(f, "WHY|%ld|%d|%s\n", (long)time(NULL), L.turns + 1, why);
    }
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

/* Mark a word as deserving more attention ("high") or back to normal. Appends MARK|unix|word|level. 0 ok, 2 setup, 4 bad word or level. */
int hai_chat_mark(const char *datadir, const char *ledger, const char *word, const char *level, long now, char *out, size_t out_len) {
    (void)datadir;
    if (!word || !word[0] || (strcmp(level, "high") != 0 && strcmp(level, "normal") != 0)) { snprintf(out, out_len, "usage: /mark <word> [high|normal]"); return 4; }
    char w[64]; int k = 0;
    for (const char *p = word; *p && k < 63; p++) { if (!isalnum((unsigned char)*p)) { snprintf(out, out_len, "a marked word is letters and digits only"); return 4; } w[k++] = (char)tolower((unsigned char)*p); }
    w[k] = 0;
    FILE *f = fopen(ledger, "a"); if (!f) { snprintf(out, out_len, "cannot write chat ledger"); return 3; }
    fprintf(f, "MARK|%ld|%s|%s\n", now, w, level);
    fclose(f);
    snprintf(out, out_len, "OK marked %s %s", w, level);
    return 0;
}

static int clean_word(const char *in, char *w, size_t cap) {
    size_t k = 0;
    if (!in || !in[0]) return 0;
    for (const char *p = in; *p && k + 1 < cap; p++) { if (!isalnum((unsigned char)*p)) return 0; w[k++] = (char)tolower((unsigned char)*p); }
    w[k] = 0; return k > 0;
}
static void clean_text(const char *in, char *t, size_t cap) {
    snprintf(t, cap, "%s", in ? in : "");
    for (char *p = t; *p; p++) if (*p == '|' || *p == '\n' || *p == '\r') *p = ' ';
}
static void said_row(FILE *f, long now, int turns, const char *text) {
    char said[512]; snprintf(said, sizeof said, "%s", text);
    for (char *p = said; *p; p++) if (*p == '|' || *p == '\n') *p = '/';
    fprintf(f, "SAID|%ld|%d|%s\n", now, turns, said);
}

/* The teaching levers. verb: define | alias | say | forget | pack. Every lever appends rows; nothing else changes state.
 * define word meaning | alias word concept | say word text | forget word | pack name (arg1 = name, reads <datadir>/packs/<name>.pdl).
 * 0 ok, 2 setup, 3 write failed, 4 bad input. */
int hai_chat_cmd(const char *datadir, const char *ledger, const char *verb, const char *arg1, const char *arg2, long now, char *out, size_t out_len) {
    char w[64], t[220];
    ledger_t *L = calloc(1, sizeof *L); if (!L) { snprintf(out, out_len, "out of memory"); return 2; }
    replay(ledger, 0, L, datadir);
    int turns = L->turns; free(L);
    FILE *f;
    if (strcmp(verb, "pack") == 0) {
        char name[64], path[1100], line[LINE]; int rows = 0;
        if (!clean_word(arg1, name, sizeof name)) { snprintf(out, out_len, "usage: /pack <name>"); return 4; }
        snprintf(path, sizeof path, "%s/packs/%s.pdl", datadir, name);
        FILE *pf = fopen(path, "r"); if (!pf) { snprintf(out, out_len, "no pack called %s", name); return 4; }
        f = fopen(ledger, "a"); if (!f) { fclose(pf); snprintf(out, out_len, "cannot write chat ledger"); return 3; }
        fprintf(f, "PACK|%ld|%s\n", now, name);
        while (fgets(line, sizeof line, pf)) {
            if (strncmp(line, "DEFINE", 6) != 0 && strncmp(line, "ALIAS", 5) != 0 && strncmp(line, "SAY", 3) != 0) continue;
            char *fld[4]; int nf = split_keep(line, fld, 4); if (nf < 3 || !clean_word(fld[1], w, sizeof w)) continue;
            clean_text(fld[2], t, sizeof t);
            fprintf(f, "%s|%ld|%s|%s|pack:%s\n", fld[0], now, w, t, name); rows++;
        }
        fclose(pf);
        char msg[160]; snprintf(msg, sizeof msg, "Loaded the %s pack: %d things taught.", name, rows);
        said_row(f, now, turns, msg); fclose(f);
        snprintf(out, out_len, "%s", msg); return 0;
    }
    if (!clean_word(arg1, w, sizeof w)) { snprintf(out, out_len, "a word is letters and digits only"); return 4; }
    char msg[600];
    if (strcmp(verb, "define") == 0 || strcmp(verb, "say") == 0) {
        clean_text(arg2, t, sizeof t); if (!t[0]) { snprintf(out, out_len, "usage: /%s <word> = <text>", strcmp(verb, "define") == 0 ? "teach" : "say"); return 4; }
        f = fopen(ledger, "a"); if (!f) { snprintf(out, out_len, "cannot write chat ledger"); return 3; }
        fprintf(f, "%s|%ld|%s|%s|you\n", strcmp(verb, "define") == 0 ? "DEFINE" : "SAY", now, w, t);
        if (strcmp(verb, "define") == 0) snprintf(msg, sizeof msg, "Thank you. I will remember what you said about \"%s\": %s", w, t);
        else snprintf(msg, sizeof msg, "OK. When you mention \"%s\" I will say: %s", w, t);
    } else if (strcmp(verb, "alias") == 0) {
        char cid[64]; static concept_t cs[MAXC]; int nc = load_concepts(datadir, cs, MAXC), ok = 0;
        if (!clean_word(arg2, cid, sizeof cid) || nc < 0) { snprintf(out, out_len, "usage: /alias <word> = <concept>"); return 4; }
        for (int i = 0; i < nc; i++) if (strcmp(cs[i].id, cid) == 0) ok = 1;
        if (!ok) { char list[400] = ""; size_t off = 0; for (int i = 0; i < nc && off + 70 < sizeof list; i++) off += (size_t)snprintf(list + off, sizeof list - off, "%s%s", cs[i].id, i + 1 < nc ? ", " : ""); snprintf(out, out_len, "I know these meanings to alias to: %s", list); return 4; }
        f = fopen(ledger, "a"); if (!f) { snprintf(out, out_len, "cannot write chat ledger"); return 3; }
        fprintf(f, "ALIAS|%ld|%s|%s|you\n", now, w, cid);
        snprintf(msg, sizeof msg, "OK. \"%s\" now means the same as %s to me.", w, cid);
    } else if (strcmp(verb, "forget") == 0) {
        f = fopen(ledger, "a"); if (!f) { snprintf(out, out_len, "cannot write chat ledger"); return 3; }
        fprintf(f, "FORGET|%ld|%s\n", now, w);
        snprintf(msg, sizeof msg, "OK. I forgot everything you taught me about \"%s\".", w);
    } else { snprintf(out, out_len, "unknown lever %s", verb); return 4; }
    said_row(f, now, turns, msg); fclose(f);
    snprintf(out, out_len, "%s", msg);
    return 0;
}

/* The bank: everything this agent knows, one line per item, for the window's Bank view and for tutorial dumps.
 *   CONCEPT|id|keywords|reply id|trust   TAUGHT|word|meaning|marked 0/1|trust   ALIAS|word|concept   SAY|word|text
 * Replayed from the ledger (the same replay as every turn), so it is exactly what the next reply will use. 0 ok, 2 setup. */
int hai_chat_bank(const char *datadir, const char *ledger, char *out, size_t out_len) {
    static concept_t cs[MAXC]; static ledger_t L;
    int nc = load_concepts(datadir, cs, MAXC);
    if (nc < 0) { snprintf(out, out_len, "setup missing under %s", datadir); return 2; }
    if (replay(ledger, 0, &L, datadir) != 0) { snprintf(out, out_len, "seed refers to an unknown concept"); return 2; }
    size_t off = 0; out[0] = 0;
    #define EMIT(...) do { if (off < out_len) off += (size_t)snprintf(out + off, out_len - off, __VA_ARGS__); } while (0)
    for (int c = 0; c < nc; c++) {
        char kw[300] = ""; for (int w = 0; w < cs[c].nw; w++) { strncat(kw, cs[c].words[w], sizeof kw - strlen(kw) - 2); if (w + 1 < cs[c].nw) strncat(kw, ",", sizeof kw - strlen(kw) - 1); }
        EMIT("CONCEPT|%s|%s|%s|%.2f\n", cs[c].id, kw, cs[c].reply, weight_of(L.concepts, L.nconcepts, cs[c].id));
    }
    for (int i = 0; i < L.ntaught; i++) {
        int mk = 0; for (int m = 0; m < L.nmarked; m++) if (strcmp(L.marked[m], L.taught[i].word) == 0) mk = 1;
        char key[96]; snprintf(key, sizeof key, "taught:%s", L.taught[i].word);
        EMIT("TAUGHT|%s|%s|%d|%.2f\n", L.taught[i].word, L.taught[i].meaning, mk, weight_of(L.concepts, L.nconcepts, key));
    }
    for (int i = 0; i < L.nalias; i++) EMIT("ALIAS|%s|%s\n", L.alias[i].word, L.alias[i].concept);
    for (int i = 0; i < L.nsay; i++) EMIT("SAY|%s|%s\n", L.say[i].word, L.say[i].text);
    #undef EMIT
    return 0;
}

int hai_chat_viz(const char *datadir, const char *ledger, char *out, size_t out_len) {
    static concept_t cs[MAXC]; static ledger_t L;
    int nc = load_concepts(datadir, cs, MAXC);
    if (nc < 0) { snprintf(out, out_len, "setup missing under %s", datadir); return 2; }
    memset(&L, 0, sizeof L); snprintf(L.state, sizeof L.state, "start"); L.last_graded_turn = -1;
    if (apply_seeds(datadir, &L) != 0) { snprintf(out, out_len, "seed refers to an unknown concept"); return 2; }
    size_t off = 0; out[0] = 0;
    #define VEMIT(...) do { if (off < out_len) off += (size_t)snprintf(out + off, out_len - off, __VA_ARGS__); } while (0)
    FILE *f = fopen(ledger, "r");
    if (f) {
        char line[LINE], copy[LINE];
        while (fgets(line, sizeof line, f)) {
            int is_turn = strncmp(line, "TURN|", 5) == 0; char cpt[64] = ""; int conf = 0;
            if (is_turn) { snprintf(copy, sizeof copy, "%s", line); char *fld[9]; int nf = split_keep(copy, fld, 9); if (nf >= 8) { snprintf(cpt, sizeof cpt, "%s", fld[3]); conf = nf >= 9 ? atoi(fld[8]) : 0; } else is_turn = 0; }
            replay_line(&L, line, 0);
            if (is_turn) { VEMIT("CONF|%d|%d|%s\n", L.turns, conf, cpt); VEMIT("KNOW|%d|%d|%d|%d|%d\n", L.turns, L.ntaught, L.nalias, L.nsay, L.nmarked); }
        }
        fclose(f);
    }
    for (int c = 0; c < nc; c++) {
        double r = 0, p = 0; for (int i = 0; i < L.nconcepts; i++) if (strcmp(L.concepts[i].id, cs[c].id) == 0) { r = L.concepts[i].r; p = L.concepts[i].p; }
        VEMIT("TRUST|%s|%.2f|%.0f|%.0f\n", cs[c].id, weight_of(L.concepts, L.nconcepts, cs[c].id), r, p);
    }
    for (int i = 0; i < L.ntaught; i++) {
        char key[96]; snprintf(key, sizeof key, "taught:%s", L.taught[i].word); double r = 0, p = 0;
        for (int k = 0; k < L.nconcepts; k++) if (strcmp(L.concepts[k].id, key) == 0) { r = L.concepts[k].r; p = L.concepts[k].p; }
        VEMIT("TRUST|%s|%.2f|%.0f|%.0f\n", key, weight_of(L.concepts, L.nconcepts, key), r, p);
    }
    {   /* calibration bins, from the same function the calib verb uses */
        static char cal[8192];
        if (hai_chat_calibration(datadir, ledger, cal, sizeof cal) == 0) {
            for (char *ln = strtok(cal, "\n"); ln; ln = strtok(NULL, "\n")) {
                int lo, hi, n, good; double rate, mc;
                if (sscanf(ln, "BIN lo=%d hi=%d n=%d good=%d rate=%lf mean_conf=%lf", &lo, &hi, &n, &good, &rate, &mc) == 6) VEMIT("CAL|%d|%d|%d|%d|%.4f|%.4f\n", lo, hi, n, good, rate, mc);
            }
        }
    }
    #undef VEMIT
    return 0;
}
