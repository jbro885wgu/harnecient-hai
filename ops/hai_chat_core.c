/* hai_chat_core.c - see hai_chat_core.h.
 * Ledger row: TURN|unix|input|concept|reply|state_after|action|typed_name
 * Replay: state starts at "start"; each TURN row sets state_after, and store_name sets the name.
 * Rows are split keeping EMPTY fields (strtok drops them and shifts columns). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "hai_chat_core.h"

#define MAXC 64
#define MAXW 16
#define LINE 2048

typedef struct { char id[64], words[MAXW][64]; int nw; char reply[64], label[64]; } concept_t;
typedef struct { char from[32], concept[64], to[32], action[32]; } next_t;
typedef struct { char id[64], text[512]; } reply_t;

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

static long rule_long(const char *dir, const char *key, long dflt) {
    char path[1024], line[LINE];
    snprintf(path, sizeof path, "%s/chat_rules.pdl", dir);
    FILE *f = fopen(path, "r");
    if (!f) return dflt;
    while (fgets(line, sizeof line, f)) {
        char k[128]; long v;
        if (sscanf(line, "RULE | %127[^|] | %ld", k, &v) == 2) {
            for (char *p = k; *p; p++) if (*p == ' ') { *p = 0; break; }
            if (strcmp(k, key) == 0) { fclose(f); return v; }
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

/* Replay the ledger: state and name come from the TURN rows, never from memory. */
static void replay(const char *ledger, char *state, size_t sn, char *name, size_t nn) {
    snprintf(state, sn, "start"); name[0] = 0;
    FILE *f = fopen(ledger, "r"); if (!f) return;
    char line[LINE];
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "TURN|", 5) != 0) continue;
        char *fld[9]; int nf = split_keep(line, fld, 9);
        if (nf < 8) continue;
        snprintf(state, sn, "%s", fld[5]);
        if (strcmp(fld[6], "store_name") == 0) snprintf(name, nn, "%s", fld[7]);
    }
    fclose(f);
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
    int nc = load_concepts(datadir, cs, MAXC), nr = load_replies(datadir, rs, MAXC), nn = load_next(datadir, ns, 64);
    if (nc < 0 || nr < 0 || nn < 0) { snprintf(out, out_len, "setup missing under %s", datadir); return 2; }
    long input_max = rule_long(datadir, "input_max", 0), options_max = rule_long(datadir, "options_max", 0);
    if (input_max <= 0 || options_max <= 0) { snprintf(out, out_len, "setup missing rules"); return 2; }

    char clean[LINE];
    snprintf(clean, sizeof clean, "%s", input ? input : "");
    for (char *p = clean; *p; p++) if (*p == '|' || *p == '\n' || *p == '\r') *p = ' ';
    if ((long)strlen(clean) > input_max) clean[input_max] = 0;

    char state[32], name[64];
    replay(ledger, state, sizeof state, name, sizeof name);

    /* classify: the concept with the most matched tokens wins; ties go to file order */
    static char toks[64][64];
    int nt = tokens(clean, toks, 64);
    int best = -1, best_hits = 0;
    for (int c = 0; c < nc; c++) {
        int hits = 0;
        for (int t = 0; t < nt; t++)
            for (int w = 0; w < cs[c].nw; w++)
                if (strcmp(toks[t], cs[c].words[w]) == 0) { hits++; break; }
        if (hits > best_hits) { best_hits = hits; best = c; }
    }
    const char *concept = best >= 0 ? cs[best].id : "none";

    /* FSM: the first NEXT row for this state and concept (or *) wins */
    char to[32] = "", action[32] = "none";
    for (int i = 0; i < nn; i++) {
        if (strcmp(ns[i].from, state) != 0) continue;
        if (strcmp(ns[i].concept, "*") != 0 && strcmp(ns[i].concept, concept) != 0) continue;
        snprintf(to, sizeof to, "%s", ns[i].to);
        snprintf(action, sizeof action, "%s", ns[i].action);
        break;
    }
    if (!to[0]) snprintf(to, sizeof to, "%s", state);

    /* the typed name: first token of the input, stored only on store_name */
    char typed[64] = "";
    if (strcmp(action, "store_name") == 0 && nt > 0) snprintf(typed, sizeof typed, "%s", toks[0]);

    /* reply: concept's reply, or the fallback with options */
    char reply_id[64] = "r_fallback";
    if (best >= 0) snprintf(reply_id, sizeof reply_id, "%s", cs[best].reply);
    if (strcmp(state, "await_name") == 0 && strcmp(action, "store_name") == 0) snprintf(reply_id, sizeof reply_id, "r_named");
    if (strcmp(state, "done") == 0) snprintf(reply_id, sizeof reply_id, "r_done");

    char text[512] = "";
    for (int i = 0; i < nr; i++) if (strcmp(rs[i].id, reply_id) == 0) { snprintf(text, sizeof text, "%s", rs[i].text); break; }
    if (!text[0]) snprintf(text, sizeof text, "I don't know that yet.");

    char *nm = strstr(text, "{name}");
    if (nm) {
        char who[64];
        if (strcmp(action, "store_name") == 0) snprintf(who, sizeof who, "%s", typed);
        else snprintf(who, sizeof who, "%s", name);
        char tmp[512]; size_t pre = (size_t)(nm - text);
        snprintf(tmp, sizeof tmp, "%.*s%s%s", (int)pre, text, who, nm + 6);
        snprintf(text, sizeof text, "%s", tmp);
    }

    if (strcmp(reply_id, "r_fallback") == 0) {
        char opts[256] = ""; int shown = 0;
        for (int c = 0; c < nc && shown < options_max; c++) {
            char piece[96]; snprintf(piece, sizeof piece, "%s[%d] %s", shown ? " " : "", shown + 1, cs[c].label);
            strncat(opts, piece, sizeof opts - strlen(opts) - 1);
            shown++;
        }
        char full[512]; snprintf(full, sizeof full, "%s Did you mean: %s?", text, opts);
        snprintf(text, sizeof text, "%s", full);
    }

    FILE *f = fopen(ledger, "a");
    if (!f) { snprintf(out, out_len, "cannot write chat ledger"); return 3; }
    fprintf(f, "TURN|%ld|%s|%s|%s|%s|%s|%s\n", (long)time(NULL), clean, concept, reply_id, to, action, typed);
    fclose(f);

    snprintf(out, out_len, "%s", text);
    if (strcmp(to, "done") == 0 && strcmp(state, "done") != 0) return 1;
    return 0;
}

int hai_chat_state(const char *ledger, char *out, size_t out_len) {
    char state[32], name[64];
    replay(ledger, state, sizeof state, name, sizeof name);
    snprintf(out, out_len, "state=%s name=%s", state, name);
    return 0;
}
